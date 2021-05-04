from ctypes import sizeof
from typing import Tuple

from .data_models.v_frame_meta import (AvfFrameFooter, AvfFrameHeader,
                                       VFrameInfo)


class AvfReader:
    MAGIC_STR = bytes("00dc", "utf-8")

    def __init__(self, file_path) -> None:
        self.__file_path = file_path
        self.__file = None

    def open(self) -> bool:
        try:
            self.__file = open(file=self.__file_path, mode="rb")
            print(f"Opened file {self.__file_path}")
            return True
        except FileNotFoundError as e:
            print("FileNotFoundError", e)
        return False

    def close(self):
        if self.__file:
            self.__file.close()

    def __del__(self):
        self.close()

    def get_frame(self) -> Tuple[VFrameInfo, bytes]:
        frame_info = None
        frame_data = None
        if not self.__file:
            if not self.open():
                return (frame_info, frame_data)

        magic_str = None
        magic_str = self.__file.read(4)
        if magic_str == AvfReader.MAGIC_STR:
            data_array = self.__file.read(sizeof(AvfFrameHeader))
            avf_frame_header = AvfFrameHeader.from_buffer_copy(data_array)
            frame_info = VFrameInfo(avf_frame_header)
            # print(" ", frame_info)
            frame_data = self.__file.read(frame_info.frameLength)
            data_array = self.__file.read(sizeof(AvfFrameFooter))
            # print(" %s " % (avf_frame_header))
            _ = AvfFrameFooter.from_buffer_copy(data_array)

        else:
            print("no match")
        return (frame_info, frame_data)


def main():
    x = AvfReader("/workspaces/VImageCodecCUDA2021/vid/1.AVF")
    x.open()
    count = 0
    while True:
        frame_info, frame_data = x.get_frame()
        if not frame_info:
            break
        count += 1
        if count >= 20:
            break
    x.close()
