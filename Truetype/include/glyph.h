#pragma once
#include <iostream>
#include <stdlib.h>
#include <stdio.h>

#include "readData.h"
#include "writeData.h"
#include "dataStructures.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_write.h"

namespace Truetype
{
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

	uint32 getGlyphId(uint16 character, const FontInfo& fontInfo)
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
		uint16 segment = segCount;
		for (uint16 i = 0; i < segCount; i++)
		{
			uint16 segStartCode = toUShort((uint8*)(startCode + i));
			uint16 segEndCode = toUShort((uint8*)(endCode + i));
			if (character >= segStartCode && character <= segEndCode)
			{
				segment = i;
				break;
			}
		}

		if (segment == segCount)
		{
			return 0;
		}

		uint16 charIdRangeOffset = toUShort((uint8*)(idRangeOffset + segment));
		uint16 charStartCode = toUShort((uint8*)(startCode + segment));
		uint16 idDelta = toUShort((uint8*)(idDeltaOffsetPtr + segment));

		uint16 val = 0;
		if (charIdRangeOffset != 0)
		{
			val = toUShort((uint8*)(&idRangeOffset[segment] + (charIdRangeOffset / 2 + (character - charStartCode))));
		}
		else
		{
			val = idDelta + character;
		}

		// Modulo 65536 per the docs
		return val % 0xFFFF;
	}

	Glyph getGlyph(uint16 character, const FontInfo& fontInfo)
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

	int parametric(int16 p0, int16 p1, int16 p2, float t)
	{
		// Calculate the parametric equation for bezier curve using cValue as the parameter 
		float tSquared = t * t;
		float oneMinusT = (1.0f - t);
		float oneMinusTSquared = oneMinusT * oneMinusT;
		float floatResult = (oneMinusTSquared * (float)p0) + (2.0f * t * (float)p1 * oneMinusT) + (tSquared * (float)p2);
		return (int)floatResult;
	}

	int parametric(int16 p0, int16 p1, float t)
	{
		// Calculate the parametric equation for bezier curve using cValue as the parameter 
		float floatResult = (1.0f - t) * p0 + t * p1;
		return (int16)floatResult;
	}

	int max(int a, int b)
	{
		return a > b ? a : b;
	}

	int min(int a, int b)
	{
		return a < b ? a : b;
	}

	int calculateWindingNumber(int16 p0x, int16 p0y, int16 p1x, int16 p1y, int16 p2x, int16 p2y, int16 x, int16 y)
	{
		// Translate the points to x's local space
		p0x -= x;
		p0y -= y;
		p1x -= x;
		p1y -= y;
		p2x -= x;
		p2y -= y;

		float a = (float)p0y - 2.0f * (float)p1y + (float)p2y;
		float b = (float)p0y - (float)p1y;
		float c = (float)p0y;

		float squareRootOperand = max(b * b - a * c, 0);
		float squareRoot = sqrt(squareRootOperand);

		float t0 = (b - squareRoot) / a;
		float t1 = (b + squareRoot) / a;
		if (abs(a) < 0.0001f)
		{
			// If a is nearly 0, solve for a linear equation instead of a quadratic equation
			t0 = t1 = c / (2.0f * b);
		}

		uint16 magicNumber = 0x2E74;
		uint16 shiftAmount = ((p0y > 0) ? 2 : 0) + ((p1y > 0) ? 4 : 0) + ((p2y > 0) ? 8 : 0);
		uint16 shiftedMagicNumber = magicNumber >> shiftAmount;

		int cxt0 = parametric(p0x, p1x, p2x, t0);
		int cxt1 = parametric(p0x, p1x, p2x, t1);

		int result = 0;
		if ((shiftedMagicNumber & 0x1) != 0 && cxt0 >= 0)
		{
			result++;
		}
		if ((shiftedMagicNumber & 0x2) != 0 && cxt1 >= 0)
		{
			result--;
		}
		return result;
	}

	int calculateWindingNumber(int16 p0x, int16 p0y, int16 p1x, int16 p1y, int16 x, int16 y)
	{
		// Treat like bezier curve with control point in linear fashion
		int16 midx = (p0x + p1x) / 2;
		int16 midy = (p0y + p1y) / 2;
		return calculateWindingNumber(p0x, p0y, midx, midy, p1x, p1y, x, y);
	}

	int calculateWindingNumber(int numPoints, bool* finalFlags, int16* xCoords, int16* yCoords, int numContours, uint16* contourEnds, int x, int y, const Glyph& glyph)
	{
		float maxX = (float)glyph.xMax;
		float minX = (float)glyph.xMin;
		float maxY = (float)glyph.yMax;
		float minY = (float)glyph.yMin;

		float newX = (float)(x - 0) / (341.0f - 0.0f) * (maxX - minX) + minX;
		float newY = (float)(y - 0) / (341.0f - 0.0f) * (maxY - minY) + minY;
		int16 pixelX = (int16)newX;
		int16 pixelY = (int16)newY;

		// If the winding number is 0, pixel is off, otherwise pixel is on
		float windingNumber = 0;
		uint16 point = 0;
		for (uint16 i = 0; i < numContours; i++)
		{
			uint16 contourEnd = contourEnds[i];
			uint16 contourBegin = point;
			for (; point < contourEnd; point++)
			{
				// 0x01 is the flag for ON_CURVE_POINT
				// Two on points means it's a straight line
				if (finalFlags[point] && finalFlags[point + 1])
				{
					windingNumber += calculateWindingNumber(xCoords[point], yCoords[point], xCoords[point + 1], yCoords[point + 1], pixelX, pixelY);
				}
				// On point, off point, on point is a standard bezier curve
				else if (finalFlags[point] && !finalFlags[point + 1] && finalFlags[point + 2])
				{
					// Swap coords if anticlockwise
					// We enter here.
					windingNumber += calculateWindingNumber(xCoords[point], yCoords[point], xCoords[point + 1], yCoords[point + 1], xCoords[point + 2], yCoords[point + 2], pixelX, pixelY);
					point++;
				}
			}
			point++;
		}


		return windingNumber;
	}

	void drawSimpleGlyph(const Glyph& glyph, const FontInfo& fontInfo)
	{
		static uint8 flagBuffer[10000];
		static int16 xPoints[10000];
		static int16 yPoints[10000];
		static uint16 contourEnds[10];

		static uint16 ON_CURVE = 0x01;
		static uint16 X_IS_BYTE = 0x02;
		static uint16 Y_IS_BYTE = 0x04;
		static uint16 REPEAT = 0x08;
		static uint16 X_DELTA = 0x10;
		static uint16 Y_DELTA = 0x20;

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
		if (numPoints == 0)
		{
			printf("Warning 0 points...\n");
			return;
		}
		TTF_ASSERT(numPoints != 0);
		numPoints++;
		uint16 adjustedNumPoints = numPoints;

		uint8* dataPtr = instructions + instructionsLength;
		if (instructionsLength == 0) dataPtr++;
		//printf("NumInstructions: %u\n", instructionsLength);

		Buffer dataBuffer = getBuffer(dataPtr, fontInfo);
		bool onCurve = true;
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

			if (!(flag & ON_CURVE) && !onCurve)
			{
				adjustedNumPoints++;
			}

			if (!(flag & ON_CURVE))
			{
				onCurve = false;
			}
			else
			{
				onCurve = true;
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

		adjustedNumPoints += glyph.numberOfContours;
		int16* finalXCoords = (int16*)malloc(sizeof(int16) * adjustedNumPoints);
		int16* finalYCoords = (int16*)malloc(sizeof(int16) * adjustedNumPoints);
		bool* finalFlags = (bool*)malloc(sizeof(bool) * adjustedNumPoints);
		int currentIndex = 0;
		uint16 contourBegin = 0;
		uint16 c = 0;
		for (int i = 0; i < numPoints; i++)
		{
			// Two off points, generate in between on points
			if (i < numPoints - 1 && !(flagBuffer[i] & ON_CURVE) && !(flagBuffer[i + 1] & ON_CURVE))
			{
				finalXCoords[currentIndex] = xPoints[i];
				finalYCoords[currentIndex] = yPoints[i];
				finalFlags[currentIndex] = false;
				printf("i:%d  (%d, %d) %d\n", currentIndex, finalXCoords[currentIndex], finalYCoords[currentIndex], finalFlags[currentIndex]);
				currentIndex++;
				// While off curve, copy new points in
				while (i < numPoints - 1 && !(flagBuffer[i] & ON_CURVE) && !(flagBuffer[i + 1] & ON_CURVE))
				{
					finalXCoords[currentIndex] = (xPoints[i] + xPoints[i + 1]) / 2;
					finalYCoords[currentIndex] = (yPoints[i] + yPoints[i + 1]) / 2;
					finalFlags[currentIndex] = true;
					printf("i:%d  (%d, %d) %d\n", currentIndex, finalXCoords[currentIndex], finalYCoords[currentIndex], finalFlags[currentIndex]);
					currentIndex++;
					finalXCoords[currentIndex] = xPoints[i + 1];
					finalYCoords[currentIndex] = yPoints[i + 1];
					finalFlags[currentIndex] = false;
					printf("i:%d  (%d, %d) %d\n", currentIndex, finalXCoords[currentIndex], finalYCoords[currentIndex], finalFlags[currentIndex]);
					currentIndex++;
					i++;
				}
			}
			else
			{
				finalXCoords[currentIndex] = xPoints[i];
				finalYCoords[currentIndex] = yPoints[i];
				finalFlags[currentIndex] = flagBuffer[i] & ON_CURVE;
				printf("i:%d  (%d, %d) %d\n", currentIndex, finalXCoords[currentIndex], finalYCoords[currentIndex], finalFlags[currentIndex]);
				currentIndex++;
			}

			if (i >= contourEnds[c])
			{
				// If we're at the end of a contour, add one extra point to connect the last point to the first point
				finalXCoords[currentIndex] = finalXCoords[contourBegin];
				finalYCoords[currentIndex] = finalYCoords[contourBegin];
				finalFlags[currentIndex] = finalFlags[contourBegin];
				printf("Ci:%d  (%d, %d) %d\n", currentIndex, finalXCoords[currentIndex], finalYCoords[currentIndex], finalFlags[currentIndex]);
				contourEnds[c] = currentIndex;
				contourBegin = currentIndex + 1;
				currentIndex++;
				c++;
			}
		}

		float scale = 128.0f / fontInfo.unitsPerEm;
		int16 canvasWidth = 341;
		int16 canvasHeight = 341;
		printf("Adjusted points: %u\n", adjustedNumPoints);

		uint8* fileOutput = (uint8*)malloc(sizeof(uint8) * 3 * canvasWidth * canvasHeight);
		for (int y = 0; y < canvasHeight; y++)
		{
			for (int x = 0; x < canvasWidth * 3; x += 3)
			{
				int windingNumber = calculateWindingNumber(adjustedNumPoints, finalFlags, finalXCoords, finalYCoords, glyph.numberOfContours, contourEnds, x / 3, y, glyph);
				if (windingNumber == 0)
				{
					fileOutput[(y * canvasWidth * 3) + x + 0] = 0;
					fileOutput[(y * canvasWidth * 3) + x + 1] = 0;
					fileOutput[(y * canvasWidth * 3) + x + 2] = 0;
				}
				else
				{
					int16 color = abs(windingNumber) * 255.0f;
					fileOutput[(y * canvasWidth * 3) + x + 0] = color;
					fileOutput[(y * canvasWidth * 3) + x + 1] = color;
					fileOutput[(y * canvasWidth * 3) + x + 2] = color;
				}
			}
		}

		stbi_flip_vertically_on_write(1);
		if (!stbi_write_png("fontLetterA.png", canvasWidth, canvasHeight, 3, fileOutput, canvasWidth * 3))
		{
			printf("Failure!");
		}

		free(finalXCoords);
		free(finalYCoords);
		free(finalFlags);
		free(fileOutput);

		// Let's use that data now to create a new list of points

		//uint16 p = 0;
		//uint16 c = 0;
		//uint16 first = 1;
		//float scale = 64.0f / fontInfo.unitsPerEm;
		//float width = fontInfo.xMax - fontInfo.xMin;
		//float height = fontInfo.yMax - fontInfo.yMin;
		//printf("canvas.width = %2.3f;\n", width * scale);
		//printf("canvas.height = %2.3f;\n", height * scale);
		//printf("var ctx = canvas.getContext(\"2d\");\n");
		//printf("ctx.scale(%2.3f, -%2.3f);\n", scale, scale);
		//printf("ctx.translate(%2.3f, %2.3f);\n", -fontInfo.xMin, -fontInfo.yMin - height);
		//printf("ctx.beginPath();\n");
		//while (p < numPoints)
		//{
		//	int16 x = xPoints[p];
		//	int16 y = yPoints[p];
		//	if (first == 1)
		//	{
		//		printf("ctx.moveTo(%d, %d);\n", x, y);
		//		first = 0;
		//	}
		//	else
		//	{
		//		printf("ctx.lineTo(%d, %d);\n", x, y);
		//	}

		//	if (p == contourEnds[c])
		//	{
		//		c++;
		//		first = 1;
		//	}

		//	p++;
		//}
	}

	GlyphData getGlyphData(const Glyph& glyph, const FontInfo& fontInfo)
	{
		static uint16 ON_CURVE = 0x01;
		static uint16 X_IS_BYTE = 0x02;
		static uint16 Y_IS_BYTE = 0x04;
		static uint16 REPEAT = 0x08;
		static uint16 X_DELTA = 0x10;
		static uint16 Y_DELTA = 0x20;

		uint16* endPtsOfContours = (uint16*)glyph.simpleGlyphTable;
		uint16 instructionsLength = toUShort((uint8*)&endPtsOfContours[glyph.numberOfContours]);
		uint8* instructions = (uint8*)(endPtsOfContours + glyph.numberOfContours + 1);

		if (glyph.numberOfContours == 0)
		{
			return {
				0,
				nullptr,
				0,
				nullptr,
				nullptr,
				nullptr
			};
		}

		uint16* contourEnds = (uint16*)malloc(sizeof(uint16) * glyph.numberOfContours);
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
		// TODO: ADD INSTRUCTION SUPPORT?

		uint16* flagBuffer = (uint16*)malloc(sizeof(uint16) * numPoints);
		int16* xPoints = (int16*)malloc(sizeof(int16) * numPoints);
		int16* yPoints = (int16*)malloc(sizeof(int16) * numPoints);

		Buffer dataBuffer = getBuffer(dataPtr, fontInfo);
		for (uint16 i = 0; i < numPoints; i++)
		{
			uint8 flag = getUint8(dataBuffer);
			flagBuffer[i] = flag;

			if (flag & REPEAT)
			{
				uint8 repeatCount = getUint8(dataBuffer);
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

		return {
			glyph.numberOfContours,
			contourEnds,
			numPoints,
			flagBuffer,
			xPoints,
			yPoints
		};
	}

	void freeGlyphData(GlyphData& glyph)
	{
		free(glyph.contourEnds);
		free(glyph.flags);
		free(glyph.xCoords);
		free(glyph.yCoords);
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

	void initFont(FontInfo& fontInfo, char* fontData, int fontSize)
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

	bool checkCompatibility(FontInfo& fontInfo)
	{
		Buffer buffer = getBuffer((uint8*)fontInfo.data, fontInfo.fontSize);

		uint32 sfntVersion = getUint32(buffer);
		if (sfntVersion != 0x00010000)
		{
			printf("Dealing with 'OTTO'. Unable to handle this file.\n");
			return false;
		}
		return true;
	}

	uint32 writeInternalFont(FontInfo& fontInfo, const char* fontToWrite)
	{
		if (!checkCompatibility(fontInfo))
		{
			printf("Incompatible font.\n");
			return 0;
		}

		uint8* writeBufferPtr = (uint8*)malloc(sizeof(uint8) * fontInfo.fontSize);
		Buffer writeBuffer = getBuffer(writeBufferPtr, fontInfo.fontSize);
		uint32 numGlyphs = fontInfo.numGlyphs + 1;
		writeUint32(writeBuffer, numGlyphs);
		for (uint16 c = 0; c <= fontInfo.numGlyphs; c++)
		{
			writeUint32(writeBuffer, c);
		}

		for (uint16 c = 0; c <= fontInfo.numGlyphs; c++)
		{
			// Write the offset of this glyph to the appropriate index in the beginning of the file
			uint32 currentPos = (uint32)writeBuffer.cursor;
			Buffer writeBufferBegin = getBuffer(writeBufferPtr + 4, fontInfo.fontSize - 4);
			seek(writeBufferBegin, c * 4);
			writeUint32(writeBufferBegin, currentPos);

			Glyph glyph = getGlyph(c, fontInfo);

			if (glyph.simpleGlyphTable == nullptr)
			{
				writeUint16(writeBuffer, 0);
				continue;
			}

			if (glyph.numberOfContours == 0)
			{
				writeUint16(writeBuffer, 0);
				continue;
			}

			//drawGlyph(glyph, fontInfo);
			GlyphData glyphData = getGlyphData(glyph, fontInfo);
			writeUint16(writeBuffer, glyphData.numContours);

			if (c == 0x00be)
			{
				drawGlyph(glyph, fontInfo);
				printf("'%c' Addr: 0x%08x\n", c, currentPos);
				printf("'%c' has %u numContours\n", c, glyphData.numContours);
				printf("'%c' has %u numPoints\n", c, glyphData.numPoints);
				printf("'%c' xRange: (%d, %d)\n", c, glyph.xMin, glyph.xMax);
				printf("'%c' yRange: (%d, %d)\n\n", c, glyph.yMin, glyph.yMax);
				for (int i = 0; i < glyphData.numPoints; i++)
				{
					if (glyphData.flags[i] & 0x1)
						printf("i %d: ON\n", i);
					else
						printf("i %d: OFF\n", i);
					printf("flag: %u", glyphData.flags[i]);
					printf("(%d, %d)\n\n", glyphData.xCoords[i], glyphData.yCoords[i]);
				}
			}

			for (int i = 0; i < glyphData.numContours; i++)
			{
				writeUint16(writeBuffer, glyphData.contourEnds[i]);
			}

			writeUint16(writeBuffer, glyphData.numPoints);
			for (int i = 0; i < glyphData.numPoints; i++)
			{
				writeUint16(writeBuffer, glyphData.flags[i]);
			}

			for (int i = 0; i < glyphData.numPoints; i++)
			{
				writeInt16(writeBuffer, glyphData.xCoords[i]);
			}

			for (int i = 0; i < glyphData.numPoints; i++)
			{
				writeInt16(writeBuffer, glyphData.yCoords[i]);
			}

			freeGlyphData(glyphData);
		}

		// Write font file
		FILE* writeFile = fopen(fontToWrite, "wb");
		fwrite(writeBufferPtr, sizeof(uint8) * writeBuffer.cursor, 1, writeFile);
		fclose(writeFile);

		// Free write buffer
		free(writeBufferPtr);

		return writeBuffer.cursor / 4;
	}

	void parseFont(FontInfo& fontInfo)
	{
		// Doesn't really do much right now...
	}

	void freeFont(FontInfo& font)
	{
		// Free file
		free(font.data);
	}
}