#pragma once
#include "dataStructures.h"

#define getFWord     getInt16
#define getUFWord    getUint16
#define getOffset16  getUint16
#define getOffset32  getUint32
#define getFixed     getUint32

namespace Truetype
{
	bool tagEquals(uint8* p, char c0, char c1, char c2, char c3);
	bool tagEquals(uint8* p, const char* str);

	uint16 toUShort(uint8* pointer);
	int16 toShort(uint8* pointer);
	uint32 toULong(uint8* pointer);
	int32 toLong(uint8* pointer);

	uint8 peekUint8(Buffer& buffer);
	void seek(Buffer& buffer, int offset);
	void skip(Buffer& buffer, int numBytes);

	uint32 get(Buffer& buffer, int numBytes);
	uint8 getUint8(Buffer& buffer);
	uint16 getUint16(Buffer& buffer);
	uint32 getUint32(Buffer& buffer);
	uint64 getUint64(Buffer& buffer);

	int16 getInt16(Buffer& buffer); 
	int32 getInt32(Buffer& buffer);

	Tag getTag(Buffer& buffer);

	LongDateTime getDateTime(Buffer& buffer);

	Buffer getBuffer(uint8* dataPtr, const FontInfo& fontInfo);
}