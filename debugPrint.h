#pragma once

#include "dataStructures.h"

void printTag(Tag tag)
{
	char tagString[5] = { (tag & 0xFF000000) >> 24, (tag & 0x00FF0000) >> 16, (tag & 0x0000FF00) >> 8, (tag & 0x000000FF), '\0' };
	printf("%s", tagString);
}

void printTableInfo(TableInfo& tableInfo, Tag tag)
{
	printTag(tag);
	printf("\n");
	printf("\tchecksum: %u\n", tableInfo.checksum);
	printf("\toffset: %u\n", tableInfo.offset);
	printf("\tlength: %u\n", tableInfo.length);
}

void printHeadTable(HeadTable& headTable)
{
	printf("Major Version: %u\n", headTable.majorVersion);
	printf("Minor Version: %u\n", headTable.minorVersion);
	printf("Font Revision: %d\n", headTable.fontRevision);
	printf("Checksum Adjustment: %u\n", headTable.checksumAdjustment);
	printf("Magic Number: 0x%08x\n", headTable.magicNumber);
	printf("Flags: 0x%08x\n", headTable.flags);
	printf("Units Per Em: %u\n", headTable.unitsPerEm);
	printf("Created: %u\n", headTable.created);
	printf("Modified: %u\n", headTable.modified);
	printf("XMin: %d\n", headTable.xMin);
	printf("YMin: %d\n", headTable.yMin);
	printf("XMax: %d\n", headTable.xMax);
	printf("YMax: %d\n", headTable.yMax);
	printf("Mac Style: %u\n", headTable.macStyle);
	printf("Lowest Rec PPEM: %u\n", headTable.lowestRecPPEM);
	printf("Font Direction Hint: %d\n", headTable.fontDirectionHint);
	printf("Index To Local Format: %d\n", headTable.indexToLocFormat);
	printf("Glyph Data Format: %d\n", headTable.glyphDataFormat);
}

void printMaxpTable(MaxpTable& maxpTable)
{
	printf("Version: %d\n", maxpTable.version);
	printf("Num Glyphs: %u\n", maxpTable.numGlyphs);
	printf("Max Points: %u\n", maxpTable.maxPoints);
	printf("Max Contours: %u\n", maxpTable.maxContours);
	printf("Max Composite Points: %u\n", maxpTable.maxCompositePoints);
	printf("Max Composite Contours: %u\n", maxpTable.maxCompositeContours);
	printf("Max Zones: %u\n", maxpTable.maxZones);
	printf("Max Twighlight Points: %u\n", maxpTable.maxTwilightPoints);
	printf("Max Storage: %u\n", maxpTable.maxStorage);
	printf("Max Function Defs: %u\n", maxpTable.maxFunctionDefs);
	printf("Max Instruction Defs: %u\n", maxpTable.maxInstructionDefs);
	printf("Max Stack Elements: %u\n", maxpTable.maxStackElements);
	printf("Max Size Of Instructions: %u\n", maxpTable.maxSizeOfInstructions);
	printf("Max Component Elements: %u\n", maxpTable.maxComponentElements);
	printf("Max Component Depth: %u\n", maxpTable.maxComponentDepth);
}

void printHheaTable(HheaTable& hheaTable)
{
	printf("Major Version: %u\n", hheaTable.majorVersion);
	printf("Minor Version: %u\n", hheaTable.minorVersion);
	printf("Ascender: %d\n", hheaTable.ascender);
	printf("Descender: %d\n", hheaTable.descender);
	printf("Line Gap: %d\n", hheaTable.lineGap);
	printf("Advance Width Max: %u\n", hheaTable.advanceWidthMax);
	printf("Min Left Side Bearing: %d\n", hheaTable.minLeftSideBearing);
	printf("Min Right Side Bearing: %d\n", hheaTable.minRightSideBearing);
	printf("X Max Extent: %d\n", hheaTable.xMaxExtent);
	printf("Caret Slope Rise: %d\n", hheaTable.caretSlopeRise);
	printf("Caret Slope Run: %d\n", hheaTable.caretSlopeRun);
	printf("Caret Offset: %d\n", hheaTable.caretOffset);
	printf("Reserved0: %d\n", hheaTable.reserved0);
	printf("Reserved1: %d\n", hheaTable.reserved1);
	printf("Metric Data Format: %d\n", hheaTable.metricDataFormat);
	printf("Number of HMetrics: %u\n", hheaTable.numberOfHMetrics);
}

void printHMetrics(HMetrics& hMetrics)
{
	printf("Advance Width: %u\n", hMetrics.advanceWidth);
	printf("Left Side Bearing: %d\n", hMetrics.leftSideBearing);
}

void printLoca(Loca& loca)
{
	printf("Is Short: %d\n", loca.isShort);
	printf("Data: %u\n", loca.offset);
}

void printGlyph(Glyph& glyph)
{
	printf("Number of Contours: %d\n", glyph.numberOfContours);
	printf("xMin: %d\n", glyph.xMin);
	printf("xMax: %d\n", glyph.xMax);
	printf("yMin: %d\n", glyph.yMin);
	printf("yMax: %d\n", glyph.yMax);
}
