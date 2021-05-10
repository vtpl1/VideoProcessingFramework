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
    pSurfaceRGB8 = Surface::Make(RGB, in_width, in_height, context);
    pSurfaceResizedRGB8 = Surface::Make(RGB, out_width, out_height, context);
    pSurfaceRGB32F = Surface::Make(RGB_32F, out_width, out_height, context);
    pSurfaceRGB32FPlanar = Surface::Make(RGB_32F_PLANAR, out_width, out_height, context);
    pSurfaceRGB32FPlanarContigous = Surface::Make(RGB_32F_PLANAR_CONTIGUOUS, out_width, out_height, context);
  }
  ~nv12_rgb32f_deinterleave() {
    delete pSurfaceRGB8;
    delete pSurfaceResizedRGB8;
    delete pSurfaceRGB32F;
    delete pSurfaceRGB32FPlanar;
    delete pSurfaceRGB32FPlanarContigous;
  }
  Token *Execute(Token *pInputNV12) override {
    if (!pInputNV12) {
      return nullptr;
    }
    auto pInput = (Surface *)pInputNV12;
    const Npp8u *const pSrc[] = {(const Npp8u *const)pInput->PlanePtr(0U),
                                 (const Npp8u *const)pInput->PlanePtr(1U)};

    auto pDst = (Npp8u *)pSurfaceRGB8->PlanePtr();
    NppiSize oSizeRoi = {(int)pInput->Width(), (int)pInput->Height()};    
    CudaCtxPush ctxPush(cu_ctx);

    auto err = nppiNV12ToRGB_709HDTV_8u_P2C3R_Ctx(
        pSrc, pInput->Pitch(), pDst, pSurfaceRGB8->Pitch(), oSizeRoi, nppCtx);
    if (NPP_NO_ERROR != err) {
      cerr << "Failed to convert surface. Error code: " << err << endl;
      return nullptr;
    }

    if (pSurfaceResizedRGB8->PixelFormat() != pSurfaceRGB8->PixelFormat()) {
      cerr << "pSurfaceRGB8 and pSurfaceResizedRGB8 does not match  " << endl;
      return nullptr;
    }

    auto srcPlane = pSurfaceRGB8->GetSurfacePlane();
    auto dstPlane = pSurfaceResizedRGB8->GetSurfacePlane();

    const Npp8u *pSrc2 = (const Npp8u *)srcPlane->GpuMem();
    int nSrcStep2 = (int)pSurfaceRGB8->Pitch();
    NppiSize oSrcSize2 = {0};
    oSrcSize2.width = pSurfaceRGB8->Width();
    oSrcSize2.height = pSurfaceRGB8->Height();
    NppiRect oSrcRectROI2 = {0};
    oSrcRectROI2.width = oSrcSize2.width;
    oSrcRectROI2.height = oSrcSize2.height;

    Npp8u *pDst2 = (Npp8u *)dstPlane->GpuMem();
    int nDstStep2 = (int)pSurfaceResizedRGB8->Pitch();
    NppiSize oDstSize2 = {0};
    oDstSize2.width = pSurfaceResizedRGB8->Width();
    oDstSize2.height = pSurfaceResizedRGB8->Height();
    NppiRect oDstRectROI2 = {0};
    oDstRectROI2.width = oDstSize2.width;
    oDstRectROI2.height = oDstSize2.height;

    int eInterpolation = NPPI_INTER_LANCZOS;

    auto err2 = nppiResize_8u_C3R_Ctx(pSrc2, nSrcStep2, oSrcSize2, oSrcRectROI2,
                                      pDst2, nDstStep2, oDstSize2, oDstRectROI2,
                                      eInterpolation, nppCtx);
    if (NPP_NO_ERROR != err2) {
      cerr << "Can't resize 3-channel packed image. Error code: " << err2
           << endl;
      return nullptr;
    }

    const Npp8u *pSrc3 = (const Npp8u *)pSurfaceResizedRGB8->PlanePtr();

    int nSrcStep3 = pSurfaceResizedRGB8->Pitch();
    Npp32f *pDst3 = (Npp32f *)pSurfaceRGB32F->PlanePtr();
    int nDstStep3 = pSurfaceRGB32F->Pitch();
    NppiSize oSizeRoi3 = {0};
    oSizeRoi3.height = pSurfaceRGB32F->Height();
    oSizeRoi3.width = pSurfaceRGB32F->Width();
    Npp32f nMin = 0.0;
    Npp32f nMax = 1.0;

    auto err3 = nppiScale_8u32f_C3R_Ctx(pSrc3, nSrcStep3, pDst3, nDstStep3,
                                        oSizeRoi3, nMin, nMax, nppCtx);
    if (NPP_NO_ERROR != err3) {
      cerr << "Failed to convert surface. Error code: " << err3 << endl;
      return nullptr;
    }

    if (RGB_32F != pSurfaceRGB32F->PixelFormat()) {
      return nullptr;
    }

    const Npp32f *pSrc4 = (const Npp32f *)pSurfaceRGB32F->PlanePtr();
    int nSrcStep4 = pSurfaceRGB32F->Pitch();
    Npp32f *aDst4[] = {(Npp32f *)((uint8_t *)pSurfaceRGB32FPlanar->PlanePtr()),
                       (Npp32f *)((uint8_t *)pSurfaceRGB32FPlanar->PlanePtr() +
                                  pSurfaceRGB32FPlanar->Height() * pSurfaceRGB32FPlanar->Pitch()),
                       (Npp32f *)((uint8_t *)pSurfaceRGB32FPlanar->PlanePtr() +
                                  pSurfaceRGB32FPlanar->Height() * pSurfaceRGB32FPlanar->Pitch() * 2)};
    int nDstStep4 = pSurfaceRGB32FPlanar->Pitch();
    NppiSize oSizeRoi4 = {0};
    oSizeRoi4.height = pSurfaceRGB32FPlanar->Height();
    oSizeRoi4.width = pSurfaceRGB32FPlanar->Width();
    auto err4 =
        nppiCopy_32f_C3P3R_Ctx(pSrc4, nSrcStep4, aDst4, nDstStep4, oSizeRoi4, nppCtx);
    if (NPP_NO_ERROR != err4) {
      cerr << "Failed to convert surface. Error code: " << err4 << endl;
      return nullptr;
    }
    CUDA_MEMCPY2D m = {0};
    m.srcMemoryType = CU_MEMORYTYPE_DEVICE;
    m.dstMemoryType = CU_MEMORYTYPE_DEVICE;
    CUdeviceptr pDstDevice = pSurfaceRGB32FPlanarContigous->PlanePtr();
    for (auto plane = 0; plane < pSurfaceRGB32FPlanar->NumPlanes(); plane++) {
      m.srcDevice = pSurfaceRGB32FPlanar->PlanePtr(plane);
      m.srcPitch = pSurfaceRGB32FPlanar->Pitch(plane);
      m.dstDevice = pDstDevice;
      m.dstPitch = pSurfaceRGB32FPlanar->WidthInBytes(plane);
      m.WidthInBytes = pSurfaceRGB32FPlanar->WidthInBytes(plane);
      m.Height = pSurfaceRGB32FPlanar->Height(plane);

      if (CUDA_SUCCESS != cuMemcpy2DAsync(&m, nppCtx.hStream)) {
        return nullptr;
      }

      pDstDevice += m.WidthInBytes * m.Height;
    }
    return pSurfaceRGB32FPlanarContigous;
  }
  Surface *pSurfaceRGB8 = nullptr;
  Surface *pSurfaceResizedRGB8 = nullptr;
  Surface *pSurfaceRGB32F = nullptr;
  Surface *pSurfaceRGB32FPlanar = nullptr;
  Surface *pSurfaceRGB32FPlanarContigous = nullptr;
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

TaskExecStatus PreprocessSurface::Execute() {
  ClearOutputs();
  auto pOutput = pImpl->Execute(GetInput(0));
  SetOutput(pOutput, 0U);
  return TASK_EXEC_SUCCESS;
}
