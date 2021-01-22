#include "main.h"
#ifdef SDF_EXAMPLE
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "shader.h"
#include "glyph.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_write.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <glm/matrix.hpp>
#include <glm/gtc/matrix_transform.hpp>

static unsigned int fontTextureHandle;

struct Vertex
{
	float x, y, z;
	float r, g, b;
	float texX, texY;
};

struct CharInfo
{
	float ux0, uy0;
	float ux1, uy1;
	float advance;
	float bearingX, bearingY;
	float chScaleX, chScaleY;
};

struct SdfBitmapContainer
{
	int width, height;
	int xoff, yoff;
	float advance;
	float bearingX, bearingY;
	float chScaleX, chScaleY;
	unsigned char* bitmap;
};

static CharInfo characterMap['z' + 1];

static Vertex vertices[] = {
	// positions           // colors           // Tex Coords
	{0.5f, 0.5f, 0.0f,     0.0f, 0.0f, 0.0f,   1.0f, 1.0f},   // Top right
	{0.5f, -0.5f, 0.0f,    0.0f, 0.0f, 0.0f,   1.0f, 0.0f},   // Bottom Right
	{-0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 0.0f,   0.0f, 0.0f},   // Bottom Left
	{-0.5f, 0.5f, 0.0f,    0.0f, 0.0f, 0.0f,   0.0f, 1.0f},   // Top Left
};

static unsigned int indices[] = {
	0, 1, 3,   // first triangle
	1, 2, 3    // second triangle
};

static const int batchSize = 100;
struct Batch
{
	int size;
	unsigned int vao;
	unsigned int vbo;

	// One batch stores up to 25 quads
	Vertex vertices[batchSize];
};

static Batch batch;
static Shader fontShader;
static glm::mat4 projection = glm::ortho(0.0f, 800.0f, 0.0f, 600.0f);

static void generateEbo()
{
	int elementBuffer[batchSize * 3];

	// Generate 100 elements for 1 batch
	for (int i = 0; i < batchSize * 3; i++)
	{
		elementBuffer[i] = indices[(i % 6)] + ((i / 6) * 4);
	}

	unsigned int ebo;
	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elementBuffer), elementBuffer, GL_STATIC_DRAW);
}

static void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

static int max(int a, int b)
{
	return a > b ? a : b;
}

static int min(int a, int b)
{
	return a < b ? a : b;
}

