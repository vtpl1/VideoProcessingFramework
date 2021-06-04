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

#include "MemoryInterfaces.hpp"
#include <cstring>
#include <new>
#include <sstream>
#include <stdexcept>

using namespace VPF;
using namespace VPF;
using namespace std;

#ifdef TRACK_TOKEN_ALLOCATIONS
#include <algorithm>
#include <atomic>
#include <iostream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace VPF {

struct AllocInfo {
  uint64_t id;
  uint64_t size;

  bool operator==(const AllocInfo &other) {
    /* Buffer size may change during the lifetime so we check id only;
     */
    return id == other.id;
  }

  explicit AllocInfo(decltype(id) const &newId, decltype(size) const &newSize)
      : id(newId), size(newSize) {}
};

struct AllocRegister {
  vector<AllocInfo> instances;
  mutex guard;
  uint64_t ID = 1U;

  decltype(AllocInfo::id) AddNote(decltype(AllocInfo::size) const &size) {
    unique_lock<decltype(guard)> lock;
    auto id = ID++;
    AllocInfo info(id, size);
    instances.push_back(info);
    return id;
  }

  void DeleteNote(AllocInfo const &allocInfo) {
    unique_lock<decltype(guard)> lock;
    instances.erase(remove(instances.begin(), instances.end(), allocInfo),
                    instances.end());
  }

  /* Call this after you're done releasing mem objects in your app;
   */
  size_t GetSize() const { return instances.size(); }

  /* Call this after you're done releasing mem objects in your app;
   */
  AllocInfo const *GetNoteByIndex(uint64_t idx) {
    return idx < instances.size() ? instances.data() + idx : nullptr;
  }
};

AllocRegister BuffersRegister, HWSurfaceRegister;

bool CheckAllocationCounters() {
  auto numLeakedBuffers = BuffersRegister.GetSize();
  auto numLeakedSurfaces = HWSurfaceRegister.GetSize();

  if (numLeakedBuffers) {
    cerr << "Leaked buffers (id : size): " << endl;
    for (auto i = 0; i < numLeakedBuffers; i++) {
      auto pNote = BuffersRegister.GetNoteByIndex(i);
      cerr << "\t" << pNote->id << "\t: " << pNote->size << endl;
    }
  }

  if (numLeakedSurfaces) {
    cerr << "Leaked surfaces (id : size): " << endl;
    for (auto i = 0; i < numLeakedSurfaces; i++) {
      auto pNote = HWSurfaceRegister.GetNoteByIndex(i);
      cerr << "\t" << pNote->id << "\t: " << pNote->size << endl;
    }
  }

  return (0U == numLeakedBuffers) && (0U == numLeakedSurfaces);
}

} // namespace VPF
#endif

Buffer *Buffer::Make(size_t bufferSize) {
  return new Buffer(bufferSize, false);
}

Buffer *Buffer::Make(size_t bufferSize, void *pCopyFrom) {
  return new Buffer(bufferSize, pCopyFrom, false);
}

Buffer::Buffer(size_t bufferSize, bool ownMemory)
    : mem_size(bufferSize), own_memory(ownMemory) {
  if (own_memory) {
    if (!Allocate()) {
      throw bad_alloc();
    }
  }
#ifdef TRACK_TOKEN_ALLOCATIONS
  id = BuffersRegister.AddNote(mem_size);
#endif
}

Buffer::Buffer(size_t bufferSize, void *pCopyFrom, bool ownMemory)
    : mem_size(bufferSize), own_memory(ownMemory) {
  if (own_memory) {
    if (Allocate()) {
      memcpy(this->GetRawMemPtr(), pCopyFrom, bufferSize);
    } else {
      throw bad_alloc();
    }
  } else {
    pRawData = pCopyFrom;
  }
#ifdef TRACK_TOKEN_ALLOCATIONS
  id = BuffersRegister.AddNote(mem_size);
#endif
}

Buffer::Buffer(size_t bufferSize, const void *pCopyFrom)
    : mem_size(bufferSize), own_memory(true) {
  if (Allocate()) {
    memcpy(this->GetRawMemPtr(), pCopyFrom, bufferSize);
  } else {
    throw bad_alloc();
  }
#ifdef TRACK_TOKEN_ALLOCATIONS
  id = BuffersRegister.AddNote(mem_size);
#endif
}

Buffer::~Buffer() {
  Deallocate();
#ifdef TRACK_TOKEN_ALLOCATIONS
  AllocInfo info(id, mem_size);
  BuffersRegister.DeleteNote(info);
#endif
}

size_t Buffer::GetRawMemSize() const { return mem_size; }


bool Buffer::Allocate() {
  if (GetRawMemSize()) {
      pRawData = calloc(GetRawMemSize(), sizeof(uint8_t));
    return (nullptr != pRawData);
  }
  return true;
}

void Buffer::Deallocate() {
  if (own_memory) {
      free(pRawData);
  }
  pRawData = nullptr;
}

void *Buffer::GetRawMemPtr() { return pRawData; }

const void *Buffer::GetRawMemPtr() const { return pRawData; }

void Buffer::Update(size_t newSize, void *newPtr) {
  Deallocate();

  mem_size = newSize;
  if (own_memory) {
    Allocate();
    if (newPtr) {
      memcpy(GetRawMemPtr(), newPtr, newSize);
    }
  } else {
    pRawData = newPtr;
  }
}

Buffer *Buffer::MakeOwnMem(size_t bufferSize) {
  return new Buffer(bufferSize, true);
}

bool Buffer::CopyFrom(size_t size, void const *ptr) {

  if (mem_size != size) {
    return false;
  }

  if (!ptr) {
    return false;
  }

  memcpy(GetRawMemPtr(), ptr, size);
  return true;
}

Buffer *Buffer::MakeOwnMem(size_t bufferSize, const void *pCopyFrom) {
  return new Buffer(bufferSize, pCopyFrom);
}


