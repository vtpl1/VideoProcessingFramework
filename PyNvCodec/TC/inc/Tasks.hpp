/*
 * Copyright 2019 NVIDIA Corporation
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
#include "CodecsSupport.hpp"
#include "MemoryInterfaces.hpp"
#include "TC_CORE.hpp"

extern "C" {
#include <libavutil/frame.h>
}

using namespace VPF;

// VPF stands for Video Processing Framework;
namespace VPF {

class DllExport FfmpegDecodeFrame final : public Task {
 public:
  FfmpegDecodeFrame() = delete;
  FfmpegDecodeFrame(const FfmpegDecodeFrame &other) = delete;
  FfmpegDecodeFrame &operator=(const FfmpegDecodeFrame &other) = delete;

  TaskExecStatus Run() final;
  TaskExecStatus GetSideData(AVFrameSideDataType);

  ~FfmpegDecodeFrame() final;
  static FfmpegDecodeFrame *Make(const char *URL);

 private:
  static const uint32_t num_inputs = 0U;
  // Reconstructed pixels + side data;
  static const uint32_t num_outputs = 2U;
  struct FfmpegDecodeFrame_Impl *pImpl = nullptr;

  FfmpegDecodeFrame(const char *URL);
};

class DllExport DemuxFrame final : public Task {
 public:
  DemuxFrame() = delete;
  DemuxFrame(const DemuxFrame &other) = delete;
  DemuxFrame &operator=(const DemuxFrame &other) = delete;

  void GetParams(struct MuxingParams &params) const;
  void Flush();
  TaskExecStatus Run() final;
  ~DemuxFrame() final;
  static DemuxFrame *Make(const char *url, const char **ffmpeg_options,
                          uint32_t opts_size);

 private:
  DemuxFrame(const char *url, const char **ffmpeg_options, uint32_t opts_size);
  static const uint32_t numInputs = 2U;
  static const uint32_t numOutputs = 4U;
  struct DemuxFrame_Impl *pImpl = nullptr;
};

}  // namespace VPF