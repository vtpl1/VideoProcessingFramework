#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "PyNvCodec.hpp"
#include "utils.hpp"
int decode_to_rgb(int argc, char const* argv[]) {
  std::string input("./../../videos/1_2.mp4");
  int gpu_id = 0;
  std::unique_ptr<PyNvDecoder> nv_dec;
  std::unique_ptr<PySurfaceConverter> from_nv12_to_yuv;
  std::unique_ptr<PySurfaceResizer> yuv_resizer;
  std::unique_ptr<PySurfaceConverter> from_yuv_to_rgb;
  std::unique_ptr<PySurfaceConverter> from_rgb_to_rgb_32f;
  std::unique_ptr<PySurfaceDownloader> rgb_32f_downloader;
  std::unique_ptr<PySurfaceConverter> from_rgb_32f_to_rgb_32f_planar;
  std::unique_ptr<PySurfaceDownloader> rgb_32f_planar_downloader;
  std::vector<float> rgb_32f_frame;
  std::vector<float> rgb_32f_planar_frame;
  std::vector<uint8_t> rgb_frame;
  int frame_count = 0;

  while (true) {
    if (!nv_dec) nv_dec.reset(new PyNvDecoder(input, gpu_id));
    std::shared_ptr<VPF::Surface> nv12_surface;
    try {
      nv12_surface = nv_dec->DecodeSingleSurface();
    } catch (const HwResetException& e) {
      std::cerr << e.what() << '\n';
      continue;
    }
    if (!nv12_surface) break;
    if (nv12_surface->Empty()) break;
    if (!from_nv12_to_yuv)
      from_nv12_to_yuv.reset(new PySurfaceConverter(
          nv12_surface->Width(), nv12_surface->Height(),
          nv12_surface->PixelFormat(), Pixel_Format::YUV420, gpu_id));
    auto yuv_surface = from_nv12_to_yuv->Execute(nv12_surface);
    if (!yuv_surface) break;
    if (yuv_surface->Empty()) break;
    if (!yuv_resizer)
      yuv_resizer.reset(
          new PySurfaceResizer(800, 800, yuv_surface->PixelFormat(), gpu_id));
    auto resized_yuv_surface = yuv_resizer->Execute(yuv_surface);
    if (!resized_yuv_surface) break;
    if (resized_yuv_surface->Empty()) break;
    if (!from_yuv_to_rgb)
      from_yuv_to_rgb.reset(new PySurfaceConverter(
          resized_yuv_surface->Width(), resized_yuv_surface->Height(),
          resized_yuv_surface->PixelFormat(), Pixel_Format::RGB, gpu_id));
    auto rgb_surface = from_yuv_to_rgb->Execute(resized_yuv_surface);
    if (!rgb_surface) break;
    if (rgb_surface->Empty()) break;
    if (!from_rgb_to_rgb_32f)
      from_rgb_to_rgb_32f.reset(new PySurfaceConverter(
          rgb_surface->Width(), rgb_surface->Height(),
          rgb_surface->PixelFormat(), Pixel_Format::RGB_32F, gpu_id));
    auto rgb_32f_surface = from_rgb_to_rgb_32f->Execute(rgb_surface);
    if (!rgb_32f_surface) break;
    if (rgb_32f_surface->Empty()) break;
    if (!rgb_32f_downloader)
      rgb_32f_downloader.reset(new PySurfaceDownloader(
          rgb_32f_surface->Width(), rgb_32f_surface->Height(),
          rgb_32f_surface->PixelFormat(), gpu_id));
    if (!rgb_32f_downloader->DownloadSingleSurface(rgb_32f_surface,
                                                   rgb_32f_frame))
      break;
    if (!from_rgb_32f_to_rgb_32f_planar)
      from_rgb_32f_to_rgb_32f_planar.reset(new PySurfaceConverter(
          rgb_32f_surface->Width(), rgb_32f_surface->Height(),
          rgb_32f_surface->PixelFormat(), Pixel_Format::RGB_32F_PLANAR, gpu_id));
    auto rgb_32f_planar_surface =
        from_rgb_32f_to_rgb_32f_planar->Execute(rgb_32f_surface);
    if (!rgb_32f_planar_surface) break;
    if (rgb_32f_planar_surface->Empty()) break;
    if (!rgb_32f_planar_downloader)
      rgb_32f_planar_downloader.reset(new PySurfaceDownloader(
          rgb_32f_planar_surface->Width(), rgb_32f_planar_surface->Height(),
          rgb_32f_planar_surface->PixelFormat(), gpu_id));
    if (!rgb_32f_planar_downloader->DownloadSingleSurface(rgb_32f_planar_surface,
                                                          rgb_32f_planar_frame))
      break;

    std::cout << " rgb_32f_frame size: " << rgb_32f_frame.size()
              << " rgb_32f_planar_frame size: " << rgb_32f_planar_frame.size()
              << std::endl;
    // if (rgb_frame.size() < rgb_32f_frame.size())
    //   rgb_frame.resize(rgb_32f_frame.size(), false);
    // if (frame_count == 10) {
    //   for (size_t i = 0; i < rgb_32f_frame.size(); i++) {
    //     rgb_frame[i] = rgb_32f_frame[i];
    //   }
    // }

    frame_count += 1;
    std::stringstream unique_file_name;
    unique_file_name << "dump/out" << std::setfill('0') << std::setw(5)
                     << frame_count << ".ppm";
    // writePPMFromRgb(rgb_32f_frame.data(), rgb_32f_surface->Height(),
    //                 rgb_32f_surface->Width(),
    //                 unique_file_name.str().c_str());
    writePPMFromRgbPlanar(
        rgb_32f_planar_frame.data(), rgb_32f_planar_surface->Height(),
        rgb_32f_planar_surface->Width(), unique_file_name.str().c_str());

    if (frame_count > 100) break;
  }

  // int frame_count = 0;
  // do {
  //   std::vector<float> frame;

  //   auto nv12_surface = nv_dec->DecodeSingleSurface();
  //   std::stringstream unique_file_name;
  //   if (!nv12_surface) {
  //     break;
  //   } else {
  //     if (!from_nv12_to_yuv) {
  //       from_nv12_to_yuv.reset(new PySurfaceConverter(
  //           nv_dec->Width(), nv_dec->Height(), nv_dec->GetPixelFormat(),
  //           Pixel_Format::RGB, gpu_id));
  //     }
  //     auto cvtSurface = from_nv12_to_yuv->Execute(nv12_surface);
  //     if (!cvtSurface) {
  //       break;
  //     } else {
  //       if (!nvDown) {
  //         nvDown.reset(
  //             new PySurfaceDownloader(nv_dec->Width(), nv_dec->Height(),
  //                                     from_nv12_to_yuv->GetFormat(),
  //                                     gpu_id));
  //       }
  //       nvDown->DownloadSingleSurface32F(cvtSurface, frame);
  //     }
  //     std::cout << " Width: " << nv_dec->Width()
  //               << " Height: " << nv_dec->Height() << " Size: " <<
  //               frame.size()
  //               << " GetPixelFormat: " << nv_dec->GetPixelFormat()
  //               << " GetFormat: " << from_nv12_to_yuv->GetFormat() <<
  //               std::endl;
  //     unique_file_name << "dump/out" << std::setfill('0') << std::setw(5) <<
  //     i
  //                      << ".ppm";
  //     // writePPMFromRgb(frame.data(), nv_dec->Height(), nv_dec->Width(),
  //     //                 unique_file_name.str().c_str());
  //     // fout.write((const char *)frame.data(), frame.size());
  //   }
  // } while (frame_count++ < 38);

  // fout.close();
  return 0;
}

