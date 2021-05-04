import os
import signal
import sys
import threading
import time
from dataclasses import dataclass, field
from threading import Event, Lock, Thread
import argparse
import cv2
import numpy as np
import ruamel.yaml as yaml
from multiprocessing import Process

import sys

sys.path.append(
    os.path.join(os.path.dirname(__file__), os.pardir, "build/PyNvCodec"))
import PyNvCodec as nvc

from check_cuda.controllers import get_system_info

is_shutdown = Event()


@dataclass
class FpsValue:
    fps: float = 0.0
    frame_id: int = -1


@dataclass
class FpsStats:
    instant_fps: FpsValue = field(default_factory=FpsValue)
    total_fps: float = 0.0
    total_frame_count: int = 0
    total_time_diff: float = 0.0
    min_fps: FpsValue = field(default_factory=FpsValue)
    max_fps: FpsValue = field(default_factory=FpsValue)
    fps_counter: int = 0
    gpu_id: int = 0


class StatusMonitor:
    def __init__(self) -> None:
        self.thred_to_fps = {}
        self.__lock = Lock()

    def put_fps(self, thread_id, frame_count, time_diff, fps, gpu_id):
        with self.__lock:
            if thread_id not in self.thred_to_fps:
                self.thred_to_fps[thread_id] = FpsStats(gpu_id=gpu_id)
                self.thred_to_fps[thread_id].min_fps.fps = sys.float_info.max

            fps_stats = self.thred_to_fps[thread_id]

            frame_id = fps_stats.fps_counter
            fps_value = FpsValue(fps=fps, frame_id=frame_id)

            fps_stats.instant_fps = fps_value
            fps_stats.total_fps += fps_value.fps
            fps_stats.fps_counter += 1
            fps_stats.total_frame_count += frame_count
            fps_stats.total_time_diff += time_diff
            if fps_value.fps < fps_stats.min_fps.fps:
                fps_stats.min_fps.fps = fps_value.fps
                fps_stats.min_fps.frame_id = frame_id

            if fps_value.fps > fps_stats.max_fps.fps:
                fps_stats.max_fps.fps = fps_value.fps
                fps_stats.max_fps.frame_id = frame_id

    def print_fps(self):
        x = ""
        with self.__lock:
            for fps_stats in self.thred_to_fps.values():
                fps_stats.total_fps
                x += (" [" + "{:.1f} {}".format(
                    fps_stats.total_fps / fps_stats.fps_counter,
                    fps_stats.gpu_id) + "] ")
        print(f"{x}")

def stop_handler(*args):
    # del signal_received, frame
    print("")
    print("=============================================")
    print("Bradcasting global shutdown from stop_handler")
    print("=============================================")
    # zope.event.notify(shutdown_event.ShutdownEvent("KeyboardInterrupt received"))
    global is_shutdown
    is_shutdown.set()


status_monitor = StatusMonitor()

