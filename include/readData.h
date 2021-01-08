#pragma once
#include "dataStructures.h"

#define getFWord     getInt16
#define getUFWord    getUint16
#define getOffset16  getUint16
#define getOffset32  getUint32
#define getFixed     getUint32

namespace Truetype
{
	bool tagEquals(uint8* p, char c0, char c1, char c2, char c3) { return ((p)[0] == (c0) && (p)[1] == (c1) && (p)[2] == (c2) && (p)[3] == (c3)); }
	bool tagEquals(uint8* p, const char* str) { return tagEquals(p, str[0], str[1], str[2], str[3]); }

	uint16 toUShort(uint8* pointer) { return pointer[0] * 256 + pointer[1]; }
	int16 toShort(uint8* pointer) { return pointer[0] * 256 + pointer[1]; }
	uint32 toULong(uint8* pointer) { return (pointer[0] << 24) | (pointer[1] << 16) | (pointer[2] << 8) | pointer[3]; }
	int32 toLong(uint8* pointer) { return (pointer[0] << 24) | (pointer[1] << 16) | (pointer[2] << 8) | pointer[3]; }

	uint8 getUint8(Buffer& buffer)
	{
		if (buffer.cursor >= buffer.size)
			return 0;
		return buffer.data[buffer.cursor++];
	}

	uint8 peekUint8(Buffer& buffer)
	{
		if (buffer.cursor >= buffer.size)
			return 0;
		return buffer.data[buffer.cursor];
	}

	void seek(Buffer& buffer, int offset)
	{
		TTF_ASSERT(!(offset > buffer.size || offset < 0));
		buffer.cursor = (offset > buffer.size || offset < 0) ? buffer.size : offset;
	}

	void skip(Buffer& buffer, int numBytes)
	{
		seek(buffer, buffer.cursor + numBytes);
	}

	uint32 get(Buffer& buffer, int numBytes)
	{
		uint32 value = 0;
		TTF_ASSERT(numBytes >= 1 && numBytes <= 4);
		for (int i = 0; i < numBytes; i++)
			value = (value << 8) | getUint8(buffer);
		return value;
	}

	uint16 getUint16(Buffer& buffer)
	{
		return get(buffer, 2);
	}

	uint32 getUint32(Buffer& buffer)
	{
		return get(buffer, 4);
	}

	uint64 getUint64(Buffer& buffer)
	{
		return (get(buffer, 4) << 32) | get(buffer, 4);
	}

	int16 getInt16(Buffer& buffer)
	{
		// Read the data as 2's complement if necessary
		// See here: https://stackoverflow.com/a/60881515
		uint16 val = getUint16(buffer);
		if (val & 0x8000) return -(int)(~val) - 1;
		return val;
	}


	int32 getInt32(Buffer& buffer)
	{
		return get(buffer, 4);
	}

	Tag getTag(Buffer& buffer)
	{
		Tag tag = getUint32(buffer);
		return tag;
	}

	LongDateTime getDateTime(Buffer& buffer)
	{
		LongDateTime date = getUint64(buffer);
		return date;
	}

	std::string toString(Tag tag)
	{
		std::string str = { (char)(tag >> 24), (char)(tag >> 16), (char)(tag >> 8), (char)tag };
		return str;
	}

	Buffer getBuffer(uint8* dataPtr, const FontInfo& fontInfo)
	{
		return {
			(char*)dataPtr,
			0,
			(int)(fontInfo.data + fontInfo.fontSize - (char*)dataPtr)
		};
	}
}