static float map(float val, float in_min, float in_max, float out_min, float out_max)
{
	return (val - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

static int getPixel(int x, int y, unsigned char* bitmap, int width, int height)
{
	return  x < width&& y < height&& x >= 0 && y >= 0 ?
		bitmap[x + (y * width)] :
		0;
}

static int findNearestPixel(int pixX, int pixY, unsigned char* bitmap, int width, int height, int spread)
{
	int state = getPixel(pixX, pixY, bitmap, width, height);
	int minX = pixX - spread;
	int maxX = pixX + spread;
	int minY = pixY - spread;
	int maxY = pixY + spread;

	int minDistance = spread;
	for (int y = minY; y <= maxY; y++)
	{
		for (int x = minX; x <= maxX; x++)
		{
			int pixelState = getPixel(x, y, bitmap, width, height);
			int xSquared = (x - pixX) * (x - pixX);
			int ySquared = (y - pixY) * (y - pixY);
			int distance = sqrt(xSquared + ySquared);
			if (pixelState != state && distance < minDistance)
			{
				minDistance = distance;
			}
		}
	}

	if (state == 0)
	{
		return -minDistance;
	}
	return minDistance;
}

static SdfBitmapContainer generateSdfCodepointBitmap(int codepoint, FT_Face font, int fontSize, int padding = 5, int spread = 128, int upscaleResolution = 4096, bool flipVertically = false)
{
	// Render a very large sized character to generate the sdf from
	FT_Set_Pixel_Sizes(font, 0, upscaleResolution);
	if (FT_Load_Char(font, codepoint, FT_LOAD_RENDER))
	{
		printf("Could not generate GIANT '%c'.\n", codepoint);
		return {
			0, 0, 0, 0, 0, 0, 0, 0, 0, nullptr
		};
	}

	int width = font->glyph->bitmap.width;
	int height = font->glyph->bitmap.rows;
	unsigned char* img = font->glyph->bitmap.buffer;

	float widthScale = (float)width / (float)upscaleResolution;
	float heightScale = (float)height / (float)upscaleResolution;
	int characterWidth = fontSize * widthScale;
	int characterHeight = fontSize * heightScale;
	int bitmapWidth = characterWidth + padding * 2;
	int bitmapHeight = characterHeight + padding * 2;
	float scaleX = (float)width / (float)characterWidth;
	float scaleY = (float)height / (float)characterHeight;
	unsigned char* sdfBitmap = (unsigned char*)malloc(sizeof(unsigned char) * bitmapHeight * bitmapWidth);
	for (int y = -padding; y < bitmapHeight - padding; y++)
	{
		for (int x = -padding; x < bitmapWidth - padding; x++)
		{
			int pixelX = map(x, -padding, characterWidth + padding, -padding * scaleX, (characterWidth + padding) * scaleX);
			int pixelY = map(characterHeight - y, -padding, characterHeight + padding, -padding * scaleY, (characterHeight + padding) * scaleY);
			int sdf = findNearestPixel(pixelX, pixelY, img, width, height, spread);
			float val = (float)sdf / (float)spread;
			val = (val + 1.0f) / 2.0f;
			if (!flipVertically)
			{
				sdfBitmap[(x + padding) + ((y + padding) * bitmapWidth)] = (int)(val * 255.0f);
			}
			else
			{
				sdfBitmap[(x + padding) + ((bitmapHeight - (y + padding) - 1) * bitmapWidth)] = (int)(val * 255.0f);
			}
		}
	}
	
	FT_Set_Pixel_Sizes(font, 0, 64.0f);
	FT_Load_Char(font, codepoint, FT_LOAD_RENDER);
	return {
		bitmapWidth, bitmapHeight,
		padding, padding,
		(float)(font->glyph->metrics.horiAdvance >> 6) / 64.0f,
		(float)(font->glyph->metrics.horiBearingX >> 6) / (float)64.0f,
		(float)(font->glyph->metrics.horiBearingY >> 6) / (float)64.0f,
		(float)(font->glyph->metrics.width >> 6) / (float)64.0f, 
		(float)(font->glyph->metrics.height >> 6) / (float)64.0f,
		sdfBitmap
	};
}

static void uploadFontAsTexture(const char* fontFile)
{
	FT_Library ft;
	if (FT_Init_FreeType(&ft))
	{
		printf("Could not initialize freetype.\n");
		return;
	}

	FT_Face font;
	if (FT_New_Face(ft, fontFile, 0, &font))
	{
		printf("Could not load font %s.\n", fontFile);
		return;
	}

	int lowResFontSize = 26;
	FT_Set_Pixel_Sizes(font, 0, lowResFontSize);
	if (FT_Load_Char(font, 'M', FT_LOAD_RENDER))
	{
		printf("Failed to load glyph.\n");
		return;
	}
	// Estimate a "squarish" width for the font texture, and leave it at that
	int emWidth = font->glyph->bitmap.width;
	int squareLength = sqrt('z');
	int fixedWidth = squareLength * emWidth;

	// Calculate the dimensions of the actual width and height of the font texture
	// and save all the sdf metrics for each codepoint
	int sdfHeight = 0;
	int sdfWidth = 0;
	int rowHeight = 0;
	int x = 0;
	int y = 0;

	SdfBitmapContainer sdfBitmaps['z' + 1];
	for (int codepoint = 0; codepoint <= 'z'; codepoint++)
	{
		if (FT_Load_Char(font, codepoint, FT_LOAD_RENDER))
		{
			sdfBitmaps[codepoint] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, nullptr };
			printf("Failed to load glyph.\n");
			continue;
		}

		SdfBitmapContainer container = generateSdfCodepointBitmap(codepoint, font, lowResFontSize, 5, 24, 1024);
		sdfBitmaps[codepoint] = container;
		int width = container.width;
		int height = container.height;

		rowHeight = max(height, rowHeight);
		if (x + width >= fixedWidth)
		{
			x = 0;
			y += rowHeight;
			rowHeight = height;
		}

		characterMap[codepoint] = {
			(float)x + container.xoff,
			(float)y + container.yoff,
			(float)(x + width - container.xoff),
			(float)(y + height - container.yoff),
			container.advance,
			container.bearingX,
			container.bearingY,
			container.chScaleX,
			container.chScaleY
		};

		sdfWidth = max(sdfWidth, x + width);
		sdfHeight = max(sdfHeight, y + rowHeight);
		x += width;
	}

	// Write all the image data to the final sdf
	int bitmapLength = sdfWidth * sdfHeight * 4;
	unsigned char* finalSdf = (unsigned char*)malloc(sizeof(unsigned char*) * bitmapLength);
	memset(finalSdf, 0, bitmapLength);
	int endBitmap = bitmapLength + 1;

	for (int codepoint = 0; codepoint <= 'z'; codepoint++)
	{
		SdfBitmapContainer sdf = sdfBitmaps[codepoint];
		int x0 = characterMap[codepoint].ux0;
		int y0 = characterMap[codepoint].uy0;
		int x1 = characterMap[codepoint].ux1;
		int y1 = characterMap[codepoint].uy1;

		if (!sdf.bitmap) continue;

		// Texture biases give a little wiggle room for sampling the textures, consider adding these as a parameter
		float bottomLeftTextureBias = -0.1f;
		float topRightTextureBias = 1;
		characterMap[codepoint] = {
			(float)(x0 + bottomLeftTextureBias) / (float)sdfWidth,
			(float)(y0 + bottomLeftTextureBias) / (float)sdfHeight,
			(float)(x1 + topRightTextureBias) / (float)sdfWidth,
			(float)(y1 + topRightTextureBias) / (float)sdfHeight,
			sdf.advance,
			sdf.bearingX,
			sdf.bearingY,
			sdf.chScaleX,
			sdf.chScaleY
		};

		int width = sdf.width;
		int height = sdf.height;
		int xoff = sdf.xoff;
		int yoff = sdf.yoff;
		x = x0 - xoff;
		y = y0 - yoff;
		for (int imgY = 0; imgY < height; imgY++)
		{
			for (int imgX = 0; imgX < width; imgX++)
			{
				unsigned char pixelData = sdf.bitmap[imgX + imgY * width];
				int index = (x + imgX) * 4 + (y + imgY) * sdfWidth * 4;
				TTF_ASSERT(index + 3 < endBitmap);
				finalSdf[index] = pixelData;
				finalSdf[index + 1] = pixelData;
				finalSdf[index + 2] = pixelData;
				finalSdf[index + 3] = pixelData;
			}
		}

		free(sdf.bitmap);
	}

	printf("Writing png for font\n");
	stbi_write_png("tmp.png", sdfWidth, sdfHeight, 4, finalSdf, sdfWidth * 4);

	glGenTextures(1, &fontTextureHandle);
	glBindTexture(GL_TEXTURE_2D, fontTextureHandle);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sdfWidth, sdfHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, finalSdf);

	free(finalSdf);
	FT_Done_Face(font);
	FT_Done_FreeType(ft);
}

