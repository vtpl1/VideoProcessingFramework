#include <cuda.h>
#include <cuda_runtime.h>

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <csignal>
#include <cstdlib>
#include <iomanip>  // std::setprecision
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "AVFReader.h"
#include "PyNvCodec.hpp"
#include "utils.hpp"
// namespace

class FpsValue {
 public:
  FpsValue() : FpsValue(0.0, 0) {}
  FpsValue(float fps, int frame_id) : fps(fps), frame_id(frame_id) {}
  float fps = 0.0;
  int frame_id = -1;
};
class FpsStats {
 public:
  FpsStats() : FpsStats(0) {}
  FpsStats(int gpu_id) : gpu_id(gpu_id) {}
  FpsValue instant_fps;
  float total_fps = 0.0;
  int total_frame_count = 0;
  float total_time_diff = 0.0;
  FpsValue min_fps;
  FpsValue max_fps;
  int fps_counter = 0;
  int gpu_id = 0;
};

class StatusMonitor {
 private:
  std::mutex m;
  std::map<size_t, FpsStats> thread_to_fps;

 public:
  StatusMonitor(/* args */){};
  ~StatusMonitor(){};
  void put_fps(size_t thread_id, int frame_count, float time_diff, float fps,
               int gpu_id) {
    std::lock_guard<std::mutex> lk(m);
    if (thread_to_fps.find(thread_id) == thread_to_fps.end()) {
      thread_to_fps[thread_id] = FpsStats(gpu_id = gpu_id);
      thread_to_fps[thread_id].min_fps.fps = 10000;
    }
    auto &fps_stats = thread_to_fps[thread_id];
    auto &frame_id = fps_stats.fps_counter;
    auto fps_value = FpsValue(fps, frame_id);

    fps_stats.instant_fps = fps_value;
    fps_stats.total_fps += fps_value.fps;
    fps_stats.fps_counter += 1;
    fps_stats.total_frame_count += frame_count;
    fps_stats.total_time_diff += time_diff;
    if (fps_value.fps < fps_stats.min_fps.fps) {
      fps_stats.min_fps.fps = fps_value.fps;
      fps_stats.min_fps.frame_id = frame_id;
    }

    if (fps_value.fps > fps_stats.max_fps.fps) {
      fps_stats.max_fps.fps = fps_value.fps;
      fps_stats.max_fps.frame_id = frame_id;
    }
  }
  void print_fps() {
    std::stringstream ss;
    std::lock_guard<std::mutex> lk(m);
    for (auto &fps_stats : thread_to_fps) {
      ss << " [";
      ss << std::setprecision(0) << std::fixed
         << (fps_stats.second.total_fps / fps_stats.second.fps_counter);
      ss << " " << fps_stats.second.gpu_id << "] ";
    }
    std::cout << ss.str() << std::endl;
  }
};
namespace {
std::mutex m;
std::condition_variable cv;
volatile std::atomic<bool> shutdown_requested{false};
StatusMonitor status_monitor;
}  // namespace
void signal_handler(int signal) {
  {
    std::lock_guard<std::mutex> lk(m);
    shutdown_requested = true;
  }
  cv.notify_all();
}
static auto ThrowOnCudaErrorMain = [](CUresult res, int lineNum = -1) {
  if (CUDA_SUCCESS != res) {
    std::stringstream ss;

    if (lineNum > 0) {
      ss << __FILE__ << ":";
      ss << lineNum << std::endl;
    }

    const char *errName = nullptr;
    if (CUDA_SUCCESS != cuGetErrorName(res, &errName)) {
      ss << "CUDA error with code " << res << std::endl;
    } else {
      ss << "CUDA error: " << errName << std::endl;
    }

    const char *errDesc = nullptr;
    if (CUDA_SUCCESS != cuGetErrorString(res, &errDesc)) {
      // Try CUDA runtime function then;
      errDesc = cudaGetErrorString((cudaError_t)res);
    }

    if (!errDesc) {
      ss << "No error string available" << std::endl;
    } else {
      ss << errDesc << std::endl;
    }

    throw std::runtime_error(ss.str());
  }
};

/*
 * Case Insensitive Implementation of endsWith()
 * It checks if the string 'mainStr' ends with given string 'toMatch'
 */
bool endsWithCaseInsensitive(std::string mainStr, std::string toMatch) {
  auto it = toMatch.begin();
  return mainStr.size() >= toMatch.size() &&
         std::all_of(std::next(mainStr.begin(), mainStr.size() - toMatch.size()),
                     mainStr.end(), [&it](const char &c) {
                       return ::tolower(c) == ::tolower(*(it++));
                     });
}

