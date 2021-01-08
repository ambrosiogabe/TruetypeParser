#pragma once
#include <stdint.h>

#ifdef _DEBUG
#include <assert.h>
#define TTF_ASSERT(x) assert(x)
#else 
#define TTF_ASSERT(x) 
#endif

namespace Truetype
{
	typedef uint64_t uint64;
	typedef uint32_t uint32;
	typedef uint16_t uint16;
	typedef uint8_t uint8;
	typedef int32_t int32;
	typedef int16_t int16;
	typedef int8_t int8;
	typedef int32 Fixed;
	typedef int16 FWord;
	typedef uint16 UFWord;
	typedef int64_t LongDateTime;
	typedef uint32 Tag;
	typedef uint16 Offset16;
	typedef uint32 Offset32;

	struct Buffer
	{
		char* data;
		int cursor;
		int size;
	};

	struct FontInfo
	{
		void* userdata;
		char* data;            // Pointer to .ttf file
		int fontStart;         // Offset to beginning of font
		int fontSize;          // Size of data pointer

		int numGlyphs;

		int loca, head, glyf, hhea, hmtx, kern, gpos, svg; // Table locations offset from start of .ttf file
		int indexMap;          // a cmap mapping for our chosen character encoding
		int indexToLocFormat;  // Format needed to map from glyph index to glyph
		int cmap;              // Offset to cmap table that is in a version we would like to parse

		int xMin, yMin, xMax, yMax;
		int unitsPerEm;
	};

	struct Glyph
	{
		int16 numberOfContours;
		int16 xMin;
		int16 yMin;
		int16 xMax;
		int16 yMax;

		uint8* simpleGlyphTable = nullptr;
		uint8* compositeGlyphTable = nullptr;
	};

	struct GlyphData
	{
		int16 numContours;
		uint16* contourEnds;
		uint16 numPoints;
		uint16* flags;
		int16* xCoords;
		int16* yCoords;
	};
}