class Worker(Thread):
    def __init__(self, gpu_id, channel_id, enc_file, to_width, to_height, fps=25) -> None:
        # https://github.com/NVIDIA/VideoProcessingFramework/issues/119
        # https://blog.paperspace.com/how-to-implement-a-yolo-v3-object-detector-from-scratch-in-pytorch-part-2/
        # super(Process, self).__init__(group=None)
        super().__init__(group=None)
        self.gpu_id = gpu_id
        self.channel_id = channel_id
        self.encFile = enc_file
        self.width = to_width
        self.height = to_height
        self.frame_count = 0
        self.monolithic_frame_counter = 0
        self.last_frame_time = 0

        self.__is_shutdown = Event()
        self.__already_shutting_down = False
        self.frame_gap = 1000.0 / fps

    def run(self):
        print(f"Started {threading.get_ident()}")
        try:
            self.nvDec = nvc.PyNvDecoder(self.encFile, self.gpu_id)
            self.from_nv12_to_yuv = nvc.PySurfaceConverter(
                self.nvDec.Width(),
                self.nvDec.Height(),
                self.nvDec.Format(),
                nvc.PixelFormat.YUV420,
                self.gpu_id,
            )

            self.nv_resizer = nvc.PySurfaceResizer(
                self.nvDec.Width(),
                self.nvDec.Height(),
                self.from_nv12_to_yuv.Format(),
                self.gpu_id,
            )

            self.from_yuv_to_rgb_full_frame = nvc.PySurfaceConverter(
                self.nvDec.Width(),
                self.nvDec.Height(),
                self.from_nv12_to_yuv.Format(),
                nvc.PixelFormat.RGB,
                self.gpu_id,
            )
            self.from_yuv_to_rgb = nvc.PySurfaceConverter(
                self.width,
                self.height,
                self.nv_resizer.Format(),
                nvc.PixelFormat.RGB,
                self.gpu_id,
            )

            self.nv_rgb_download_full_frame = nvc.PySurfaceDownloader(
                self.nvDec.Width(),
                self.nvDec.Height(),
                nvc.PixelFormat.RGB,
                self.gpu_id,
            )
            self.nv_rgb_download = nvc.PySurfaceDownloader(
                self.width,
                self.height,
                nvc.PixelFormat.RGB,
                self.gpu_id,
            )
            self.from_nv12_to_rgb_32f_planar = None
            self.rgb_32f_planar_downloader = None
            self.rgb_32f_planar_frame = None
            self.frame_rate = self.nvDec.Framerate()

            while not self.__is_shutdown.is_set():
                if not self.__decode_and_detect():
                    self.stop()
        except Exception as e:
            print(e, self.channel_id, self.gpu_id)
        print(f"Stopped {threading.get_ident()}")

    
    def stop(self):
        if self.__already_shutting_down:
            return
        self.__already_shutting_down = True
        self.__is_shutdown.set()

    def __decode_and_detect(self):
        t = time.monotonic()
        if self.last_frame_time < 1:
            self.last_frame_time = t
        diff = t - self.last_frame_time
        if diff > 10.0:
            self.fps = self.frame_count / diff
            self.last_frame_time = t
            global status_monitor
            status_monitor.put_fps(
                threading.get_ident(), self.frame_count, diff, self.fps, self.gpu_id
            )
            self.frame_count = 0

            # print(
            #     f"FPS {self.fps:.2f} {self.gpu_id} {self.channel_id} {os.getpid()}"
            # )

        try:
            t_start = (time.time()) * 1000.0
            nv12Surface = self.nvDec.DecodeSingleSurface()
            if nv12Surface.Empty():
                return False
            assert nv12Surface is not None
            if nv12Surface.Empty():
                return False
            if self.from_nv12_to_rgb_32f_planar is None:
                self.from_nv12_to_rgb_32f_planar = nvc.PySurfacePreprocessor(
                    nv12Surface.Width(),
                    nv12Surface.Height(),
                    nv12Surface.Format(),
                    960,
                    640,
                    nvc.PixelFormat.RGB_32F_PLANAR,
                    self.gpu_id,
                )
            rgb_32f_planar_surface = self.from_nv12_to_rgb_32f_planar.Execute(nv12Surface)
            assert rgb_32f_planar_surface is not None
            if rgb_32f_planar_surface.Empty():
                print("rgb_32f_planar_surface error")
                return False

            self.frame_count += 1
            t_end = (time.time()) * 1000.0
            diff2 = t_end - t_start
            # print(f"Time to execute is {diff2}" )
            # time_to_sleep = (self.frame_gap-diff2)/1000.0
            # if time_to_sleep<0:
            #     time_to_sleep = 0.0
            # time.sleep(time_to_sleep)
            # assert ret
            # time.sleep(0.001)

            self.monolithic_frame_counter += 1
        except nvc.HwResetException:
            print("Continue after HW decoder was reset")

        return True


def init_argparser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Deeper look engine")
    # parser.add_argument('--app', type=int, help="Analytics id, e.g. 207")
    # parser.add_argument('--input', help="Input url, e.g. D:/monotosh_todelete/AERunner_base2/video/7.AVI")
    # parser.add_argument('--uid', help="Input uid, e.g. admin")
    # parser.add_argument('--passwd', help="pass, e.g. admin")
    parser.add_argument("number_of_channels", type=int)
    parser.add_argument("gpu_id", type=int)
    return parser

def main():
    signal.signal(signal.SIGINT, stop_handler)
    signal.signal(signal.SIGTERM, stop_handler)
    parser = init_argparser()
    args = parser.parse_args()

    s = get_system_info()
    if args.gpu_id < 0:
        num_of_gpus = len(s.gpus)
    else:
        num_of_gpus = 1

    print("=============================================")
    print(
        "              Started  {} {}               ".format(__name__, args.gpu_id)
    )
    print("=============================================")
    thread_list = []
    try:
        global is_shutdown
        # global status_monitor
        
        for indx in range(num_of_gpus):
            if num_of_gpus == 1 and args.gpu_id >= 0:
                i = args.gpu_id
            else:
                i = indx
            
            print("-------For Video Merged_20200918_90003.mp4 --------------- ")
            print(
                f"---------------------Running on gpu id {s.gpus[i].gpu_id} having name {s.gpus[i].name}-------------------"
            )
            print(
                f"The system is running {s.os} with cpu architecture {s.cpu.name} with clock frequency {s.cpu.frequency} having core count {s.cpu.count} with architecture {s.cpu.arch}"
            )
            print("Running in single channel mode")
            for j in range(args.number_of_channels):
                x = Worker(
                    s.gpus[i].gpu_id,
                    j,
                    "{}/Merged_20200918_90003.mp4".format(os.getcwd()),
                    1920,
                    1088
                )
                thread_list.append(x)
        for x in thread_list:
            x.start()
        while not is_shutdown.wait(10.0):
            status_monitor.print_fps()
            val = True
            for item in thread_list:
                if item.is_alive():
                    val = True
                else:
                    val = False
            if not val:
                break

        for x in thread_list:
            x.stop()

        for x in thread_list:
            x.join()

    except Exception as e:
        print(e)
