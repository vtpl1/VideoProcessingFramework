import logging
import os
import sys

import cv2
import numpy as np

sys.path.append(os.path.join(os.path.dirname(__file__), "build/PyNvCodec"))
import PyNvCodec as nvc
import PyNvCodec as nvc
import pytest


@pytest.fixture
def delete_session_folder():
    os.system("rm -rf session")
    print("Deleted")
    return


def get_folder(sub_folder) -> str:
    session_folder = os.path.join(os.getcwd(), sub_folder)
    if not os.path.exists(session_folder):
        try:
            os.makedirs(session_folder)
            print("{} folder created in {}".format(sub_folder, session_folder))
        except OSError as e:
            print(e)
            raise
    return session_folder + os.path.sep


def get_dump_folder() -> str:
    return get_folder("session")


def write_planar_rgb(file_name, rgb_planar):
    c, w, h = rgb_planar.shape
    img = np.ndarray(shape=(w, h, c), dtype=np.uint8, order="C")
    img[..., 0] = rgb_planar[0]
    img[..., 1] = rgb_planar[1]
    img[..., 2] = rgb_planar[2]
    cv2.imwrite(file_name, img)


def write_rgb(file_name, rgb):
    w, h, c = rgb.shape
    assert c == 3
    img = rgb.astype(np.float)
    write_rgb_32f(file_name, img)


def write_rgb_32f(file_name, rgb_32f):
    w, h, c = rgb_32f.shape
    assert c == 3
    # logging.info(rgb_32f.shape)
    rgb_32f *= 255.0
    img = rgb_32f.astype(np.uint8)
    cv2.imwrite(file_name, img)


def test_vpf_numpy_transform(delete_session_folder, caplog):
    caplog.set_level(logging.INFO)
    logging.info("Start")
    input = os.path.join(os.path.dirname(__file__), "videos/1_2.mp4")
    gpu_id = 0
    nv_dec = None
    from_nv12_to_yuv = None
    frame_count = 0
    yuv_resizer = None
    from_yuv_to_rgb = None

    rgb_downloader = None
    rgb_frame = None
    from_rgb_to_rgb_planar = None
    from_rgb_to_rgb_32f = None
    rgb_32f_downloader = None
    rgb_32f_frame = None
    rgb_planar_frame = None
    rgb_planar_downloader = None
    while True:
        if nv_dec is None:
            nv_dec = nvc.PyNvDecoder(input, gpu_id)
        try:
            nv12_surface = nv_dec.DecodeSingleSurface()
        except nvc.HwResetException:
            continue
        assert nv12_surface is not None
        if nv12_surface.Empty():
            break
        if from_nv12_to_yuv is None:
            from_nv12_to_yuv = nvc.PySurfaceConverter(
                nv12_surface.Width(),
                nv12_surface.Height(),
                nv12_surface.Format(),
                nvc.PixelFormat.YUV420,
                gpu_id,
            )
        yuv_surface = from_nv12_to_yuv.Execute(nv12_surface)
        assert yuv_surface is not None
        if yuv_surface.Empty():
            logging.error("yuv_surface empty")
            break
        if yuv_resizer is None:
            yuv_resizer = nvc.PySurfaceResizer(800, 800, yuv_surface.Format(), gpu_id)
        resized_yuv_surface = yuv_resizer.Execute(yuv_surface)
        assert resized_yuv_surface is not None
        if resized_yuv_surface.Empty():
            logging.error("resized_yuv_surface empty")
            break
        if from_yuv_to_rgb is None:
            from_yuv_to_rgb = nvc.PySurfaceConverter(
                resized_yuv_surface.Width(),
                resized_yuv_surface.Height(),
                resized_yuv_surface.Format(),
                nvc.PixelFormat.RGB,
                gpu_id,
            )
        rgb_surface = from_yuv_to_rgb.Execute(resized_yuv_surface)
        assert rgb_surface is not None
        if rgb_surface.Empty():
            logging.error("rgb_surface empty")
            break
        if from_rgb_to_rgb_planar is None:
            from_rgb_to_rgb_planar = nvc.PySurfaceConverter(
                rgb_surface.Width(),
                rgb_surface.Height(),
                rgb_surface.Format(),
                nvc.PixelFormat.RGB_PLANAR,
                gpu_id,
            )
        rgb_planar_surface = from_rgb_to_rgb_planar.Execute(rgb_surface)
        assert rgb_planar_surface is not None

        if rgb_frame is None:
            rgb_frame = np.ndarray(
                shape=(rgb_surface.Height(), rgb_surface.Width(), 3), dtype=np.uint8
            )
        if rgb_downloader is None:
            rgb_downloader = nvc.PySurfaceDownloader(
                rgb_surface.Width(), rgb_surface.Height(), rgb_surface.Format(), gpu_id
            )
        if not rgb_downloader.DownloadSingleSurface(rgb_surface, rgb_frame):
            logging.error("DownloadSingleSurface error")
            break
        if rgb_planar_frame is None:
            rgb_planar_frame = np.ndarray(
                shape=(3, rgb_planar_surface.Width(), rgb_planar_surface.Height()),
                dtype=np.uint8,
            )
        if rgb_planar_downloader is None:
            rgb_planar_downloader = nvc.PySurfaceDownloader(
                rgb_planar_surface.Width(),
                rgb_planar_surface.Height(),
                rgb_planar_surface.Format(),
                gpu_id,
            )

        if not rgb_planar_downloader.DownloadSingleSurface(
            rgb_planar_surface, rgb_planar_frame
        ):
            logging.error("DownloadSingleSurface error")
            break
        if from_rgb_to_rgb_32f is None:
            from_rgb_to_rgb_32f = nvc.PySurfaceConverter(
                rgb_surface.Width(),
                rgb_surface.Height(),
                rgb_surface.Format(),
                nvc.PixelFormat.RGB_32F,
                gpu_id,
            )
        rgb_32f_surface = from_rgb_to_rgb_32f.Execute(rgb_surface)
        assert rgb_32f_surface is not None
        if rgb_32f_surface.Empty():
            logging.error("rgb_32f_surface error")
            break
        if rgb_32f_downloader is None:
            rgb_32f_downloader = nvc.PySurfaceDownloader(
                rgb_32f_surface.Width(),
                rgb_32f_surface.Height(),
                rgb_32f_surface.Format(),
                gpu_id,
            )
        if rgb_32f_frame is None:
            rgb_32f_frame = np.ndarray(
                shape=(rgb_32f_surface.Width(), rgb_32f_surface.Height(), 3),
                dtype=np.float32,
            )
        if not rgb_32f_downloader.DownloadSingleSurface(rgb_32f_surface, rgb_32f_frame):
            logging.error("DownloadSingleSurface error")
            break
        width = rgb_planar_surface.Width()
        height = rgb_planar_surface.Height()
        rgb_32f_frame = np.reshape(
            rgb_32f_frame, (rgb_32f_surface.Width(), rgb_32f_surface.Height(), 3)
        )
        file_name = f"{get_dump_folder()}out{frame_count:05}.jpg"
        write_rgb_32f(file_name, rgb_32f_frame)
        # WritePpmFromFloat32Planar(rgb_planar_frame, width, height, file_name)
        # cv2.imwrite(file_name, rgb_planar_frame)
        # write_planar_rgb(file_name, rgb_planar_frame)
        # cv2.imshow("vpf", rgb_frame)
        # cv2.waitKey(10)

        frame_count += 1
        logging.info(
            f"{frame_count} planar Width: {rgb_planar_surface.Width()} Height: {rgb_planar_surface.Height()} Format: {rgb_planar_surface.Format()} Pitch: {rgb_planar_surface.Pitch()}"
        )
        logging.info(
            f"{frame_count} rgb    Width: {rgb_surface.Width()} Height: {rgb_surface.Height()} Format: {rgb_surface.Format()} Pitch: {rgb_surface.Pitch()}"
        )
    # cv2.destroyAllWindows()
    assert frame_count == 200


