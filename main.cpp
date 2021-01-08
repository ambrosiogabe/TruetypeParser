#include "dataStructures.h"

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <map>

#include "readData.h"
#include "dataStructures.h"
#include "debugPrint.h"


uint32 findTableOffset(const char* fontData, const char* tag)
{
	uint8* data = (uint8*)fontData;
	int32 numTables = toUShort(data + 4);
	uint32 tableDir = 12;
	for (int32 i = 0; i < numTables; i++)
	{
		uint32 location = tableDir + (16 * i);
		if (tagEquals(data + location, tag))
		{
			return toULong(data + location + 8);
		}
	}
}

uint32 getGlyphId(uint16 character, FontInfo& fontInfo)
{
	uint8* data = (uint8*)fontInfo.data;
	uint32 cmapRecordOffset = fontInfo.cmap;
	uint16 segCountX2 = toUShort(data + cmapRecordOffset + 6);
	uint16 segCount = segCountX2 / 2;
	uint32 endCodeOffset = cmapRecordOffset + 14;
	uint32 startCodeOffset = endCodeOffset + segCountX2 + 2;
	uint32 idDeltaOffset = startCodeOffset + segCountX2;
	uint32 idRangeOffsets = idDeltaOffset + segCountX2;
	uint32 glyphIdArrayOffset = idRangeOffsets + segCountX2;

	uint16* idRangeOffset = (uint16*)(data + idRangeOffsets);
	uint16* idDeltaOffsetPtr = (uint16*)(data + idDeltaOffset);
	uint16* startCode = (uint16*)(data + startCodeOffset);
	uint16* endCode = (uint16*)(data + endCodeOffset);

	TTF_ASSERT(endCode[segCount - 1] == 0xffff);
	TTF_ASSERT(startCode[segCount - 1] == 0xffff);
	uint16 i = 0;
	for (; i < segCount; i++)
	{
		uint16 segStartCode = toUShort((uint8*)startCode + i);
		uint16 segEndCode = toUShort((uint8*)endCode + i);
		if (character >= segStartCode && character <= segEndCode)
		{
			break;
		}
	}

	uint16 charIdRangeOffset = toUShort((uint8*)idRangeOffset + i);
	uint16 charStartCode = toUShort((uint8*)startCode + i);
	uint16 idDelta = toUShort((uint8*)idDeltaOffsetPtr + i);
	uint16 val = *(charIdRangeOffset / 2 + (character - charStartCode) + &idRangeOffset[i]);


	if (charIdRangeOffset != 0)
	{
		uint16 startCodeOffset = (character - charStartCode) * 2;
		uint16 currentRangeOffset = i * 2;

		uint32 glyphIndexOffset = idRangeOffsets + currentRangeOffset + idRangeOffsets + startCodeOffset;
		uint32 glyphIndex = toUShort(data + glyphIndexOffset);

		if (glyphIndex != 0)
		{
			glyphIndex = (glyphIndex + idDelta) & 0xFFFF;
		}
		return glyphIndex;
	}
	else
	{
		return (character + idDelta) & 0xFFFF;
	}
}

