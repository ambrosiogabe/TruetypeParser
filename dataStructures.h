#pragma once
#include <stdint.h>
#include <map>

#ifdef _DEBUG
#include <assert.h>
#define TTF_ASSERT(x) assert(x)
#else 
#define TTF_ASSERT(x) 
#endif

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
	const char* data;      // Pointer to .ttf file
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

struct TableInfo
{
	uint32 checksum;
	uint32 offset;
	uint32 length;
};

struct HeadTable
{
	uint16 majorVersion;
	uint16 minorVersion;
	Fixed fontRevision;
	uint32 checksumAdjustment;
	uint32 magicNumber;
	uint16 flags;
	uint16 unitsPerEm;
	LongDateTime created;
	LongDateTime modified;
	FWord xMin;
	FWord yMin;
	FWord xMax;
	FWord yMax;
	uint16 macStyle;
	uint16 lowestRecPPEM;
	int16 fontDirectionHint;
	int16 indexToLocFormat;
	int16 glyphDataFormat;
};

struct MaxpTable
{
	Fixed version;
	uint16 numGlyphs;
	uint16 maxPoints;
	uint16 maxContours;
	uint16 maxCompositePoints;
	uint16 maxCompositeContours;
	uint16 maxZones;
	uint16 maxTwilightPoints;
	uint16 maxStorage;
	uint16 maxFunctionDefs;
	uint16 maxInstructionDefs;
	uint16 maxStackElements;
	uint16 maxSizeOfInstructions;
	uint16 maxComponentElements;
	uint16 maxComponentDepth;
};

struct HheaTable
{
	uint16 majorVersion;
	uint16 minorVersion;
	FWord ascender;
	FWord descender;
	FWord lineGap;
	UFWord advanceWidthMax;
	FWord minLeftSideBearing;
	FWord minRightSideBearing;
	FWord xMaxExtent;
	int16 caretSlopeRise;
	int16 caretSlopeRun;
	FWord caretOffset;
	int32 reserved0;
	int32 reserved1;
	int16 metricDataFormat;
	uint16 numberOfHMetrics;
};

struct HMetrics
{
	uint16 advanceWidth;
	int16 leftSideBearing;
};

struct Loca
{
	bool isShort;
	union
	{
		Offset16 shortOffset;
		Offset32 longOffset;
	} offset;
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

struct EncodingRecord
{
	uint16 platformId;
	uint16 encodingId;
	Offset32 subtableOffset;
};

struct CMap
{
	uint16 version;
	uint16 numTables;
	EncodingRecord* encodingRecords;
};

struct Format4
{
	uint16 format;
	uint16 length;
	uint16 language;
	uint16 segCountX2;
	uint16 searchRange;
	uint16 entrySelector;
	uint16 rangeShift;
	uint16* endCode;
	uint16* startCode;
	int16* idDelta;
	uint16* idRangeOffsets;
	uint16* glyphIdArray;
	std::map<uint16, uint16> glyphIndexMap;
	std::map<uint16, uint16> glyphSegmentMap;
};