def test_vpf_numpy_transform_rgb32f(delete_session_folder, caplog):
    caplog.set_level(logging.INFO)
    logging.info("Start")
    input = os.path.join(os.path.dirname(__file__), "videos/1_2.mp4")
    gpu_id = 0
    nv_dec = None
    from_nv12_to_yuv = None
    frame_count = 0
    yuv_resizer = None
    from_yuv_to_rgb = None
    from_rgb_to_rgb_32f = None
    from_rgb_32f_to_rgb_32f_planar = None
    rgb_downloader = None
    rgb_frame = None
    rgb_32f_downloader = None
    rgb_32f_frame = None
    rgb_32f_planar_downloader = None
    rgb_32f_planar_frame = None
    while True:
        if nv_dec is None:
            nv_dec = nvc.PyNvDecoder(input, gpu_id)
        try:
            nv12_surface = nv_dec.DecodeSingleSurface()
        except nvc.HwResetException:
            continue
        assert nv12_surface is not None
        if nv12_surface.Empty():
            break
        if from_nv12_to_yuv is None:
            from_nv12_to_yuv = nvc.PySurfaceConverter(
                nv12_surface.Width(),
                nv12_surface.Height(),
                nv12_surface.Format(),
                nvc.PixelFormat.YUV420,
                gpu_id,
            )
        yuv_surface = from_nv12_to_yuv.Execute(nv12_surface)
        assert yuv_surface is not None
        if yuv_surface.Empty():
            logging.error("yuv_surface empty")
            break
        if yuv_resizer is None:
            yuv_resizer = nvc.PySurfaceResizer(800, 800, yuv_surface.Format(), gpu_id)
        resized_yuv_surface = yuv_resizer.Execute(yuv_surface)
        assert resized_yuv_surface is not None
        if resized_yuv_surface.Empty():
            logging.error("resized_yuv_surface empty")
            break
        if from_yuv_to_rgb is None:
            from_yuv_to_rgb = nvc.PySurfaceConverter(
                resized_yuv_surface.Width(),
                resized_yuv_surface.Height(),
                resized_yuv_surface.Format(),
                nvc.PixelFormat.RGB,
                gpu_id,
            )
        rgb_surface = from_yuv_to_rgb.Execute(resized_yuv_surface)
        assert rgb_surface is not None
        if rgb_surface.Empty():
            logging.error("rgb_surface empty")
            break
        if rgb_downloader is None:
            rgb_downloader = nvc.PySurfaceDownloader(
                rgb_surface.Width(),
                rgb_surface.Height(),
                rgb_surface.Format(),
                gpu_id,
            )
        if rgb_frame is None:
            rgb_frame = (
                np.ones(
                    shape=rgb_surface.Width() * rgb_surface.Height() * 3,
                    dtype=np.uint8,
                )
            )
        if not rgb_downloader.DownloadSingleSurface(rgb_surface, rgb_frame):
            logging.error("rgb_32f_downloader DownloadSingleSurface32F error")
            break
        rgb_frame = np.reshape(
            rgb_frame, (rgb_surface.Width(), rgb_surface.Height(), 3)
        )

        if from_rgb_to_rgb_32f is None:
            from_rgb_to_rgb_32f = nvc.PySurfaceConverter(
                rgb_surface.Width(),
                rgb_surface.Height(),
                rgb_surface.Format(),
                nvc.PixelFormat.RGB_32F,
                gpu_id,
            )
        rgb_32f_surface = from_rgb_to_rgb_32f.Execute(rgb_surface)
        assert rgb_32f_surface is not None
        if rgb_32f_surface.Empty():
            logging.error("rgb_32f_surface error")
            break
        if rgb_32f_downloader is None:
            logging.info(f"Downloader surface format {rgb_32f_surface.Format()}")
            rgb_32f_downloader = nvc.PySurfaceDownloader(
                rgb_32f_surface.Width(),
                rgb_32f_surface.Height(),
                rgb_32f_surface.Format(),
                gpu_id,
            )
        if rgb_32f_frame is None:
            rgb_32f_frame = (
                np.ones(
                    shape=rgb_32f_surface.Width() * rgb_32f_surface.Height() * 3,
                    dtype=np.float32,
                )
                * 150.0
            )

        if not rgb_32f_downloader.DownloadSingleSurface(rgb_32f_surface, rgb_32f_frame):
            logging.error("rgb_32f_downloader DownloadSingleSurface32F error")
            break
        rgb_32f_frame = np.reshape(
            rgb_32f_frame, (rgb_32f_surface.Width(), rgb_32f_surface.Height(), 3)
        )

        file_name = f"{get_dump_folder()}out{frame_count:05}.jpg"

        write_rgb_32f(file_name, rgb_32f_frame)

        if from_rgb_32f_to_rgb_32f_planar is None:
            from_rgb_32f_to_rgb_32f_planar = nvc.PySurfaceConverter(
                rgb_32f_surface.Width(),
                rgb_32f_surface.Height(),
                rgb_32f_surface.Format(),
                nvc.PixelFormat.RGB_32F_PLANAR,
                gpu_id,
            )
        rgb_32f_planar_surface = from_rgb_32f_to_rgb_32f_planar.Execute(rgb_32f_surface)
        assert rgb_32f_planar_surface is not None
        if rgb_32f_planar_surface.Empty():
            logging.error("rgb_32f_planar_surface error")
            break
        # WritePpmFromFloat32Planar(rgb_planar_frame, width, height, file_name)
        # cv2.imwrite(file_name, rgb_planar_frame)
        # write_planar_rgb(file_name, rgb_planar_frame)
        cv2.imshow("vpf", rgb_frame)
        cv2.waitKey(10)

        frame_count += 1
        # logging.info(
        #     f"{frame_count} rgb_32f Width: {rgb_32f_surface.Width()} Height: {rgb_32f_surface.Height()} Format: {rgb_32f_surface.Format()} Pitch: {rgb_32f_surface.Pitch()}"
        # )
    cv2.destroyAllWindows()
    assert frame_count == 200

def test_vpf_to_rgb32f_planar(delete_session_folder, caplog):
    caplog.set_level(logging.INFO)
    logging.info("Start")
    input = os.path.join(os.path.dirname(__file__), "videos/1_2.mp4")
    gpu_id = 0
    nv_dec = None
    from_nv12_to_yuv = None
    frame_count = 0
    yuv_resizer = None
    from_yuv_to_rgb = None
    from_rgb_to_rgb_32f = None
    from_rgb_32f_to_rgb_32f_planar = None
    rgb_32f_downloader = None
    rgb_32f_frame = None
    rgb_32f_planar_downloader = None
    rgb_32f_planar_frame = None
    while True:
        if nv_dec is None:
            nv_dec = nvc.PyNvDecoder(input, gpu_id)
        try:
            nv12_surface = nv_dec.DecodeSingleSurface()
        except nvc.HwResetException:
            continue
        assert nv12_surface is not None
        if nv12_surface.Empty():
            break
        frame_count += 1

    assert frame_count == 200