Glyph getGlyph(uint16 character, FontInfo& fontInfo)
{
	uint8* data = (uint8*)fontInfo.data;
	uint32 glyphId = getGlyphId(character, fontInfo);

	// Loca table uses the glyph id to find the correct offset
	// If it is a short table version (i.e indexToLocFormat is 0)
	// Then each entry in the table is 2 bytes, hence the * 2, and it is half the size of the actual offset
	// Otherwise, each entry in the table is 4 bytes, hence the * 4, and it is the actual offset
	uint8* locaTablePtr = (uint8*)fontInfo.data + fontInfo.loca;
	uint32 locaOffset = fontInfo.indexToLocFormat == 0 ? (uint32)(toUShort(locaTablePtr + glyphId * 2)) * 2 : toULong(locaTablePtr + glyphId * 4);

	uint8* glyphPtr = data + fontInfo.glyf + locaOffset;
	Buffer glyphBuffer = getBuffer(glyphPtr, fontInfo);
	int16 numberOfContours = getInt16(glyphBuffer);
	int16 xmin = getInt16(glyphBuffer);
	int16 ymin = getInt16(glyphBuffer);
	int16 xmax = getInt16(glyphBuffer);
	int16 ymax = getInt16(glyphBuffer);

	if (numberOfContours >= 0)
	{
		return {
			numberOfContours,
			xmin,
			ymin,
			xmax,
			ymax,
			(uint8*)glyphBuffer.data + glyphBuffer.cursor,
			nullptr
		};
	}

	return {
		numberOfContours,
		xmin,
		ymin,
		xmax,
		ymax,
		nullptr,
		(uint8*)glyphBuffer.data + glyphBuffer.cursor
	};
}

uint8 flagBuffer[10000];
int16 xPoints[10000];
int16 yPoints[10000];
uint16 contourEnds[10];
void drawSimpleGlyph(const Glyph& glyph, const FontInfo& fontInfo)
{
	const uint16 ON_CURVE   = 0x01;
	const uint16 X_IS_BYTE  = 0x02;
	const uint16 Y_IS_BYTE  = 0x04;
	const uint16 REPEAT     = 0x08;
	const uint16 X_DELTA    = 0x10;
	const uint16 Y_DELTA    = 0x20;

	uint16* endPtsOfContours = (uint16*)glyph.simpleGlyphTable;
	uint16 instructionsLength = toUShort((uint8*)&endPtsOfContours[glyph.numberOfContours]);
	uint8* instructions = (uint8*)(endPtsOfContours + glyph.numberOfContours + 1);

	if (glyph.numberOfContours == 0)
	{
		return;
	}

	uint16 numPoints = 0;
	for (int i = 0; i < glyph.numberOfContours; i++)
	{
		contourEnds[i] = toUShort((uint8*)(endPtsOfContours + i));
		if (contourEnds[i] > numPoints)
		{
			numPoints = contourEnds[i];
		}
	}
	TTF_ASSERT(numPoints != 0);
	numPoints++;

	uint8* dataPtr = instructions + instructionsLength;
	if (instructionsLength == 0) dataPtr++;
	//printf("NumInstructions: %u\n", instructionsLength);

	Buffer dataBuffer = getBuffer(dataPtr, fontInfo);
	for (uint16 i = 0; i < numPoints; i++)
	{
		uint8 flag = getUint8(dataBuffer);
		flagBuffer[i] = flag;

		if (flag & REPEAT)
		{
			uint8 repeatCount = getUint8(dataBuffer);
			//printf("Repeat count: %u\n", repeatCount);
			TTF_ASSERT(repeatCount > 0);
			for (int j = 0; j < repeatCount; j++)
			{
				flagBuffer[i + j + 1] = flag;
			}
			i += repeatCount;
		}
	}

	// We read all the flags, now we are at xcoords
	int16 value = 0;
	for (uint16 i = 0; i < numPoints; i++)
	{
		uint16 flag = flagBuffer[i];
		if (flag & X_IS_BYTE)
		{
			if (flag & X_DELTA)
			{
				value += getUint8(dataBuffer);
			}
			else
			{
				value -= getUint8(dataBuffer);
			}
		}
		else if (~flag & X_DELTA)
		{
			value += getInt16(dataBuffer);
		}

		xPoints[i] = value;
	}

	// Now we are at y coords
	value = 0;
	for (uint16 i = 0; i < numPoints; i++)
	{
		uint16 flag = flagBuffer[i];
		if (flag & Y_IS_BYTE)
		{
			if (flag & Y_DELTA)
			{
				value += getUint8(dataBuffer);
			}
			else
			{
				value -= getUint8(dataBuffer);
			}
		}
		else if (~flag & Y_DELTA)
		{
			value += getInt16(dataBuffer);
		}

		yPoints[i] = value;
	}

	uint16 p = 0;
	uint16 c = 0;
	uint16 first = 1;
	float scale = 64.0f / fontInfo.unitsPerEm;
	float width = fontInfo.xMax - fontInfo.xMin;
	float height = fontInfo.yMax - fontInfo.yMin;
	//printf("canvas.width = %2.3f;\n", width * scale);
	//printf("canvas.height = %2.3f;\n", height * scale);
	//printf("var ctx = canvas.getContext(\"2d\");\n");
	//printf("ctx.scale(%2.3f, -%2.3f);\n", scale, scale);
	//printf("ctx.translate(%2.3f, %2.3f);\n", -fontInfo.xMin, -fontInfo.yMin - height);
	//printf("ctx.beginPath();\n");
	while (p < numPoints)
	{
		int16 x = xPoints[p];
		int16 y = yPoints[p];
		if (first == 1)
		{
			//printf("ctx.moveTo(%d, %d);\n", x, y);
			first = 0;
		}
		else
		{
			//printf("ctx.lineTo(%d, %d);\n", x, y);
		}

		if (p == contourEnds[c])
		{
			c++;
			first = 1;
		}

		p++;
	}
}