int demux_decode_to_rgb(int argc, char const* argv[]) {
  std::string input("./../../videos/1_2.mp4");
  int gpu_id = 0;
  std::unique_ptr<PyFFmpegDemuxer> nv_demux;
  std::unique_ptr<PyNvDecoder> nv_dec;
  std::unique_ptr<PySurfaceConverter> from_nv12_to_yuv;
  std::unique_ptr<PySurfaceResizer> yuv_resizer;
  std::unique_ptr<PySurfaceConverter> from_yuv_to_rgb;
  std::unique_ptr<PySurfaceConverter> from_rgb_to_rgb_32f;
  std::unique_ptr<PySurfaceDownloader> rgb_32f_downloader;
  std::unique_ptr<PySurfaceConverter> from_rgb_32f_to_rgb_32f_planar;
  std::unique_ptr<PySurfaceDownloader> rgb_32f_planar_downloader;
  std::vector<float> rgb_32f_frame;
  std::vector<float> rgb_32f_planar_frame;
  std::vector<uint8_t> rgb_frame;
  std::vector<uint8_t> encoded_packet;
  int frame_count = 0;

  while (true) {
    if (!nv_demux) nv_demux.reset(new PyFFmpegDemuxer(input));
    if (!nv_demux->DemuxSinglePacket(encoded_packet)) {
      std::cerr << "DemuxSinglePacket error" << '\n';
      break;
    }
    // Here we got a valid width and height so we can proceed
    std::cout << "MONOTOSH: Width: " << nv_demux->Width()
              << " Height: " << nv_demux->Height()
              << " Format: " << nv_demux->Format()
              << " Codec: " << nv_demux->Codec() << std::endl;
    if (!nv_dec)
      nv_dec.reset(new PyNvDecoder(nv_demux->Width(), nv_demux->Height(),
                                   nv_demux->Format(), nv_demux->Codec(),
                                   gpu_id));
    std::shared_ptr<VPF::Surface> nv12_surface;
    try {
      nv12_surface = nv_dec->DecodeSurfaceFromPacket(encoded_packet);
    } catch (const HwResetException& e) {
      std::cerr << e.what() << '\n';
      continue;
    }
    if (!nv12_surface) continue;
    if (nv12_surface->Empty()) {
      continue;
    }
    if (!from_nv12_to_yuv)
      from_nv12_to_yuv.reset(new PySurfaceConverter(
          nv12_surface->Width(), nv12_surface->Height(),
          nv12_surface->PixelFormat(), Pixel_Format::YUV420, gpu_id));
    auto yuv_surface = from_nv12_to_yuv->Execute(nv12_surface);
    if (!yuv_surface) break;
    if (yuv_surface->Empty()) break;
    if (!yuv_resizer)
      yuv_resizer.reset(
          new PySurfaceResizer(800, 800, yuv_surface->PixelFormat(), gpu_id));
    auto resized_yuv_surface = yuv_resizer->Execute(yuv_surface);
    if (!resized_yuv_surface) break;
    if (resized_yuv_surface->Empty()) break;
    if (!from_yuv_to_rgb)
      from_yuv_to_rgb.reset(new PySurfaceConverter(
          resized_yuv_surface->Width(), resized_yuv_surface->Height(),
          resized_yuv_surface->PixelFormat(), Pixel_Format::RGB, gpu_id));
    auto rgb_surface = from_yuv_to_rgb->Execute(resized_yuv_surface);
    if (!rgb_surface) break;
    if (rgb_surface->Empty()) break;
    if (!from_rgb_to_rgb_32f)
      from_rgb_to_rgb_32f.reset(new PySurfaceConverter(
          rgb_surface->Width(), rgb_surface->Height(),
          rgb_surface->PixelFormat(), Pixel_Format::RGB_32F, gpu_id));
    auto rgb_32f_surface = from_rgb_to_rgb_32f->Execute(rgb_surface);
    if (!rgb_32f_surface) break;
    if (rgb_32f_surface->Empty()) break;
    if (!rgb_32f_downloader)
      rgb_32f_downloader.reset(new PySurfaceDownloader(
          rgb_32f_surface->Width(), rgb_32f_surface->Height(),
          rgb_32f_surface->PixelFormat(), gpu_id));
    if (!rgb_32f_downloader->DownloadSingleSurface(rgb_32f_surface,
                                                   rgb_32f_frame))
      break;
    if (!from_rgb_32f_to_rgb_32f_planar)
      from_rgb_32f_to_rgb_32f_planar.reset(new PySurfaceConverter(
          rgb_32f_surface->Width(), rgb_32f_surface->Height(),
          rgb_32f_surface->PixelFormat(), Pixel_Format::RGB_32F_PLANAR, gpu_id));
    auto rgb_32f_planar_surface =
        from_rgb_32f_to_rgb_32f_planar->Execute(rgb_32f_surface);
    if (!rgb_32f_planar_surface) break;
    if (rgb_32f_planar_surface->Empty()) break;
    if (!rgb_32f_planar_downloader)
      rgb_32f_planar_downloader.reset(new PySurfaceDownloader(
          rgb_32f_planar_surface->Width(), rgb_32f_planar_surface->Height(),
          rgb_32f_planar_surface->PixelFormat(), gpu_id));
    if (!rgb_32f_planar_downloader->DownloadSingleSurface(rgb_32f_planar_surface,
                                                          rgb_32f_planar_frame))
      break;

    std::cout << " rgb_32f_frame size: " << rgb_32f_frame.size()
              << " rgb_32f_planar_frame size: " << rgb_32f_planar_frame.size()
              << std::endl;
    // if (rgb_frame.size() < rgb_32f_frame.size())
    //   rgb_frame.resize(rgb_32f_frame.size(), false);
    // if (frame_count == 10) {
    //   for (size_t i = 0; i < rgb_32f_frame.size(); i++) {
    //     rgb_frame[i] = rgb_32f_frame[i];
    //   }
    // }

    frame_count += 1;
    std::stringstream unique_file_name;
    unique_file_name << "dump/out" << std::setfill('0') << std::setw(5)
                     << frame_count << ".ppm";
    // writePPMFromRgb(rgb_32f_frame.data(), rgb_32f_surface->Height(),
    //                 rgb_32f_surface->Width(),
    //                 unique_file_name.str().c_str());
    writePPMFromRgbPlanar(
        rgb_32f_planar_frame.data(), rgb_32f_planar_surface->Height(),
        rgb_32f_planar_surface->Width(), unique_file_name.str().c_str());

    if (frame_count > 100) break;
  }

  return 0;
}

