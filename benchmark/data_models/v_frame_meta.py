import ctypes


class VFrameMeta(ctypes.BigEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("mediaType", ctypes.c_int32),
        ("frameType", ctypes.c_int32),
        ("bitRate", ctypes.c_int32),
        ("fps", ctypes.c_int32),
        ("timeStamp", ctypes.c_int64),
        ("isMotion", ctypes.c_uint8),
        ("streamType", ctypes.c_uint8),
        ("channelID", ctypes.c_int16),
    ]


class AvfFrameHeader(ctypes.BigEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("refFrameOff", ctypes.c_int64),
        ("mediaType", ctypes.c_uint32),
        ("frameType", ctypes.c_uint32),
        ("timeStamp", ctypes.c_int64),
        ("frameLength", ctypes.c_uint32),
    ]
    def __str__(self) -> str:
        return f"mediaType:  {self.mediaType} frameType {self.frameType} timeStamp {self.timeStamp} frameLength {self.frameLength}"

class AvfFrameFooter(ctypes.BigEndianStructure):
    _pack_ = 1
    _fields_ = [("currentFrameOff", ctypes.c_int64)]


class VFrameInfo(ctypes.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("mediaType", ctypes.c_uint32),
        ("frameType", ctypes.c_uint32),
        ("timeStamp", ctypes.c_int64),
        ("frameLength", ctypes.c_uint32),
    ]

    def __init__(self, avf_frame: AvfFrameHeader) -> None:
        super().__init__()
        self.mediaType = avf_frame.mediaType
        self.frameType = avf_frame.frameType
        self.timeStamp = avf_frame.timeStamp
        self.frameLength = avf_frame.frameLength

    def __str__(self) -> str:
        return f"mediaType:  {self.mediaType} frameType {self.frameType} timeStamp {self.timeStamp} frameLength {self.frameLength}"