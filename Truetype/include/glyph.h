#pragma once
#include <iostream>
#include <stdlib.h>
#include <stdio.h>

#include "readData.h"
#include "writeData.h"
#include "dataStructures.h"

#include "stb_write.h"

namespace Truetype
{
	uint32 findTableOffset(const char* fontData, const char* tag);

	uint32 getGlyphId(uint16 character, const FontInfo& fontInfo);

	Glyph getGlyph(uint16 character, const FontInfo& fontInfo);

	GlyphData getGlyphData(const Glyph& glyph, const FontInfo& fontInfo);

	void freeGlyphData(GlyphData& glyph);

	void drawCompositeGlyph(const Glyph& glyph);

	void drawGlyph(uint16 codepoint, const FontInfo& fontInfo, const char* fileLocation = "glyph.png");

	void initFont(FontInfo& fontInfo, char* fontData, int fontSize);

	bool checkCompatibility(FontInfo& fontInfo);

	uint32 writeInternalFont(FontInfo& fontInfo, const char* fontToWrite);

	void parseFont(FontInfo& fontInfo);

	void freeFont(FontInfo& font);
}