/*
 * Copyright 2019 NVIDIA Corporation
 * Copyright 2021 Videonetics Technology Private Limited
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

#include <chrono>
#include <fstream>
#include <map>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "CodecsSupport.hpp"
#include "MemoryInterfaces.hpp"
#include "Tasks.hpp"
#include "FFmpegDemuxer.h"

extern "C" {
#include <libavutil/pixdesc.h>
}

using namespace VPF;
using namespace std;
using namespace chrono;

constexpr auto TASK_EXEC_SUCCESS = TaskExecStatus::TASK_EXEC_SUCCESS;
constexpr auto TASK_EXEC_FAIL = TaskExecStatus::TASK_EXEC_FAIL;


namespace VPF {
struct DemuxFrame_Impl {
  size_t videoBytes = 0U;
  FFmpegDemuxer demuxer;
  Buffer *pElementaryVideo;
  Buffer *pMuxingParams;
  Buffer *pSei;
  Buffer *pPktData;

  DemuxFrame_Impl() = delete;
  DemuxFrame_Impl(const DemuxFrame_Impl &other) = delete;
  DemuxFrame_Impl &operator=(const DemuxFrame_Impl &other) = delete;

  explicit DemuxFrame_Impl(const string &url,
                           const map<string, string> &ffmpeg_options)
      : demuxer(url.c_str(), ffmpeg_options) {
    pElementaryVideo = Buffer::MakeOwnMem(0U);
    pMuxingParams = Buffer::MakeOwnMem(sizeof(MuxingParams));
    pSei = Buffer::MakeOwnMem(0U);
    pPktData = Buffer::MakeOwnMem(0U);
  }

  ~DemuxFrame_Impl() {
    delete pElementaryVideo;
    delete pMuxingParams;
    delete pSei;
    delete pPktData;
  }
};
} // namespace VPF

DemuxFrame *DemuxFrame::Make(const char *url, const char **ffmpeg_options,
                             uint32_t opts_size) {
  return new DemuxFrame(url, ffmpeg_options, opts_size);
}

DemuxFrame::DemuxFrame(const char *url, const char **ffmpeg_options,
                       uint32_t opts_size)
    : Task("DemuxFrame", DemuxFrame::numInputs, DemuxFrame::numOutputs) {
  map<string, string> options;
  if (0 == opts_size % 2) {
    for (auto i = 0; i < opts_size;) {
      auto key = string(ffmpeg_options[i]);
      i++;
      auto value = string(ffmpeg_options[i]);
      i++;

      options.insert(pair<string, string>(key, value));
    }
  }
  pImpl = new DemuxFrame_Impl(url, options);
}

DemuxFrame::~DemuxFrame() { delete pImpl; }

void DemuxFrame::Flush() { pImpl->demuxer.Flush(); }

TaskExecStatus DemuxFrame::Run() {
  ClearOutputs();

  uint8_t *pVideo = nullptr;
  MuxingParams params = {0};
  PacketData pkt_data = {0};

  auto &videoBytes = pImpl->videoBytes;
  auto &demuxer = pImpl->demuxer;

  uint8_t *pSEI = nullptr;
  size_t seiBytes = 0U;
  bool needSEI = (nullptr != GetInput(0U));

  auto pSeekCtxBuf = (Buffer *)GetInput(1U);
  if (pSeekCtxBuf) {
    SeekContext seek_ctx = *pSeekCtxBuf->GetDataAs<SeekContext>();
    auto ret = demuxer.Seek(seek_ctx, pVideo, videoBytes, pkt_data,
                            needSEI ? &pSEI : nullptr, &seiBytes);
    if (!ret) {
      return TASK_EXEC_FAIL;
    }
  } else if (!demuxer.Demux(pVideo, videoBytes, pkt_data,
                            needSEI ? &pSEI : nullptr, &seiBytes)) {
    return TASK_EXEC_FAIL;
  }

  if (videoBytes) {
    pImpl->pElementaryVideo->Update(videoBytes, pVideo);
    SetOutput(pImpl->pElementaryVideo, 0U);

    GetParams(params);
    pImpl->pMuxingParams->Update(sizeof(MuxingParams), &params);
    SetOutput(pImpl->pMuxingParams, 1U);
  }

  if (pSEI) {
    pImpl->pSei->Update(seiBytes, pSEI);
    SetOutput(pImpl->pSei, 2U);
  }

  pImpl->pPktData->Update(sizeof(pkt_data), &pkt_data);
  SetOutput((Token*)pImpl->pPktData, 3U);

  return TASK_EXEC_SUCCESS;
}

void DemuxFrame::GetParams(MuxingParams &params) const {
  params.videoContext.width = pImpl->demuxer.GetWidth();
  params.videoContext.height = pImpl->demuxer.GetHeight();
  params.videoContext.num_frames = pImpl->demuxer.GetNumFrames();
  params.videoContext.frameRate = pImpl->demuxer.GetFramerate();
  params.videoContext.timeBase = pImpl->demuxer.GetTimebase();
  params.videoContext.streamIndex = pImpl->demuxer.GetVideoStreamIndex();
  params.videoContext.codec = pImpl->demuxer.GetVideoCodec();
  params.videoContext.gop_size = pImpl->demuxer.GetGopSize();

  switch (pImpl->demuxer.GetPixelFormat()) {
  case AV_PIX_FMT_YUVJ420P:
  case AV_PIX_FMT_YUV420P:
  case AV_PIX_FMT_NV12:
    params.videoContext.format = NV12;
    break;
  case AV_PIX_FMT_YUV444P:
    params.videoContext.format = YUV444;
    break;
  default:
    stringstream ss;
    ss << "Unsupported FFmpeg pixel format: "
       << av_get_pix_fmt_name(pImpl->demuxer.GetPixelFormat()) << endl;
    throw invalid_argument(ss.str());
    params.videoContext.format = UNDEFINED;
    break;
  }

  switch (pImpl->demuxer.GetColorSpace()) {
  case AVCOL_SPC_BT709:
    params.videoContext.color_space = BT_709;
    break;
  case AVCOL_SPC_BT470BG:
  case AVCOL_SPC_SMPTE170M:
    params.videoContext.color_space = BT_601;
    break;
  default:
    params.videoContext.color_space = UNSPEC;
    break;
  }

  switch (pImpl->demuxer.GetColorRange()) {
  case AVCOL_RANGE_MPEG:
    params.videoContext.color_range = MPEG;
    break;
  case AVCOL_RANGE_JPEG:
    params.videoContext.color_range = JPEG;
    break;
  default:
    params.videoContext.color_range = UDEF;
    break;
  }
}

