#pragma once
#include "dataStructures.h"

namespace Truetype
{
	void writeUint8(Buffer& buffer, uint8 data)
	{
		TTF_ASSERT(buffer.cursor < buffer.size&& buffer.cursor >= 0);
		if (buffer.cursor >= buffer.size || buffer.cursor < 0)
			return;
		buffer.data[buffer.cursor] = data;
		buffer.cursor++;
	}

	void write(Buffer& buffer, uint32 numBytes, uint8* data)
	{
		TTF_ASSERT(numBytes >= 1 && numBytes <= 8);
		for (int i = numBytes - 1; i >= 0; i--)
			writeUint8(buffer, data[i]);
	}

	void writeUint16(Buffer& buffer, uint16 data)
	{
		write(buffer, 2, (uint8*)&data);
	}

	void writeUint32(Buffer& buffer, uint32 data)
	{
		write(buffer, 4, (uint8*)&data);
	}

	void writeUint64(Buffer& buffer, uint64 data)
	{
		write(buffer, 8, (uint8*)&data);
	}

	Buffer getBuffer(uint8* data, uint32 size)
	{
		return {
			(char*)data,
			0,
			(int)size
		};
	}
}