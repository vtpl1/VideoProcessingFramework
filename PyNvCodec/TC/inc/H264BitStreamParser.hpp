#ifndef H264BitStreamParser_H
#define H264BitStreamParser_H
class H264BitStreamParser {
 private:
  /* data */
  int m_nWidth;
  int m_nHeight;
  const unsigned char* m_pStart;
  unsigned short m_nLength;
  int m_nCurrentBit;

  unsigned int ReadBit();
  unsigned int ReadBits(int n);
  unsigned int ReadExponentialGolombCode();
  unsigned int ReadSE();

 public:
  H264BitStreamParser();
  ~H264BitStreamParser();
  bool ParseNAL(const unsigned char* pStart, unsigned short nLen);
  int GetWidth() { return m_nWidth; }
  int GetHeight() { return m_nHeight; }
};

#endif
