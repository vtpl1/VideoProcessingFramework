#include "utils.hpp"

#include <stdio.h>
void writePPMFromRgbPlanar(float *rgbImg, int height, int width,
                           const char *opf) {
  FILE *fp1;
  int level = 255;
  unsigned char temp;
  int i;
  //	printf("\nwritePPM called: %s\n",opf);
  //	fflush(stdout);
  // Open the file with writing mode
  fp1 = fopen(opf, "wb");

  // If unable to open the file
  if (fp1 == NULL) {
    //		printf("\n PPM Open Error");
    //		fflush(stdout);
    return;
  }

  // writes the header information into the image file
  putc('P', fp1);
  putc('6', fp1);
  fprintf(fp1, "\n%d %d\n%d\n", width, height, level);
  float *r_ptr = rgbImg;
  float *g_ptr = (float *)(rgbImg + height * width);
  float *b_ptr = (float *)(rgbImg + height * width * 2);

  for (i = 0; i < height * width; i += 1) {
    temp = (unsigned char)(r_ptr[i] * 255.0);
    // temp = (char)(*((int *)rgbImg));
    fwrite(&temp, 1, 1, fp1);
    temp = (unsigned char)(g_ptr[i] * 255.0);
    // temp = (char)(*((int *)rgbImg) >> 8);
    fwrite(&temp, 1, 1, fp1);
    temp = (unsigned char)(b_ptr[i] * 255.0);
    // temp = (char)(*((int *)rgbImg) >> 16);
    fwrite(&temp, 1, 1, fp1);
    // rgbImg += 3;
  }

  putc(EOF, fp1);
  fclose(fp1);
}
/**
 * @brief Write PPM from RGB image
 *
 * @param rgbImg rgb image (3 channels)
 * @param height
 * @param width
 * @param opf
 */
void writePPMFromRgb(float *rgbImg, int height, int width, const char *opf) {
  FILE *fp1;
  int level = 255;
  unsigned char temp;
  int i;
  //	printf("\nwritePPM called: %s\n",opf);
  //	fflush(stdout);
  // Open the file with writing mode
  fp1 = fopen(opf, "wb");

  // If unable to open the file
  if (fp1 == NULL) {
    //		printf("\n PPM Open Error");
    //		fflush(stdout);
    return;
  }

  // writes the header information into the image file
  putc('P', fp1);
  putc('6', fp1);
  fprintf(fp1, "\n%d %d\n%d\n", width, height, level);

  for (i = 0; i < height * width * 3; i += 3) {
    temp = (unsigned char)(rgbImg[i + 0] * 255.0);
    // temp = (char)(*((int *)rgbImg));
    fwrite(&temp, 1, 1, fp1);
    temp = (unsigned char)(rgbImg[i + 1] * 255.0);
    // temp = (char)(*((int *)rgbImg) >> 8);
    fwrite(&temp, 1, 1, fp1);
    temp = (unsigned char)(rgbImg[i + 2] * 255.0);
    // temp = (char)(*((int *)rgbImg) >> 16);
    fwrite(&temp, 1, 1, fp1);
    // rgbImg += 3;
  }

  putc(EOF, fp1);
  fclose(fp1);
}
/**
 * @brief Write PPM from RGB image
 *
 * @param rgbImg rgb image (3 channels)
 * @param height
 * @param width
 * @param opf
 */
void writePPMFromRgb(unsigned char *rgbImg, int height, int width,
                     const char *opf) {
  FILE *fp1;
  int level = 255;
  char temp;
  int i;
  //	printf("\nwritePPM called: %s\n",opf);
  //	fflush(stdout);
  // Open the file with writing mode
  fp1 = fopen(opf, "wb");

  // If unable to open the file
  if (fp1 == NULL) {
    //		printf("\n PPM Open Error");
    //		fflush(stdout);
    return;
  }

  // writes the header information into the image file
  putc('P', fp1);
  putc('6', fp1);
  fprintf(fp1, "\n%d %d\n%d\n", width, height, level);

  for (i = 0; i < height * width; i++) {
    temp = (char)(*((int *)rgbImg));
    fwrite(&temp, 1, 1, fp1);
    temp = (char)(*((int *)rgbImg) >> 8);
    fwrite(&temp, 1, 1, fp1);
    temp = (char)(*((int *)rgbImg) >> 16);
    fwrite(&temp, 1, 1, fp1);
    rgbImg += 3;
  }

  putc(EOF, fp1);
  fclose(fp1);
}

/**
 * @brief Write PPM from RGB image
 *
 * @param bgrImg bgr image (3 channels)
 * @param height
 * @param width
 * @param opf
 */
void writePPMFromBgr(unsigned char *bgrImg, int height, int width,
                     const char *opf) {
  FILE *fp1;
  int level = 255;
  char temp;
  int i;
  //	printf("\nwritePPM called: %s\n",opf);
  //	fflush(stdout);
  // Open the file with writing mode
  fp1 = fopen(opf, "wb");

  // If unable to open the file
  if (fp1 == NULL) {
    //		printf("\n PPM Open Error");
    //		fflush(stdout);
    return;
  }

  // writes the header information into the image file
  putc('P', fp1);
  putc('6', fp1);
  fprintf(fp1, "\n%d %d\n%d\n", width, height, level);

  for (i = 0; i < height * width; i++) {
    temp = (char)(*((int *)bgrImg) >> 16);
    fwrite(&temp, 1, 1, fp1);
    temp = (char)(*((int *)bgrImg) >> 8);
    fwrite(&temp, 1, 1, fp1);
    temp = (char)(*((int *)bgrImg));
    fwrite(&temp, 1, 1, fp1);
    bgrImg += 3;
  }

  putc(EOF, fp1);
  fclose(fp1);
}