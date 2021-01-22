#pragma once
#include "dataStructures.h"

namespace Truetype
{
	void write(Buffer& buffer, uint32 numBytes, uint8* data);
	void writeUint8(Buffer& buffer, uint8 data);
	void writeUint16(Buffer& buffer, uint16 data);

	void writeInt16(Buffer& buffer, int16 data);
	void writeUint32(Buffer& buffer, uint32 data);
	void writeUint64(Buffer& buffer, uint64 data);

	Buffer getBuffer(uint8* data, uint32 size);
}