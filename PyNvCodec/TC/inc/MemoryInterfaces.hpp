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

#include <cstddef>
#include "TC_CORE.hpp"

using namespace VPF;

namespace VPF {

enum Pixel_Format {
  UNDEFINED = 0,
  Y = 1,
  RGB = 2,
  NV12 = 3,
  YUV420 = 4,
  RGB_PLANAR = 5,
  BGR = 6,
  YCBCR = 7,
  YUV444 = 8,
  RGB_32F = 9,
  RGB_32F_PLANAR = 10,
  RGB_32F_PLANAR_CONTIGUOUS = 11,
};

enum ColorSpace {
  BT_601 = 0,
  BT_709 = 1,
  UNSPEC = 2,
};

enum ColorRange {
  MPEG = 0, /* Narrow range.*/
  JPEG = 1, /* Full range. */
  UDEF = 2,
};

struct ColorspaceConversionContext {
  ColorSpace color_space;
  ColorRange color_range;

  ColorspaceConversionContext() : color_space(UNSPEC), color_range(UDEF) {}

  ColorspaceConversionContext(ColorSpace cspace, ColorRange crange)
      : color_space(cspace), color_range(crange) {}
};

/* Represents CPU-side memory.
 * May own the memory or be a wrapper around existing ponter;
 */
class DllExport Buffer final : public Token {
public:
  Buffer() = delete;
  Buffer(const Buffer &other) = delete;
  Buffer &operator=(Buffer &other) = delete;

  ~Buffer() final;
  void *GetRawMemPtr();
  const void *GetRawMemPtr() const;
  size_t GetRawMemSize() const;
  void Update(size_t newSize, void *newPtr = nullptr);
  bool CopyFrom(size_t size, void const *ptr);
  template <typename T> T *GetDataAs() { return (T *)GetRawMemPtr(); }
  template <typename T> T const *GetDataAs() const {
    return (T const *)GetRawMemPtr();
  }

  static Buffer *Make(size_t bufferSize);
  static Buffer *Make(size_t bufferSize, void *pCopyFrom);

  static Buffer *MakeOwnMem(size_t bufferSize);
  static Buffer *MakeOwnMem(size_t bufferSize, const void *pCopyFrom);

private:
  explicit Buffer(size_t bufferSize, bool ownMemory = true);
  Buffer(size_t bufferSize, void *pCopyFrom, bool ownMemory);
  Buffer(size_t bufferSize, const void *pCopyFrom);
  bool Allocate();
  void Deallocate();

  bool own_memory = false;
  size_t mem_size = 0UL;
  void *pRawData = nullptr;
#ifdef TRACK_TOKEN_ALLOCATIONS
  uint32_t id;
#endif
};


#ifdef TRACK_TOKEN_ALLOCATIONS
/* Returns true if allocation counters are equal to zero, false otherwise;
 * If you want to check for dangling pointers, call this function at exit;
 */
bool DllExport CheckAllocationCounters();
#endif


} // namespace VPF