int demux_and_decode(int argc, char const* argv[]) {
  std::string input("./../../videos/1_2.mp4");
  int gpu_id = 0;
  std::unique_ptr<PyFFmpegDemuxer> nv_demux;
  std::unique_ptr<PyNvDecoder> nv_dec;
  std::vector<uint8_t> encoded_packet;
  std::vector<uint8_t> raw_frame;
  size_t encoded_frame_count = 0;
  size_t decoded_frame_count = 0;
  while (true) {
    if (!nv_demux) {
      std::cout << "Creating demuxer" << std::endl;
      nv_demux.reset(new PyFFmpegDemuxer(input));
    }
    if (!nv_demux->DemuxSinglePacket(encoded_packet)) {
      std::cerr << "DemuxSinglePacket error" << '\n';
      break;
    }
    encoded_frame_count++;
    // Here we got a valid width and height so we can proceed

    if (!nv_dec) {
      std::cout << "Creating decoder" << std::endl;
      nv_dec.reset(new PyNvDecoder(nv_demux->Width(), nv_demux->Height(),
                                   nv_demux->Format(), nv_demux->Codec(),
                                   gpu_id));
      std::cout << "MONOTOSH: Width: " << nv_demux->Width()
                << " Height: " << nv_demux->Height()
                << " Format: " << nv_demux->Format()
                << " Codec: " << nv_demux->Codec() << std::endl;
    }
    if (nv_dec->DecodeFrameFromPacket(raw_frame, encoded_packet)) {      
      decoded_frame_count++;
    }
    std::cout << "MONOTOSH: decode   frame" << decoded_frame_count << " encoded frames: " << encoded_frame_count << std::endl;
  }

  while (true) {
    if (nv_dec->FlushSingleFrame(raw_frame)) {      
      decoded_frame_count++;      
    } else {
      break;
    }
    std::cout << "MONOTOSH: flushing frame" << decoded_frame_count << " encoded frames: " << encoded_frame_count << std::endl;
  }
}

int main(int argc, char const* argv[]) { return demux_and_decode(argc, argv); }