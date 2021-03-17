void writePPMFromRgbPlanar(float *rgbImg, int height, int width,
                           const char *opf);
void writePPMFromRgb(float *rgbImg, int height, int width, const char *opf);
void writePPMFromRgb(unsigned char *rgbImg, int height, int width,
                     const char *opf);
void writePPMFromBgr(unsigned char *bgrImg, int height, int width,
                     const char *opf);