static void initBatch()
{
	glGenVertexArrays(1, &batch.vao);
	glBindVertexArray(batch.vao);

	glGenBuffers(1, &batch.vbo);

	glBindBuffer(GL_ARRAY_BUFFER, batch.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * batchSize, nullptr, GL_DYNAMIC_DRAW);

	// Does this even work? I guess we'll find out
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batchEbo);
	generateEbo();

	size_t stride = sizeof(Vertex);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(offsetof(Vertex, r)));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(offsetof(Vertex, texX)));
	glEnableVertexAttribArray(2);
	glBindVertexArray(0);
}

static void flushBatch()
{
	// Clear the buffer and upload the contents of the current buffer to the GPU
	glBindBuffer(GL_ARRAY_BUFFER, batch.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * batch.size, nullptr, GL_DYNAMIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex) * batch.size, batch.vertices);

	// Draw the font data
	fontShader.Bind();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, fontTextureHandle);
	fontShader.UploadInt("uFont", 0);
	fontShader.UploadMat4("uProjection", projection);

	// Flush it to the screen so the next batch can be rendered
	glBindVertexArray(batch.vao);
	glDrawElements(GL_TRIANGLES, batch.size * 6, GL_UNSIGNED_INT, 0);

	batch.size = 0;
}

static void addCharacter(int x, int y, int size, CharInfo& charInfo, int rgb)
{
	// If we have no more room in the current batch flush it and start with a fresh batch
	if (batch.size >= batchSize - 4)
	{
		flushBatch();
	}

	// We need to add 4 vertices
	//{0.5f, 0.5f, 0.0f,     0.0f, 0.0f, 0.0f,   1.0f, 1.0f},
	//{0.5f, -0.5f, 0.0f,    0.0f, 0.0f, 0.0f,   1.0f, 0.0f},
	//{-0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 0.0f,   0.0f, 0.0f},
	//{-0.5f, 0.5f, 0.0f,    0.0f, 0.0f, 0.0f,   0.0f, 1.0f},
	float r = (float)((rgb >> 16) & 0XFF) / 255.0f;
	float g = (float)((rgb >> 8) & 0XFF) / 255.0f;
	float b = (float)((rgb >> 0) & 0xFF) / 255.0f;

	float x0 = x;
	float y0 = y;
	float x1 = x + size * charInfo.chScaleX;
	float y1 = y + size * charInfo.chScaleY;

	batch.vertices[batch.size] = {
		x1, y1, 0.0f,
		r, g, b,
		charInfo.ux1, charInfo.uy1
	};
	batch.vertices[batch.size + 1] = {
		x1, y0, 0.0f,
		r, g, b,
		charInfo.ux1, charInfo.uy0
	};
	batch.vertices[batch.size + 2] = {
		x0, y0, 0.0f,
		r, g, b,
		charInfo.ux0, charInfo.uy0
	};
	batch.vertices[batch.size + 3] = {
		x0, y1, 0.0f,
		r, g, b,
		charInfo.ux0, charInfo.uy1
	};
	batch.size += 4;
}

