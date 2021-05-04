import os
import sys
sys.path.append(os.path.join(os.path.dirname(__file__), "build/PyNvCodec"))
import PyNvCodec as nvc
import numpy as np
import sys

from threading import Thread

class Worker(Thread):
    def __init__(self, gpuID, encFile):
        Thread.__init__(self)

        #self.nvDec = nvc.PyNvDecoder(encFile, gpuID, {'rtsp_transport': 'tcp', 'max_delay': '5000000', 'bufsize': '30000k'})
        self.nvDec = nvc.PyNvDecoder(encFile, gpuID)
        
        width, height = self.nvDec.Width(), self.nvDec.Height()
        hwidth, hheight = int(width / 2), int(height / 2)

        self.nvCvt = nvc.PySurfaceConverter(width, height, self.nvDec.Format(), nvc.PixelFormat.YUV420, gpuID)
        self.nvRes = nvc.PySurfaceResizer(hwidth, hheight, self.nvCvt.Format(), gpuID)
        self.nvDwn = nvc.PySurfaceDownloader(hwidth, hheight, self.nvRes.Format(), gpuID)
        self.num_frame = 0

    def run(self):
        try:
            while True:
                try:
                    rawSurface = self.nvDec.DecodeSingleSurface()
                    if (rawSurface.Empty()):
                        print('No more video frames')
                        break
                except nvc.HwResetException:
                    print('Continue after HW decoder was reset')
                    continue
 
                cvtSurface = self.nvCvt.Execute(rawSurface)
                if (cvtSurface.Empty()):
                    print('Failed to do color conversion')
                    break

                resSurface = self.nvRes.Execute(cvtSurface)
                if (resSurface.Empty()):
                    print('Failed to resize surface')
                    break
 
                rawFrame = np.ndarray(shape=(resSurface.HostSize()), dtype=np.uint8)
                success = self.nvDwn.DownloadSingleSurface(resSurface, rawFrame)
                if not (success):
                    print('Failed to download surface')
                    break
 
                self.num_frame += 1
                if( 0 == self.num_frame % self.nvDec.Framerate() ):
                    print(self.num_frame)
 
        except Exception as e:
            print(getattr(e, 'message', str(e)))
            # decFile.close()
 
def create_threads(gpu_id1, input_file1, gpu_id2, input_file2):
    th_list = []
    for _ in range(70):
        th_list.append(Worker(gpu_id1, input_file1))
    for th in th_list:
        th.start()

    for th in th_list:
        th.join()

 
    # th1  = Worker(gpu_id1, input_file1)
    # th2  = Worker(gpu_id1, input_file1)
 
    # th1.start()
    # th2.start()
 
    # th1.join()
    # th2.join()
 
if __name__ == "__main__":

    print('This sample decodes video stream in 2 parallel threads. It does not save output.')
    print('GPU-accelerated color conversion and resize are also applied.')
    print('Network input such as RTSP is supported as well.')
    print('This sample may serve as a stability test.')
    print('Usage: python SampleDecodeMultiThread.py $gpu_id_0 $input_0 $gpu_id_1 $input_1')
 
    if(len(sys.argv) < 3):
        print("Provide input CLI arguments as shown above")
        exit(1)
 
    gpu_1 = int(sys.argv[1])
    input_1 = sys.argv[2]
    # gpu_2 = int(sys.argv[3])
    # input_2 = sys.argv[4]
    gpu_2 = 0
    input_2 = ""
 
    create_threads(gpu_1, input_1, gpu_2, input_2)