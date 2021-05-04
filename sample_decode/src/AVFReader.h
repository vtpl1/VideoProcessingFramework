#ifndef AVFReader_H
#define AVFReader_H
#include <stdint.h>
#include <istream>
#include <fstream>
typedef struct _FrameInfo_
{
	uint32_t totalSize;
	uint32_t frameType;
	uint32_t mediaType;
	uint32_t frameSize;
	int64_t timeStamp;
}FrameInfo;

class AVFReader
{
public:
	AVFReader(const std::string& path);
	~AVFReader();
	int getFrame(FrameInfo& frameInfo, uint8_t* data);
private:
	std::string _path;
	std::ifstream _file;
	int open();
	int close();
};

#endif // !AVFReader_H