static void addText(const char* string, int x, int y, int size, int rgb)
{
	int strLength = strlen(string);
	for (int i = 0; i < strLength; i++)
	{
		char c = string[i];

		CharInfo& charInfo = characterMap[c];
		float xPos = x + charInfo.bearingX * size;
		float yPos = y - (charInfo.chScaleY - charInfo.bearingY) * size;
		addCharacter(xPos, yPos, size, charInfo, rgb);
		x += charInfo.advance * size;
	}
}

static void GLAPIENTRY MessageCallback(GLenum source,
										GLenum type,
										GLuint id,
										GLenum severity,
										GLsizei length,
										const GLchar* message,
										const void* userParam)
{
	if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
	{
		return;
	}

	printf("---------------------opengl-callback-start------------\n");
	printf("message: %s\n", message);
	printf("type: ");
	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:
		printf("ERROR\n");
		break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		printf("DEPRECATED_BEHAVIOR\n");
		break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		printf("UNDEFINED_BEHAVIOR\n");
		break;
	case GL_DEBUG_TYPE_PORTABILITY:
		printf("PORTABILITY\n");
		break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		printf("PERFORMANCE\n");
		break;
	case GL_DEBUG_TYPE_OTHER:
		printf("OTHER\n");
		break;
	}

	printf("id: %u\n", id);
	printf("severity: ");
	switch (severity)
	{
	case GL_DEBUG_SEVERITY_LOW:
		printf("LOW\n");
		break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		printf("MEDIUM\n");
		break;
	case GL_DEBUG_SEVERITY_HIGH:
		printf("HIGH\n");
		break;
	case GL_DEBUG_SEVERITY_NOTIFICATION:
		printf("NOTIFICATION\n");
		break;
	default:
		printf("Unkown\n");
		break;
	}
	printf("---------------------opengl-callback-end--------------\n");
}

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	// Nothing for now, maybe add camera controls?
}

static GLFWwindow* init()
{
	// Launch glfw window and run some sample code
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(1920 / 2, 1080 / 2, "SDF Font Rendering Example", NULL, NULL);
	if (!window)
	{
		printf("Failed to create glfw window.\n");
		glfwTerminate();
		return nullptr;
	}

	glfwMakeContextCurrent(window);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		printf("Failed to initialize GLAD.\n");
		return nullptr;
	}

	// During init, enable debug output
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(MessageCallback, 0);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glfwSetKeyCallback(window, keyCallback);

	glViewport(0, 0, 1920 / 2, 1080 / 2);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	uploadFontAsTexture("C:/Windows/Fonts/Arial.ttf");
	initBatch();

	return window;
}

void runGlWindow()
{
	GLFWwindow* window = init();
	fontShader = Shader("assets/sdfShader.glsl");

	int counter = 0;
	int numberOfAs = 1;
	while (!glfwWindowShouldClose(window))
	{
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		addText("Hello world!", 200, 200, 32, 0x00FF00);

		counter++;
		if (counter % 10 == 0)
		{
			numberOfAs++;
		}

		int x = 0;
		int y = 600 - 32;
		for (int i = 0; i < numberOfAs; i++)
		{
			if (x >= 700)
			{
				x = 0;
				y -= 32;
			}
			addText("a", x, y, 32, 0X0000FF);
			x += 32;
		}

		// If there are any lingering batches, this will draw it
		flushBatch();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
}

int main()
{

	runGlWindow();
	return 0;
}
#endif