/*
 * Copyright 2020 NVIDIA Corporation
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

#pragma once

#include "MemoryInterfaces.hpp"
//#include "NvCodecCLIOptions.h"
#include "FFmpegDemuxer.h"

#include "TC_CORE.hpp"
#include "Tasks.hpp"
#include "TasksParser.hpp"

#include <chrono>
#include <mutex>
#ifdef GENERATE_PYTHON_BINDINGS
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#else
#include <memory>
#include <iostream>
#include <vector>
#endif
#include <sstream>

extern "C" {
#include <libavutil/frame.h>
#include <libavutil/motion_vector.h>
}

using namespace VPF;
#ifdef GENERATE_PYTHON_BINDINGS
namespace py = pybind11;
#endif

struct MotionVector {
  int source;
  int w, h;
  int src_x, src_y;
  int dst_x, dst_y;
  int motion_x, motion_y;
  int motion_scale;
};

class DllExport PyFFmpegDemuxer {
  std::unique_ptr<DemuxFrame> upDemuxer;

public:
  PyFFmpegDemuxer(const std::string &pathToFile);
  PyFFmpegDemuxer(const std::string &pathToFile,
                  const std::map<std::string, std::string> &ffmpeg_options);

#ifdef GENERATE_PYTHON_BINDINGS
  bool DemuxSinglePacket(py::array_t<uint8_t> &packet);
#else
  bool DemuxSinglePacket(std::vector<uint8_t> &packet);
#endif

  void GetLastPacketData(PacketData &pkt_data);

#ifdef GENERATE_PYTHON_BINDINGS
  bool Seek(SeekContext &ctx, py::array_t<uint8_t> &packet);
#else
  bool Seek(SeekContext &ctx, std::vector<uint8_t> &packet);
#endif

  uint32_t Width() const;

  uint32_t Height() const;

  Pixel_Format Format() const;

  ColorSpace GetColorSpace() const;

  ColorRange GetColorRange() const;

  AVCodecID Codec() const;

  double Framerate() const;

  uint32_t Numframes() const;

  double Timebase() const;

};

class DllExport PyFfmpegDecoder {
  std::unique_ptr<FfmpegDecodeFrame> upDecoder = nullptr;

  void *GetSideData(AVFrameSideDataType data_type, size_t &raw_size);

public:
  PyFfmpegDecoder(const std::string &pathToFile,
                  const std::map<std::string, std::string> &ffmpeg_options);

#ifdef GENERATE_PYTHON_BINDINGS
  bool DecodeSingleFrame(py::array_t<uint8_t> &frame);

  py::array_t<MotionVector> GetMotionVectors();
#else
  bool DecodeSingleFrame(std::vector<uint8_t> &frame);

  std::vector<MotionVector> GetMotionVectors();
#endif
};

class DllExport PyBitStreamParser {
  std::unique_ptr<ParsePacket> upParsePacket;

 public:
  PyBitStreamParser(uint32_t codec);
#ifdef GENERATE_PYTHON_BINDINGS
  bool ParseSinglePacket(py::array_t<uint8_t> &packet_in);
  // bool ParseSinglePacket(py::array_t<uint8_t> &packet_in,
  //                        py::array_t<uint8_t> &packet_out,
  //                        py::array_t<uint8_t> &sei);
#else
  bool ParseSinglePacket(std::vector<uint8_t> &packet_in);

  // bool ParseSinglePacket(std::vector<uint8_t> &packet_in,
  //                        std::vector<uint8_t> &packet_out,
  //                        std::vector<uint8_t> &sei);
#endif
  uint32_t Width() const;

  uint32_t Height() const;

  Pixel_Format Format() const;

  AVCodecID Codec() const;
};
