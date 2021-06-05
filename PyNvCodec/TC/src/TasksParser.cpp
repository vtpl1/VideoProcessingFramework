#include "TasksParser.hpp"

#include <memory>
#include <vector>

#include "H264BitStreamParser.hpp"
#include <stdexcept>
using namespace VPF;

#ifdef GENERATE_PYTHON_BINDINGS
namespace py = pybind11;
#endif

constexpr auto TASK_EXEC_SUCCESS = TaskExecStatus::TASK_EXEC_SUCCESS;
constexpr auto TASK_EXEC_FAIL = TaskExecStatus::TASK_EXEC_FAIL;


namespace VPF {
struct ParsePacket_Impl {
  std::unique_ptr<H264BitStreamParser> parser;
  // Buffer *pElementaryVideo;
  // Buffer *pMuxingParams;
  // Buffer *pSei;
  // ParsePacket_Impl() = delete;
  ParsePacket_Impl(const ParsePacket_Impl &other) = delete;
  ParsePacket_Impl &operator=(const ParsePacket_Impl &other) = delete;

  explicit ParsePacket_Impl() {
    parser.reset(new H264BitStreamParser());
    // pElementaryVideo = Buffer::MakeOwnMem(0U);
    // pMuxingParams = Buffer::MakeOwnMem(sizeof(MuxingParams));
    // pSei = Buffer::MakeOwnMem(0U);
    // pContext = Buffer::MakeOwnMem(0U);
  }

  ~ParsePacket_Impl() {
    // delete pElementaryVideo;
    // delete pMuxingParams;
    // delete pSei;
    // delete pContext;
  }
};
}  // namespace VPF

ParsePacket *ParsePacket::Make(uint32_t codec) {
  switch (codec) {
    case 2:
      return new ParsePacket(codec);
    default:
      throw std::runtime_error("Unsupported codec type");
  }
}

ParsePacket::ParsePacket(uint32_t codec)
    : Task("ParsePacket", ParsePacket::numInputs, ParsePacket::numOutputs) {
  pImpl = new ParsePacket_Impl();
}

void ParsePacket::GetParams(MuxingParams &params) const {
  params.videoContext.width = pImpl->parser->GetWidth();
  params.videoContext.height = pImpl->parser->GetHeight();
  params.videoContext.frameRate = 25;
  params.videoContext.timeBase = 1000;
  params.videoContext.streamIndex = 0;
  params.videoContext.codec = AV_CODEC_ID_H264;
  params.videoContext.format = NV12;
}

TaskExecStatus ParsePacket::Execute() {
  ClearOutputs();
  auto elementaryVideo = (Buffer *)GetInput(0U);
  size_t videoBytes = elementaryVideo->GetRawMemSize();
  MuxingParams params = {0};
  PacketData pkt_data = {0};
  auto &parser = *pImpl->parser;

  uint8_t *pSEI = nullptr;
  size_t seiBytes = 0U;

  if (!parser.ParseNAL(elementaryVideo->GetDataAs<uint8_t>(),
                       (unsigned short)videoBytes)) {
    return TASK_EXEC_FAIL;
  }
  // if (elementaryVideo) {
  //   pImpl->pElementaryVideo->Update(videoBytes, elementaryVideo);
  //   SetOutput(pImpl->pElementaryVideo, 0U);
  // }
  // if (pSEI) {
  //   pImpl->pSei->Update(seiBytes, pSEI);
  //   SetOutput(pImpl->pSei, 1U);
  // }
  return TASK_EXEC_SUCCESS;
}

ParsePacket::~ParsePacket() { delete pImpl; }