class Worker {
  static std::mutex global_lock;

 private:
  size_t this_id = -1;
  int gpu_id;
  int channel_id;
  const std::string enc_file;
  int to_width;
  int to_height;
  float fps;
  int frame_count = 0;
  int consecutive_empty_surface_count = 0;
  std::atomic<bool> shutdown_requested{false};
  std::atomic<bool> done{false};  // Use an atomic flag.
  std::thread th;
  int64_t last_frame_time = -1;
  std::unique_ptr<AVFReader> reader;
  std::unique_ptr<PyBitStreamParser> parser;
  std::unique_ptr<PyNvDecoder> nv_dec;
  std::unique_ptr<PySurfacePreprocessor> from_nv12_to_rgb_32f_planar;
  int bufferLen = 2 * 1024 * 1024;
  uint8_t *buffer;

 public:
  Worker(int gpu_id, int channel_id, const std::string enc_file, int to_width,
         int to_height, int fps = 25)
      : gpu_id(gpu_id),
        channel_id(channel_id),
        enc_file(enc_file),
        to_width(to_width),
        fps(fps),
        bufferLen(2 * 1024 * 1024) {
    if (!endsWithCaseInsensitive(enc_file, "AVF"))
      throw std::runtime_error("File type not supported");
    buffer = new uint8_t[bufferLen];
    // buffer.resize(bufferLen);
  };
  bool create_decoder() {
    std::lock_guard<std::mutex> lock(Worker::global_lock);
    reader.reset(new AVFReader(enc_file));
    while (true) {
      FrameInfo frame;
      frame.totalSize = bufferLen;
      int rs = reader->getFrame(frame, buffer);
      if (rs == 2) {
        // returns error 2 or 9, buffer resize is not handled
        std::cerr << "End of file " << std::endl;
        break;
      }
      if (rs == 9) {
        // returns error 2 or 9, buffer resize is not handled
        std::cerr << "Low buffer size " << std::endl;
        break;
      }

      if (frame.mediaType != 2) {
        continue;
      }
      if (!parser) {
        try {
          parser.reset(new PyBitStreamParser(frame.mediaType));
        } catch (const std::runtime_error &e) {
          std::cerr << "MONOTOSH: dhorechi:" << e.what() << '\n';
        }
      }
      if (!parser) break;
      std::vector<uint8_t> packet_in(buffer, buffer + frame.frameSize);
      if (!parser->ParseSinglePacket(packet_in)) break;
      if (!((parser->Width() > 0) && (parser->Height() > 0))) {
        std::cerr << "Width or Height is zero " << std::endl;
        continue;
      }
      if (!nv_dec) {
        try {
          nv_dec.reset(new PyNvDecoder(
              parser->Width(), parser->Height(), Pixel_Format::NV12,
              cudaVideoCodec::cudaVideoCodec_H264, gpu_id));
        } catch (const std::runtime_error &e) {
          std::cerr << "MONOTOSH: dhorechi:" << e.what() << '\n';
        }
      }
      if (!nv_dec) break;
      std::shared_ptr<VPF::Surface> nv12_surface;

      try {
        nv12_surface = nv_dec->DecodeSurfaceFromPacket(packet_in);
      } catch (const HwResetException &e) {
        std::cerr << "MONOTOSH: dhorechi kintu egiy gechi:" << e.what() << '\n';
        break;
      } catch (const CuvidParserException &e) {
        std::cerr << "MONOTOSH: CuvidParserException dhorechi:" << e.what()
                  << '\n';
        break;
      } catch (const std::runtime_error &e) {
        std::cerr << "MONOTOSH: dhorechi:" << e.what() << '\n';
        break;
      }
      if (!nv12_surface) {
        std::cerr << "Null surface returned" << std::endl;
        consecutive_empty_surface_count++;
        continue;
      }
      if (nv12_surface->Empty()) {
        // std::cerr << "Empty surface returned" << std::endl;
        consecutive_empty_surface_count++;
        continue;
      }
      if (consecutive_empty_surface_count > 8) {
        break;
      }
      consecutive_empty_surface_count = 0;
      if (!from_nv12_to_rgb_32f_planar) {
        try {
          from_nv12_to_rgb_32f_planar.reset(new PySurfacePreprocessor(
              nv12_surface->Width(), nv12_surface->Height(),
              nv12_surface->PixelFormat(), 800, 800,
              Pixel_Format::RGB_32F_PLANAR, gpu_id));
        } catch (const std::runtime_error &e) {
          std::cerr << "MONOTOSH: dhorechi:" << e.what() << '\n';
        }
      }
      if (!from_nv12_to_rgb_32f_planar) break;
      auto rgb_32f_planar_surface =
          from_nv12_to_rgb_32f_planar->Execute(nv12_surface);
      if (!rgb_32f_planar_surface) break;
      if (rgb_32f_planar_surface->Empty()) break;
      return true;
    }

    return false;
  }
  void start() { th = std::thread(&Worker::run, this); }
  bool is_alive() { return !done; }
  bool decode_and_detect(size_t this_id) {
    auto t = std::chrono::duration_cast<std::chrono::seconds>(
                 std::chrono::system_clock::now().time_since_epoch())
                 .count();
    if (last_frame_time < 1) last_frame_time = t;
    size_t diff = t - last_frame_time;
    if (diff > 10) {
      fps = (float)frame_count / diff;
      last_frame_time = t;
      status_monitor.put_fps(this_id, frame_count, diff, fps, gpu_id);
      frame_count = 0;
    }
    FrameInfo frame;
    frame.totalSize = bufferLen;
    int rs = reader->getFrame(frame, buffer);
    if (rs == 2) {
      // returns error 2 or 9, buffer resize is not handled
      std::cerr << "End of file " << std::endl;
      return false;
    }
    if (rs == 9) {
      // returns error 2 or 9, buffer resize is not handled
      std::cerr << "Low buffer size " << std::endl;
      return false;
    }

    if (frame.mediaType != 2) {
      return true;
    }
    std::vector<uint8_t> packet_in(buffer, buffer + frame.frameSize);
    std::shared_ptr<VPF::Surface> nv12_surface;

    try {
      nv12_surface = nv_dec->DecodeSurfaceFromPacket(packet_in);
    } catch (const HwResetException &e) {
      std::cerr << "MONOTOSH: dhorechi kintu egiy gechi:" << e.what() << '\n';
      return false;
    } catch (const CuvidParserException &e) {
      std::cerr << "MONOTOSH: CuvidParserException dhorechi:" << e.what()
                << '\n';
      return false;
    } catch (const std::runtime_error &e) {
      std::cerr << "MONOTOSH: dhorechi:" << e.what() << '\n';
      return false;
    }
    if (!nv12_surface) {
      std::cerr << "Null surface returned" << std::endl;
      consecutive_empty_surface_count++;
      return true;
    }
    if (nv12_surface->Empty()) {
      // std::cerr << "Empty surface returned" << std::endl;
      consecutive_empty_surface_count++;
      return true;
    }
    if (consecutive_empty_surface_count > 8) {
      return false;
    }
    consecutive_empty_surface_count = 0;
    auto rgb_32f_planar_surface =
        from_nv12_to_rgb_32f_planar->Execute(nv12_surface);
    if (!rgb_32f_planar_surface) return false;
    if (rgb_32f_planar_surface->Empty()) return false;

    frame_count++;
    // std::cout << "frame_count: " << frame_count << std::endl;
    return true;
  }
  void run(void) {
    this_id = std::hash<std::thread::id>{}(std::this_thread::get_id());

    std::cout << "Started " << this_id << " from file: " << enc_file
              << " GPU ID: " << gpu_id << " CHANNEL ID: " << channel_id
              << std::endl;
    // if (!create_decoder()) {
    //   std::cout << "not creating decoder " << this_id << std::endl;
    //   shutdown_requested = true;
    // }
    while (shutdown_requested == false) {
      if (!decode_and_detect(this_id)) break;
    }
    done = true;
    std::cout << "End " << this_id << std::endl;
  }
  void stop(void) {
    shutdown_requested = true;
    std::cout << "Stop " << this_id << std::endl;
    th.join();
  }
  ~Worker() { delete[] buffer; };
};