void drawCompositeGlyph(const Glyph& glyph)
{
	printf("Composite!\n");
}

void drawGlyph(const Glyph& glyph, const FontInfo& fontInfo)
{
	if (glyph.simpleGlyphTable != nullptr)
	{
		drawSimpleGlyph(glyph, fontInfo);
	}
	else if (glyph.compositeGlyphTable != nullptr)
	{
		drawCompositeGlyph(glyph);
	}
	else
	{
		TTF_ASSERT(0);
	}
}

void initFont(const char* fontData, FontInfo& fontInfo, int fontSize)
{
	uint8* data = (uint8*)fontData;
	fontInfo.data = fontData;
	fontInfo.fontSize = fontSize;

	int numTables = toUShort(data + 4);
	fontInfo.fontStart = 12 + numTables * 16;

	fontInfo.loca = findTableOffset(fontData, "loca");
	fontInfo.head = findTableOffset(fontData, "head");
	fontInfo.glyf = findTableOffset(fontData, "glyf");
	fontInfo.hhea = findTableOffset(fontData, "hhea");
	fontInfo.hmtx = findTableOffset(fontData, "hmtx");
	fontInfo.kern = findTableOffset(fontData, "kern");
	fontInfo.gpos = findTableOffset(fontData, "gpos");
	fontInfo.svg = findTableOffset(fontData, "svg ");

	// TODO: This table may not exist in some font formats
	uint32 maxpTable = findTableOffset(fontData, "maxp");
	fontInfo.numGlyphs = toUShort(data + maxpTable + 4);

	// Get font information from the head table
	Buffer headTableBuffer = getBuffer(data + fontInfo.head, fontInfo);
	skip(headTableBuffer, 18);
	fontInfo.unitsPerEm = getUint16(headTableBuffer);
	TTF_ASSERT(fontInfo.unitsPerEm >= 16 && fontInfo.unitsPerEm <= 16384);
	skip(headTableBuffer, 16);
	fontInfo.xMin = getInt16(headTableBuffer);
	fontInfo.yMin = getInt16(headTableBuffer);
	fontInfo.xMax = getInt16(headTableBuffer);
	fontInfo.yMax = getInt16(headTableBuffer);
	skip(headTableBuffer, 4);
	fontInfo.indexToLocFormat = getInt16(headTableBuffer);
	
	// Find an appropriate cmap table for a version we would like to use
	uint32 cmapTableOffset = findTableOffset(fontData, "cmap");

	uint16 cmapVersion = toUShort(data + cmapTableOffset);
	TTF_ASSERT(cmapVersion == 0);
	uint16 cmapNumTables = toUShort(data + cmapTableOffset + 2);
	uint32 cmapRecordOffset = -1;
	for (int i = 0; i < cmapNumTables; i++)
	{
		uint16 recordPlatformId = toUShort(data + cmapTableOffset + 4 + i * 8);
		uint16 recordEncodingId = toUShort(data + cmapTableOffset + 6 + i * 8);
		bool isWindowsPlatform = recordPlatformId == 3 && (recordEncodingId == 0 || recordEncodingId == 1 || recordEncodingId == 10);
		bool isUnicodePlatform = recordPlatformId == 0 && recordEncodingId >= 0 && recordEncodingId <= 4;

		if (isWindowsPlatform || isUnicodePlatform)
		{
			uint32 subtableOffset = toULong(data + cmapTableOffset + 8 + i * 8);
			fontInfo.cmap = cmapTableOffset + subtableOffset;
			break;
		}
	}
}

