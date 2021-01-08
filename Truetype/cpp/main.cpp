#include <iostream>

#include "glyph.h"

int main()
{
	const char* font = "C:\\Windows\\Fonts\\arial.ttf";
	FILE* file = fopen(font, "rb");
	if (!file)
	{
		printf("Error could not open file %s", font);
		return 0;
	}

	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	rewind(file);

	char* data = (char*)malloc(sizeof(char) * size);
	if (!data)
	{
		printf("Error could not allocate enough memory");
		return 0;
	}
	// TODO: error check this, and read more if needed
	size_t readAmt = fread(data, 1, size, file);

	Truetype::FontInfo fontInfo;
	Truetype::initFont(fontInfo, data, readAmt);
	Truetype::writeInternalFont(fontInfo, "myFont.bin");
	Truetype::freeFont(fontInfo);


	return 0;
}