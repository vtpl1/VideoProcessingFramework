#include <iostream>
#include <sstream>
#include <stdexcept>

#include "CodecsSupport.hpp"
#include "MemoryInterfaces.hpp"
#include "NppCommon.hpp"
#include "Tasks.hpp"

using namespace VPF;
using namespace std;

constexpr auto TASK_EXEC_SUCCESS = TaskExecStatus::TASK_EXEC_SUCCESS;
constexpr auto TASK_EXEC_FAIL = TaskExecStatus::TASK_EXEC_FAIL;

namespace VPF {
struct NppPreprocessSurface_Impl {
  NppPreprocessSurface_Impl(CUcontext ctx, CUstream str)
      : cu_ctx(ctx), cu_str(str) {
    SetupNppContext(cu_ctx, cu_str, nppCtx);
  }
  virtual ~NppPreprocessSurface_Impl() = default;
  virtual Token *Execute(Token *pInput) = 0;
  CUcontext cu_ctx;
  CUstream cu_str;
  NppStreamContext nppCtx;
};
struct nv12_rgb32f_deinterleave final : public NppPreprocessSurface_Impl {
  nv12_rgb32f_deinterleave(uint32_t in_width, uint32_t in_height,
                           uint32_t out_width, uint32_t out_height,
                           CUcontext context, CUstream stream)
      : NppPreprocessSurface_Impl(context, stream) {
    pSurfaceRGB = Surface::Make(RGB, in_width, in_height, context);
    pSurfaceResizedRGB = Surface::Make(RGB, out_width, out_height, context);
  }
  ~nv12_rgb32f_deinterleave() {
    delete pSurfaceRGB;
    delete pSurfaceResizedRGB;
  }
  Token *Execute(Token *pInputNV12) override {
    if (!pInputNV12) {
      return nullptr;
    }
    auto pInput = (Surface *)pInputNV12;
    const Npp8u *const pSrc[] = {(const Npp8u *const)pInput->PlanePtr(0U),
                                 (const Npp8u *const)pInput->PlanePtr(1U)};

    auto pDst = (Npp8u *)pSurfaceRGB->PlanePtr();
    NppiSize oSizeRoi = {(int)pInput->Width(), (int)pInput->Height()};
    NppLock lock(nppCtx);
    CudaCtxPush ctxPush(cu_ctx);
    auto err = nppiNV12ToRGB_709HDTV_8u_P2C3R_Ctx(
        pSrc, pInput->Pitch(), pDst, pSurfaceRGB->Pitch(), oSizeRoi, nppCtx);
    if (NPP_NO_ERROR != err) {
      cerr << "Failed to convert surface. Error code: " << err << endl;
      return nullptr;
    }

    if (pSurfaceResizedRGB->PixelFormat() != pSurfaceRGB->PixelFormat()) {
      cerr << "pSurfaceRGB and pSurfaceResizedRGB does not match  " << endl;
      return nullptr;
    }

    auto srcPlane = pSurfaceRGB->GetSurfacePlane();
    auto dstPlane = pSurfaceResizedRGB->GetSurfacePlane();

    const Npp8u *pSrc2 = (const Npp8u *)srcPlane->GpuMem();
    int nSrcStep2 = (int)srcPlane->Pitch();
    NppiSize oSrcSize2 = {0};
    oSrcSize2.width = srcPlane->Width();
    oSrcSize2.height = srcPlane->Height();
    NppiRect oSrcRectROI2 = {0};
    oSrcRectROI2.width = oSrcSize2.width;
    oSrcRectROI2.height = oSrcSize2.height;

    Npp8u *pDst2 = (Npp8u *)dstPlane->GpuMem();
    int nDstStep2 = (int)dstPlane->Pitch();
    NppiSize oDstSize2 = {0};
    oDstSize2.width = dstPlane->Width();
    oDstSize2.height = dstPlane->Height();
    NppiRect oDstRectROI2 = {0};
    oDstRectROI2.width = oDstSize2.width;
    oDstRectROI2.height = oDstSize2.height;
    int eInterpolation = NPPI_INTER_LANCZOS;

    auto ret = nppiResize_8u_C3R_Ctx(pSrc2, nSrcStep2, oSrcSize2, oSrcRectROI2,
                                     pDst2, nDstStep2, oDstSize2, oDstRectROI2,
                                     eInterpolation, nppCtx);
    if (NPP_NO_ERROR != ret) {
      cerr << "Can't resize 3-channel packed image. Error code: " << ret
           << endl;
      return nullptr;
    }
    return pSurfaceRGB;
  }
  Surface *pSurfaceRGB = nullptr;
  Surface *pSurfaceResizedRGB = nullptr;
};

}  // namespace VPF

PreprocessSurface::PreprocessSurface(uint32_t in_width, uint32_t in_height,
                                     Pixel_Format inFormat, uint32_t out_width,
                                     uint32_t out_height,
                                     Pixel_Format outFormat, CUcontext ctx,
                                     CUstream str)
    : Task("NppPreprocessSurface", PreprocessSurface::numInputs,
           PreprocessSurface::numOutputs) {
  if (NV12 == inFormat && RGB_32F_PLANAR == outFormat) {
    pImpl = new nv12_rgb32f_deinterleave(in_width, in_height, out_width,
                                         out_height, ctx, str);
  } else {
    stringstream ss;
    ss << "Unsupported pixel format conversion: " << inFormat << " to "
       << outFormat;
    throw invalid_argument(ss.str());
  }
}

PreprocessSurface::~PreprocessSurface() { delete pImpl; }

PreprocessSurface *PreprocessSurface::Make(
    uint32_t in_width, uint32_t in_height, Pixel_Format inFormat,
    uint32_t out_width, uint32_t out_height, Pixel_Format outFormat,
    CUcontext ctx, CUstream str) {
  return new PreprocessSurface(in_width, in_height, inFormat, out_width,
                               out_height, outFormat, ctx, str);
}

TaskExecStatus PreprocessSurface::Execute() { return TASK_EXEC_FAIL; }