std::mutex Worker::global_lock;
int main(int argc, char const *argv[]) {
  // Install a signal handler
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  std::cout << "VPF Becnchmark started" << std::endl;
  cuInit(0);
  int nGpu;
  ThrowOnCudaErrorMain(cuDeviceGetCount(&nGpu), __LINE__);
  int number_of_channels = 1;
  int gpu_id = -1;

  /*
  if (argc < 2) {
    std::cout << " number_of_channels "
              << " gpu_id " << std::endl;
    return -1;
  }
  std::string arg_number_of_channels = argv[1];
  try {
    std::size_t pos;
    number_of_channels = std::stoi(arg_number_of_channels, &pos);
    if (pos < arg_number_of_channels.size()) {
      std::cerr << "Trailing characters after number: " << arg_number_of_channels
                << '\n';
    }
  } catch (std::invalid_argument const &ex) {
    std::cerr << "Invalid number: " << arg_number_of_channels << '\n';
  } catch (std::out_of_range const &ex) {
    std::cerr << "Number out of range: " << arg_number_of_channels << '\n';
  }

  std::string arg_gpu_id = argv[2];
  try {
    std::size_t pos;
    gpu_id = std::stoi(arg_gpu_id, &pos);
    if (pos < arg_gpu_id.size()) {
      std::cerr << "Trailing characters after number: " << arg_gpu_id << '\n';
    }
  } catch (std::invalid_argument const &ex) {
    std::cerr << "Invalid number: " << arg_gpu_id << '\n';
  } catch (std::out_of_range const &ex) {
    std::cerr << "Number out of range: " << arg_gpu_id << '\n';
  }
  //*/
  int num_of_gpus = nGpu;
  if (gpu_id > nGpu) gpu_id = -1;
  if (gpu_id >= 0) num_of_gpus = 1;  // gpu specified

  std::vector<std::unique_ptr<Worker>> thread_list;
  std::cout << "Number of GPUS: " << nGpu
            << " number_of_channels: " << number_of_channels << " gpu_id "
            << gpu_id << " num_of_gpus: " << num_of_gpus << std::endl;
  std::stringstream ss;
  ss << ".";
  // ss << "/1.AVI";
  // ss << "/Merged_20200918_90003.mp4";
  ss << "/1.AVF";
  std::vector<size_t> free_mem_before;
  for (size_t indx = 0; indx < nGpu; indx++) {
    size_t free, total;
    cudaSetDevice(indx);
    int id;
    cudaGetDevice(&id);
    assert(id == indx);
    cudaMemGetInfo(&free, &total);
    free_mem_before.push_back(free);
    std::cout << "GPU " << id << " memory: free=" << free / (1024 * 1024)
              << " MB, total=" << total / (1024 * 1024) << " MB" << std::endl;
  }
  // ss << "/workspaces/VideoProcessingFramework/Merged_20200918_90003.mp4";
  for (size_t indx = 0; indx < num_of_gpus; indx++) {
    int i = indx;
    if ((num_of_gpus == 1) && (gpu_id >= 0)) i = gpu_id;
    for (size_t j = 0; j < number_of_channels; j++) {
      std::unique_ptr<Worker> p(new Worker(i, j, ss.str(), 800, 800));
      if (p->create_decoder()) {
        thread_list.push_back(std::move(p));
      } else {
        std::cout << "Not creating decoder" << std::endl;
      }
    }
  }
  for (auto &x : thread_list) {
    x->start();
  }
  while (true) {
    std::unique_lock<std::mutex> lk(m);
    cv.wait_for(lk, std::chrono::seconds(10));
    if (shutdown_requested) break;
    status_monitor.print_fps();
    int live_threads = 0;
    for (auto &x : thread_list) {
      if (x->is_alive()) live_threads += 1;
    }
    if (live_threads == 0) break;
  }
  std::cout << "Before calling stop" << std::endl;
  for (auto &x : thread_list) {
    x->stop();
  }
  thread_list.clear();
  for (size_t indx = 0; indx < nGpu; indx++) {
    release_cuda_resource(indx);
  }
  for (size_t indx = 0; indx < nGpu; indx++) {
    size_t free, total;
    cudaSetDevice(indx);
    int id;
    cudaGetDevice(&id);
    assert(id == indx);
    cudaMemGetInfo(&free, &total);
    long used_mem = (free_mem_before[indx] - free);
    std::cout << "GPU " << id << " memory: free=" << free / (1024 * 1024)
              << " MB, free_mem_before=" << free_mem_before[indx] / (1024 * 1024)
              << " MB, total=" << total / (1024 * 1024)
              << " MB, used=" << used_mem / (1024 * 1024) << " MB" << std::endl;
  }

  std::cout << "VPF Becnchmark end press numeric \"1\" key" << std::endl;
  int i;
  std::cin >> i;
  return 0;
}
