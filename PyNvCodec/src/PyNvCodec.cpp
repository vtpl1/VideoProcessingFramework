/*
 * Copyright 2019 NVIDIA Corporation
 * Copyright 2021 Kognia Sports Intelligence
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *    http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "PyNvCodec.hpp"

using namespace std;
using namespace VPF;
using namespace chrono;

#ifdef GENERATE_PYTHON_BINDINGS
namespace py = pybind11;
#endif

constexpr auto TASK_EXEC_SUCCESS = TaskExecStatus::TASK_EXEC_SUCCESS;
constexpr auto TASK_EXEC_FAIL = TaskExecStatus::TASK_EXEC_FAIL;



PyFfmpegDecoder::PyFfmpegDecoder(const string &pathToFile,
                                 const map<string, string> &ffmpeg_options) {
  //NvDecoderClInterface cli_iface(ffmpeg_options);
  upDecoder.reset(FfmpegDecodeFrame::Make(pathToFile.c_str()));
}
#ifdef GENERATE_PYTHON_BINDINGS
bool PyFfmpegDecoder::DecodeSingleFrame(py::array_t<uint8_t> &frame) {
#else
bool PyFfmpegDecoder::DecodeSingleFrame(std::vector<uint8_t> &frame) {
#endif
  if (TASK_EXEC_SUCCESS == upDecoder->Execute()) {
    auto pRawFrame = (Buffer *)upDecoder->GetOutput(0U);
    if (pRawFrame) {
      auto const frame_size = pRawFrame->GetRawMemSize();
      if (frame_size != frame.size()) {
        frame.resize({frame_size}, false);
      }
#ifdef GENERATE_PYTHON_BINDINGS
      memcpy(frame.mutable_data(), pRawFrame->GetRawMemPtr(), frame_size);
#else
      memcpy(frame.data(), pRawFrame->GetRawMemPtr(), frame_size);
#endif
      return true;
    }
  }
  return false;
}

void *PyFfmpegDecoder::GetSideData(AVFrameSideDataType data_type,
                                   size_t &raw_size) {
  if (TASK_EXEC_SUCCESS == upDecoder->GetSideData(data_type)) {
    auto pSideData = (Buffer *)upDecoder->GetOutput(1U);
    if (pSideData) {
      raw_size = pSideData->GetRawMemSize();
      return pSideData->GetDataAs<void>();
    }
  }
  return nullptr;
}
#ifdef GENERATE_PYTHON_BINDINGS
py::array_t<MotionVector> PyFfmpegDecoder::GetMotionVectors() {
#else
std::vector<MotionVector> PyFfmpegDecoder::GetMotionVectors() {
#endif
  size_t size = 0U;
  auto ptr = (AVMotionVector *)GetSideData(AV_FRAME_DATA_MOTION_VECTORS, size);
  size /= sizeof(*ptr);

  if (ptr && size) {
#ifdef GENERATE_PYTHON_BINDINGS
    py::array_t<MotionVector> mv({size});
    auto req = mv.request(true);
    auto mvc = static_cast<MotionVector *>(req.ptr);
    for (auto i = 0; i < req.shape[0]; i++) {
#else
    std::vector<MotionVector> mv({size});
    auto mvc = static_cast<MotionVector *>(mv.data());
    for (auto i = 0; i < mv.size(); i++) {
#endif    
      mvc[i].source = ptr[i].source;
      mvc[i].w = ptr[i].w;
      mvc[i].h = ptr[i].h;
      mvc[i].src_x = ptr[i].src_x;
      mvc[i].src_y = ptr[i].src_y;
      mvc[i].dst_x = ptr[i].dst_x;
      mvc[i].dst_y = ptr[i].dst_y;
      mvc[i].motion_x = ptr[i].motion_x;
      mvc[i].motion_y = ptr[i].motion_y;
      mvc[i].motion_scale = ptr[i].motion_scale;
    }

    return move(mv);
  }
#ifdef GENERATE_PYTHON_BINDINGS
  return move(py::array_t<MotionVector>({0}));
#else
  return move(std::vector<MotionVector>({0}));
#endif
}

PyFFmpegDemuxer::PyFFmpegDemuxer(const string &pathToFile)
    : PyFFmpegDemuxer(pathToFile, map<string, string>()) {}

PyFFmpegDemuxer::PyFFmpegDemuxer(const string &pathToFile,
                                 const map<string, string> &ffmpeg_options) {
  vector<const char *> options;
  for (auto &pair : ffmpeg_options) {
    options.push_back(pair.first.c_str());
    options.push_back(pair.second.c_str());
  }
  upDemuxer.reset(
      DemuxFrame::Make(pathToFile.c_str(), options.data(), options.size()));
}
#ifdef GENERATE_PYTHON_BINDINGS
bool PyFFmpegDemuxer::DemuxSinglePacket(py::array_t<uint8_t> &packet) {
#else
bool PyFFmpegDemuxer::DemuxSinglePacket(std::vector<uint8_t> &packet) {
#endif
  Buffer *elementaryVideo = nullptr;
  do {
    if (TASK_EXEC_FAIL == upDemuxer->Execute()) {
      upDemuxer->ClearInputs();
      return false;
    }
    elementaryVideo = (Buffer *)upDemuxer->GetOutput(0U);
  } while (!elementaryVideo);

  packet.resize({elementaryVideo->GetRawMemSize()}, false);
#ifdef GENERATE_PYTHON_BINDINGS
  memcpy(packet.mutable_data(), elementaryVideo->GetDataAs<void>(),
         elementaryVideo->GetRawMemSize());
#else
  memcpy(packet.data(), elementaryVideo->GetDataAs<void>(),
         elementaryVideo->GetRawMemSize());
#endif

  upDemuxer->ClearInputs();
  return true;
}

void PyFFmpegDemuxer::GetLastPacketData(PacketData &pkt_data) {
  auto pkt_data_buf = (Buffer*)upDemuxer->GetOutput(3U);
  if (pkt_data_buf) {
    auto pkt_data_ptr = pkt_data_buf->GetDataAs<PacketData>();
    pkt_data = *pkt_data_ptr;
  }
}

uint32_t PyFFmpegDemuxer::Width() const {
  MuxingParams params;
  upDemuxer->GetParams(params);
  return params.videoContext.width;
}

ColorSpace PyFFmpegDemuxer::GetColorSpace() const {
  MuxingParams params;
  upDemuxer->GetParams(params);
  return params.videoContext.color_space;
};

ColorRange PyFFmpegDemuxer::GetColorRange() const {
  MuxingParams params;
  upDemuxer->GetParams(params);
  return params.videoContext.color_range;
};

uint32_t PyFFmpegDemuxer::Height() const {
  MuxingParams params;
  upDemuxer->GetParams(params);
  return params.videoContext.height;
}

Pixel_Format PyFFmpegDemuxer::Format() const {
  MuxingParams params;
  upDemuxer->GetParams(params);
  return params.videoContext.format;
}

// cudaVideoCodec PyFFmpegDemuxer::Codec() const {
//   MuxingParams params;
//   upDemuxer->GetParams(params);
//   return params.videoContext.codec;
// }

double PyFFmpegDemuxer::Framerate() const {
  MuxingParams params;
  upDemuxer->GetParams(params);
  return params.videoContext.frameRate;
}

double PyFFmpegDemuxer::Timebase() const {
  MuxingParams params;
  upDemuxer->GetParams(params);
  return params.videoContext.timeBase;
}

uint32_t PyFFmpegDemuxer::Numframes() const {
  MuxingParams params;
  upDemuxer->GetParams(params);
  return params.videoContext.num_frames;
}
#ifdef GENERATE_PYTHON_BINDINGS
bool PyFFmpegDemuxer::Seek(SeekContext &ctx, py::array_t<uint8_t> &packet) {
#else
bool PyFFmpegDemuxer::Seek(SeekContext &ctx, std::vector<uint8_t> &packet) {
#endif
  Buffer *elementaryVideo = nullptr;
  auto pSeekCtxBuf = shared_ptr<Buffer>(Buffer::MakeOwnMem(sizeof(ctx), &ctx));
  do {
    upDemuxer->SetInput((Token *)pSeekCtxBuf.get(), 1U);
    if (TASK_EXEC_FAIL == upDemuxer->Execute()) {
      upDemuxer->ClearInputs();
      return false;
    }
    elementaryVideo = (Buffer *)upDemuxer->GetOutput(0U);
  } while (!elementaryVideo);

  packet.resize({elementaryVideo->GetRawMemSize()}, false);
#ifdef GENERATE_PYTHON_BINDINGS
  memcpy(packet.mutable_data(), elementaryVideo->GetDataAs<void>(),
         elementaryVideo->GetRawMemSize());
#else
  memcpy(packet.data(), elementaryVideo->GetDataAs<void>(),
         elementaryVideo->GetRawMemSize());
#endif

  auto pktDataBuf = (Buffer*)upDemuxer->GetOutput(3U);
  if (pktDataBuf) {
    auto pPktData = pktDataBuf->GetDataAs<PacketData>();
    ctx.out_frame_pts = pPktData->pts;
    ctx.out_frame_duration = pPktData->duration;
  }

  upDemuxer->ClearInputs();
  return true;
}

struct ParsePacketContext {
#ifdef GENERATE_PYTHON_BINDINGS
  py::array_t<uint8_t> *pPacket;
#else
  std::vector<uint8_t> *pPacket;
#endif
#ifdef GENERATE_PYTHON_BINDINGS
  ParsePacketContext(py::array_t<uint8_t> *packet) : pPacket(packet) {}
#else
  ParsePacketContext(std::vector<uint8_t> *packet) : pPacket(packet) {}
#endif
};

PyBitStreamParser::PyBitStreamParser(uint32_t codec) {
  upParsePacket.reset(ParsePacket::Make(codec));
}

#ifdef GENERATE_PYTHON_BINDINGS
bool PyBitStreamParser::ParseSinglePacket(py::array_t<uint8_t> &packet_in) {
#else
bool PyBitStreamParser::ParseSinglePacket(std::vector<uint8_t> &packet_in) {
#endif
  unique_ptr<Buffer> elementaryVideo = nullptr;
  ParsePacketContext ctx(&packet_in);
  if (ctx.pPacket && ctx.pPacket->size()) {
    elementaryVideo = unique_ptr<Buffer>(
        Buffer::Make(ctx.pPacket->size(), (void *)ctx.pPacket->data()));
  }
  upParsePacket->SetInput(elementaryVideo ? elementaryVideo.get() : nullptr, 0U);
  if (TASK_EXEC_FAIL == upParsePacket->Execute()) {
    return false;
  }
  return true;
}


uint32_t PyBitStreamParser::Width() const {
  MuxingParams params;
  upParsePacket->GetParams(params);
  return params.videoContext.width;
}

uint32_t PyBitStreamParser::Height() const {
  MuxingParams params;
  upParsePacket->GetParams(params);
  return params.videoContext.height;
}

Pixel_Format PyBitStreamParser::Format() const {
  MuxingParams params;
  upParsePacket->GetParams(params);
  return params.videoContext.format;
}

AVCodecID PyBitStreamParser::Codec() const {
  MuxingParams params;
  upParsePacket->GetParams(params);
  return params.videoContext.codec;
}

#ifdef GENERATE_PYTHON_BINDINGS
PYBIND11_MODULE(PyNvCodec, m) {
  m.doc() = "Python bindings for Nvidia-accelerated video processing";

  PYBIND11_NUMPY_DTYPE_EX(MotionVector, source, "source", w, "w", h, "h", src_x,
                          "src_x", src_y, "src_y", dst_x, "dst_x", dst_y,
                          "dst_y", motion_x, "motion_x", motion_y, "motion_y",
                          motion_scale, "motion_scale");

  py::class_<MotionVector>(m, "MotionVector");
  


  py::enum_<Pixel_Format>(m, "PixelFormat")
      .value("Y", Pixel_Format::Y)
      .value("RGB", Pixel_Format::RGB)
      .value("NV12", Pixel_Format::NV12)
      .value("YUV420", Pixel_Format::YUV420)
      .value("RGB_PLANAR", Pixel_Format::RGB_PLANAR)
      .value("BGR", Pixel_Format::BGR)
      .value("YCBCR", Pixel_Format::YCBCR)
      .value("YUV444", Pixel_Format::YUV444)
      .value("UNDEFINED", Pixel_Format::UNDEFINED)
      .value("RGB_32F", Pixel_Format::RGB_32F)
      .value("RGB_32F_PLANAR", Pixel_Format::RGB_32F_PLANAR)
      .value("RGB_32F_PLANAR_CONTIGUOUS", Pixel_Format::RGB_32F_PLANAR_CONTIGUOUS)
      .export_values();

    py::enum_<ColorSpace>(m, "ColorSpace")
      .value("BT_601", ColorSpace::BT_601)
      .value("BT_709", ColorSpace::BT_709)
      .value("UNSPEC", ColorSpace::UNSPEC)
      .export_values();

    py::enum_<ColorRange>(m, "ColorRange")
        .value("MPEG", ColorRange::MPEG)
        .value("JPEG", ColorRange::JPEG)
        .value("UDEF", ColorRange::UDEF)
        .export_values();


  py::enum_<SeekMode>(m, "SeekMode")
      .value("EXACT_FRAME", SeekMode::EXACT_FRAME)
      .value("PREV_KEY_FRAME", SeekMode::PREV_KEY_FRAME)
      .export_values();

  py::class_<SeekContext, shared_ptr<SeekContext>>(m, "SeekContext")
      .def(py::init<int64_t>(), py::arg("seek_frame"))
      .def(py::init<int64_t, SeekMode>(), py::arg("seek_frame"), py::arg("mode"))
      .def_readwrite("seek_frame", &SeekContext::seek_frame)
      .def_readwrite("mode", &SeekContext::mode)
      .def_readwrite("out_frame_pts", &SeekContext::out_frame_pts)
      .def_readonly("num_frames_decoded", &SeekContext::num_frames_decoded);

  py::class_<PacketData, shared_ptr<PacketData>>(m, "PacketData")
      .def(py::init<>())
      .def_readwrite("pts", &PacketData::pts)
      .def_readwrite("dts", &PacketData::dts)
      .def_readwrite("pos", &PacketData::pos)
      .def_readwrite("poc", &PacketData::poc)
      .def_readwrite("duration", &PacketData::duration);


  py::class_<PyFfmpegDecoder>(m, "PyFfmpegDecoder")
      .def(py::init<const string &, const map<string, string> &>())
      .def("DecodeSingleFrame", &PyFfmpegDecoder::DecodeSingleFrame)
      .def("GetMotionVectors", &PyFfmpegDecoder::GetMotionVectors,
           py::return_value_policy::move);

  py::class_<PyFFmpegDemuxer>(m, "PyFFmpegDemuxer")
      .def(py::init<const string &>())
      .def(py::init<const string &, const map<string, string> &>())
      .def("DemuxSinglePacket", &PyFFmpegDemuxer::DemuxSinglePacket)
      .def("Width", &PyFFmpegDemuxer::Width)
      .def("Height", &PyFFmpegDemuxer::Height)
      .def("Format", &PyFFmpegDemuxer::Format)
      .def("Framerate", &PyFFmpegDemuxer::Framerate)
      .def("Timebase", &PyFFmpegDemuxer::Timebase)
      .def("Numframes", &PyFFmpegDemuxer::Numframes)
      .def("Codec", &PyFFmpegDemuxer::Codec)
      .def("LastPacketData", &PyFFmpegDemuxer::GetLastPacketData)
      .def("Seek", &PyFFmpegDemuxer::Seek)
      .def("ColorSpace", &PyFFmpegDemuxer::GetColorSpace)
      .def("ColorRange", &PyFFmpegDemuxer::GetColorRange);


  py::class_<PyBitStreamParser>(m, "PyBitStreamParser")
      .def(py::init<uint32_t>())
      .def("Width", &PyBitStreamParser::Width)
      .def("Height", &PyBitStreamParser::Height)
      .def("Format", &PyBitStreamParser::Format)
      .def("Codec", &PyBitStreamParser::Codec)
      .def("ParseSinglePacket", py::overload_cast<py::array_t<uint8_t> &>(
            &PyBitStreamParser::ParseSinglePacket),
            py::arg("packet_in"),
            py::call_guard<py::gil_scoped_release>());

}
#endif