void parseFont(const char* filename)
{
	FILE* file = fopen(filename, "rb");
	if (!file)
	{
		printf("Error could not open file %s", filename);
		return;
	}

	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	rewind(file);

	char* data = (char*)malloc(sizeof(char) * size);
	if (!data)
	{
		printf("Error could not allocate enough memory");
		return;
	}
	size_t readAmt = fread(data, 1, size, file);
	//printf("Read %d/%d bytes\n\n", readAmt, size);

	Buffer buffer = {
		data,
		0,
		readAmt
	};

	FontInfo fontInfo = {};
	initFont(data, fontInfo, readAmt);

	uint32 sfntVersion = getUint32(buffer);
	if (sfntVersion != 0x00010000)
	{
		printf("Dealing with 'OTTO'. Unable to handle this file.");
		free(data);
		return;
	}

	// TODO: Indexing using 'string' varies from platform to platform
	// TODO: Consider making this platform independent
	seek(buffer, fontInfo.head);
	HeadTable headTable = {
		getUint16(buffer),
		getUint16(buffer),
		getFixed(buffer),
		getUint32(buffer),
		getUint32(buffer),
		getUint16(buffer),
		getUint16(buffer),
		getDateTime(buffer),
		getDateTime(buffer),
		getFWord(buffer),
		getFWord(buffer),
		getFWord(buffer),
		getFWord(buffer),
		getUint16(buffer),
		getUint16(buffer),
		getInt16(buffer),
		getInt16(buffer),
		getInt16(buffer)
	};

	seek(buffer, findTableOffset(buffer.data, "maxp"));
	MaxpTable maxpTable = {
		getFixed(buffer),
		getUint16(buffer),
		getUint16(buffer),
		getUint16(buffer),
		getUint16(buffer),
		getUint16(buffer),
		getUint16(buffer),
		getUint16(buffer),
		getUint16(buffer),
		getUint16(buffer),
		getUint16(buffer),
		getUint16(buffer),
		getUint16(buffer),
		getUint16(buffer),
		getUint16(buffer)
	};

	seek(buffer, fontInfo.hhea);
	HheaTable hheaTable = {
		getUint16(buffer),
		getUint16(buffer),
		getFWord(buffer),
		getFWord(buffer),
		getFWord(buffer),
		getUFWord(buffer),
		getFWord(buffer),
		getFWord(buffer),
		getFWord(buffer),
		getInt16(buffer),
		getInt16(buffer),
		getInt16(buffer),
		getInt32(buffer),
		getInt32(buffer),
		getInt16(buffer),
		getUint16(buffer)
	};

	//printf("\n");
	//printHeadTable(headTable);
	//printf("\n");
	//printMaxpTable(maxpTable);
	//printf("\n");
	//printHheaTable(hheaTable);

	seek(buffer, fontInfo.hmtx);
	HMetrics* hMetricsTable = (HMetrics*)malloc(sizeof(HMetrics) * hheaTable.numberOfHMetrics);
	for (int i = 0; i < hheaTable.numberOfHMetrics; i++)
	{
		hMetricsTable[i] = {
			getUint16(buffer),
			getInt16(buffer)
		};
	}

	// TODO: This table is only here for truetype fonts
	seek(buffer, fontInfo.loca);
	bool isShort = headTable.indexToLocFormat == 0;
	Loca* locaTable = (Loca*)malloc(sizeof(Loca) * maxpTable.numGlyphs + 1);
	for (int i = 0; i < 100;i++)//maxpTable.numGlyphs; i++)
	{
		if (isShort)
		{
			locaTable[i].isShort = true;
			locaTable[i].offset.shortOffset = getOffset16(buffer);
		}
		else
		{
			locaTable[i].isShort = false;
			locaTable[i].offset.longOffset = getOffset32(buffer);
		}
	}

	// TODO: This table is only here for truetype 
	int glyphOffset = fontInfo.glyf;
	Glyph* glyphs = (Glyph*)malloc(sizeof(Glyph) * maxpTable.numGlyphs);
	for (int i = 0; i < 100; i++)//maxpTable.numGlyphs; i++)
	{
		uint32 locaOffset = isShort ? locaTable[i].offset.shortOffset * 2 : locaTable[i].offset.longOffset;

		seek(buffer, glyphOffset + locaOffset);
		glyphs[i] = {
			getInt16(buffer),
			getInt16(buffer),
			getInt16(buffer),
			getInt16(buffer),
			getInt16(buffer)
		};
	}

	seek(buffer, findTableOffset(buffer.data, "cmap"));
	CMap cmapTable = {
		getUint16(buffer),
		getUint16(buffer),
		nullptr
	};

	if (cmapTable.version != 0)
	{
		printf("Error: Unable to handle cmap of version > 0. Version is %d", cmapTable.version);
		free(glyphs);
		free(locaTable);
		free(hMetricsTable);
		free(data);
		return;
	}

	cmapTable.encodingRecords = (EncodingRecord*)malloc(sizeof(EncodingRecord) * cmapTable.numTables);
	for (int i = 0; i < cmapTable.numTables; i++)
	{
		cmapTable.encodingRecords[i] = {
			getUint16(buffer),
			getUint16(buffer),
			getOffset32(buffer)
		};
	}

	Offset32 selectedOffset = -1;
	for (int i = 0; i < cmapTable.numTables; i++)
	{
		EncodingRecord& record = cmapTable.encodingRecords[i];
		bool isWindowsPlatform =
			record.platformId == 3 &&
			(
				record.encodingId == 0 ||
				record.encodingId == 1 ||
				record.encodingId == 10
				);

		bool isUnicodePlatform =
			record.platformId == 0 &&
			(
				record.encodingId == 0 ||
				record.encodingId == 1 ||
				record.encodingId == 2 ||
				record.encodingId == 3 ||
				record.encodingId == 4
				);

		if (isWindowsPlatform || isUnicodePlatform)
		{
			selectedOffset = record.subtableOffset;
			break;
		}
	}

	if (selectedOffset == -1)
	{
		printf("The font does not contain any recognized platform and encoding.");
		free(cmapTable.encodingRecords);
		free(glyphs);
		free(locaTable);
		free(hMetricsTable);
		free(data);
		return;
	}

	uint16 format = getUint16(buffer);
	if (format == 4)
	{
		Format4 format4 = {
			4,
			getUint16(buffer),
			getUint16(buffer),
			getUint16(buffer),
			getUint16(buffer),
			getUint16(buffer),
			getUint16(buffer),
			nullptr,
			nullptr,
			nullptr,
			nullptr
		};

		uint16 segCount = format4.segCountX2 / 2;
		format4.endCode = (uint16*)malloc(sizeof(uint16) * segCount);
		for (int i = 0; i < segCount; i++)
		{
			format4.endCode[i] = getUint16(buffer);
			if (i == segCount - 1)
			{
				TTF_ASSERT(format4.endCode[i] == 0xffff);
			}
		}
		skip(buffer, 2); // Reserved pad.

		format4.startCode = (uint16*)malloc(sizeof(uint16) * segCount);
		for (int i = 0; i < segCount; i++)
		{
			format4.startCode[i] = getUint16(buffer);
		}

		format4.idDelta = (int16*)malloc(sizeof(int16) * segCount);
		for (int i = 0; i < segCount; i++)
		{
			format4.idDelta[i] = getInt16(buffer);
		}

		int idRangeOffsetsStart = buffer.cursor;
		format4.idRangeOffsets = (uint16*)malloc(sizeof(uint16) * segCount);
		for (int i = 0; i < segCount; i++)
		{
			format4.idRangeOffsets[i] = getUint16(buffer);
		}

		for (int i = 0; i < segCount - 1; i++)
		{
			uint16 glyphIndex = 0;
			uint16 endCode = format4.endCode[i];
			uint16 startCode = format4.startCode[i];
			int16 idDelta = format4.idDelta[i];
			uint16 idRangeOffset = format4.idRangeOffsets[i];

			for (uint16 c = startCode; c < endCode; c++)
			{
				if (idRangeOffset != 0)
				{
					uint16 startCodeOffset = (c - startCode) * 2;
					uint16 currentRangeOffset = i * 2;

					int glyphIndexOffset = idRangeOffsetsStart + currentRangeOffset + idRangeOffset + startCodeOffset;
					seek(buffer, glyphIndexOffset);
					glyphIndex = getUint16(buffer);

					if (glyphIndex != 0)
					{
						glyphIndex = (glyphIndex + idDelta) & 0xFFFF;
					}
				}
				else
				{
					glyphIndex = (c + idDelta) & 0xFFFF;
				}
				format4.glyphIndexMap[c] = glyphIndex;
				format4.glyphSegmentMap[c] = i;
			}
		}

		for (uint16 c = 'A'; c <= 'z'; c++)
		{
			uint16 index = format4.glyphIndexMap[c];
			Glyph glyph = glyphs[index];
			HMetrics hmtx = hMetricsTable[index];

			float scale = ((float)(1.0f / headTable.unitsPerEm)) * 32.0f;
			int16 x = glyph.xMin;
			int16 y = glyph.yMin;
			int16 width = glyph.xMax - x;
			int16 height = glyph.yMax - y;
			int16 lsb = hmtx.leftSideBearing;
			int16 rsb = hmtx.advanceWidth - hmtx.leftSideBearing - width;
			Glyph calcGlyph = getGlyph(c, fontInfo);

			printf("'%c'\n", c);
			drawGlyph(calcGlyph, fontInfo);
			//printf("\tIndex: %u\n", format4.glyphIndexMap[c]);
			//printf("  Lsb: %2.3f\n", lsb);
			//printf("  Rsb: %2.3f\n\n", rsb);

			float rX = (x + lsb) * scale;
			float rY = 48 - (y + height) * scale;
			float rWidth = width * scale;
			float rHeight = height * scale;
			//printf("context.fillRect(%2.3f, %2.3f, %2.3f, %2.3f);\n", rX, rY, rWidth, rHeight);
		}

		// Free format data
		free(format4.endCode);
		free(format4.startCode);
		free(format4.idDelta);
		free(format4.idRangeOffsets);
	}
	else
	{
		printf("Unsupported format: %u. Required format 4", format);
		free(cmapTable.encodingRecords);
		free(glyphs);
		free(locaTable);
		free(hMetricsTable);
		free(data);
		return;
	}

	// Free data
	free(cmapTable.encodingRecords);
	free(glyphs);
	free(locaTable);
	free(hMetricsTable);
	free(data);
}

int main()
{
	parseFont("C:\\Windows\\Fonts\\arial.ttf");
	return 0;
}