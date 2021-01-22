#include "glyph.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

struct File
{
	size_t size;
	char* data;
};

File readFile(const char* filename)
{
	FILE* file = fopen(filename, "rb");
	if (!file)
	{
		printf("Error could not open file %s", filename);
		return {0, nullptr};
	}

	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	rewind(file);

	char* data = (char*)malloc(sizeof(char) * size);
	if (!data)
	{
		printf("Error could not allocate enough memory");
		return {0, nullptr};
	}
	size_t readAmt = fread(data, 1, size, file);
	TTF_ASSERT(readAmt == size);

	return { size, data };
}

void testFontInfoMatch(stbtt_fontinfo& stbttFont, Truetype::FontInfo& myFont, const char* fontName) 
{
	// Test glyph information
	TTF_ASSERT(stbttFont.numGlyphs == myFont.numGlyphs);

	// Test table locations
	TTF_ASSERT(stbttFont.loca == myFont.loca);
	TTF_ASSERT(stbttFont.glyf == myFont.glyf);
	TTF_ASSERT(stbttFont.head == myFont.head);
	TTF_ASSERT(stbttFont.hhea == myFont.hhea);
	TTF_ASSERT(stbttFont.hmtx == myFont.hmtx);
	//TTF_ASSERT(stbttFont.kern == myFont.kern);
	//TTF_ASSERT(stbttFont.gpos == myFont.gpos);

	// Test index format
	TTF_ASSERT(stbttFont.indexToLocFormat == myFont.indexToLocFormat);

	// Test font bounds
	int x0, y0, x1, y1;
	stbtt_GetFontBoundingBox(&stbttFont, &x0, &y0, &x1, &y1);
	TTF_ASSERT(x0 == myFont.xMin);
	TTF_ASSERT(x1 == myFont.xMax);
	TTF_ASSERT(y0 == myFont.yMin);
	TTF_ASSERT(y1 == myFont.yMax);

	printf("Font info matches for '%s'\n", fontName);
}

void testFontGlyphIdsMatch(stbtt_fontinfo& stbttFont, Truetype::FontInfo& myFont, const char* fontName)
{
	for (int i = 0; i < myFont.numGlyphs; i++)
	{
		int myGlyphId = Truetype::getGlyphId(i, myFont);
		int stbGlyphId = stbtt_FindGlyphIndex(&stbttFont, i);
		TTF_ASSERT(myGlyphId == stbGlyphId);
	}

	printf("Glyph Ids all match for '%s'\n", fontName);
}

int main()
{
	const char* fontNames[] = {
		"C:/Windows/Fonts/Alger.ttf",
		"C:/Windows/Fonts/Arial.ttf",
		"C:/Windows/Fonts/ARLRDBD.TTF",
		"C:/Windows/Fonts/BowlbyOneSC-Regular.ttf",
		"C:/Windows/Fonts/BKANT.TTF",
		"C:/Windows/Fonts/SNAP____.TTF",
		"C:/Windows/Fonts/STENCIL.TTF"
	};
	int fontTestSize = 7;

	for (int fontIndex=0; fontIndex < fontTestSize; fontIndex++)
	{
		File fontData = readFile(fontNames[fontIndex]);
		if (!fontData.data)
		{
			return -1;
		}

		stbtt_fontinfo font;
		stbtt_InitFont(&font, (unsigned char*)fontData.data, stbtt_GetFontOffsetForIndex((unsigned char*)fontData.data, 0));

		Truetype::FontInfo fontInfo;
		Truetype::initFont(fontInfo, fontData.data, fontData.size);

		testFontInfoMatch(font, fontInfo, fontNames[fontIndex]);
		testFontGlyphIdsMatch(font, fontInfo, fontNames[fontIndex]);
		printf("\n");

		Truetype::freeFont(fontInfo);

	}

	printf("All tests passed.\n");
}