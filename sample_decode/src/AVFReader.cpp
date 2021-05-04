#include "AVFReader.h"
#include "Vtype_MIDT.h"
#include <iostream>
#include <string.h>

AVFReader::AVFReader(const std::string &path)
	: _path(path)
{
}

int AVFReader::open()
{
	_file.open(_path, std::ios::binary);
	return 0;
}

int AVFReader::close()
{
	_file.close();
	return 0;
}

AVFReader::~AVFReader()
{
	close();
}

int AVFReader::getFrame(FrameInfo &frameInfo, uint8_t *data)
{
	char magicStr[5] = {
		0,
	};
	if (!_file.is_open())
		open();
	int rollBackPos = _file.tellg();
	_file.read(magicStr, 4);

	BInt64 refFrameOff = -1;
	BInt32 mediaType = -1;
	BInt32 frameType = -1;
	BInt64 timeStamp = -1;
	BInt32 frameLength = -1;
	BInt64 currentFrameOff = 0;

	if (strcmp(magicStr, "00dc") == 0)
	{
		_file.read((char *)&refFrameOff, sizeof(BInt64));
		_file.read((char *)&mediaType, sizeof(BInt32));
		frameInfo.mediaType = (Int32)mediaType;

		_file.read((char *)&frameType, sizeof(BInt32));
		frameInfo.frameType = (Int32)frameType;

		_file.read((char *)&timeStamp, sizeof(BInt64));
		frameInfo.timeStamp = (Int64)timeStamp;

		_file.read((char *)&frameLength, sizeof(BInt32));
		frameInfo.frameSize = (Int32)frameLength;
		// std::cout << " mediaType " << frameInfo.mediaType << " frameType " << frameInfo.frameType << " timeStamp " << frameInfo.timeStamp << " frameSize " << frameInfo.frameSize << std::endl;
		if (frameInfo.frameSize + sizeof(FrameInfo) > frameInfo.totalSize)
		{

			_file.seekg(rollBackPos, std::ios::beg);
			frameInfo.totalSize = frameInfo.frameSize + sizeof(FrameInfo);
			return 9;
		}
		_file.read((char *)data, frameInfo.frameSize);
		_file.read((char *)&currentFrameOff, sizeof(BInt64));
		return 0;
	}
	return 2;
}