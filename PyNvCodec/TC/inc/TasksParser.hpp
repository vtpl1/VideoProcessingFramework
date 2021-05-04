#pragma once
#ifndef TASKS_PARSER_H
#define TASKS_PARSER_H

#include "CodecsSupport.hpp"
#include "TC_CORE.hpp"

using namespace VPF;

// VPF stands for Video Processing Framework;
namespace VPF {
class DllExport ParsePacket final : public Task {
 public:
  ParsePacket() = delete;
  ParsePacket(const ParsePacket &other) = delete;
  ParsePacket &operator=(const ParsePacket &other) = delete;
  void GetParams(struct MuxingParams &params) const;
  TaskExecStatus Execute() final;
  ~ParsePacket() final;
  static ParsePacket *Make(uint32_t codec);

 private:
  ParsePacket(uint32_t codec);
  static const uint32_t numInputs = 1U;
  static const uint32_t numOutputs = 0U;
  struct ParsePacket_Impl *pImpl = nullptr;
};
};      // namespace VPF
#endif  // TASKS_PARSER_H