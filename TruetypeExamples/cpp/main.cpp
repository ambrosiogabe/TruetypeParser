#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "shader.h"
#include "glyph.h"

static unsigned int fontBuffer;
static unsigned int fontTextureHandle;
static unsigned int vao;
static unsigned int vbo;
static Truetype::FontInfo fontInfo;
static unsigned int codepoint = 66;

struct Vertex
{
	float x, y, z;
	float r, g, b;
	unsigned int charId;
	int minX, minY;
	int maxX, maxY;
	float texX, texY;
};

static Vertex vertices[] = {
	//																  // Initialize these to garbage, they get patched later
	// positions           // colors            // character code     // Max Size     // Min Size           // Tex Coords
	{0.5f, 0.5f, 0.0f,     1.0f, 0.0f, 0.0f,    'a',                  0, 0,           0, 0,                 1.0f, 1.0f},   // Top right
	{0.5f, -0.5f, 0.0f,    0.0f, 1.0f, 0.0f,    'a',                  0, 0,           0, 0,                 1.0f, 0.0f},   // Bottom Right
	{-0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,    'a',                  0, 0,           0, 0,                 0.0f, 0.0f},   // Bottom Left
	{-0.5f, 0.5f, 0.0f,    1.0f, 1.0f, 0.0f,    'a',                  0, 0,           0, 0,                 0.0f, 1.0f},   // Top Left
};

static unsigned int indices[] = {
	0, 1, 3,   // first triangle
	1, 2, 3    // second triangle
};

static void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

static void uploadFontAsTexture(const char* fontFile)
{
	FILE* file = fopen(fontFile, "rb");
	if (!file)
	{
		printf("Error could not open file %s", fontFile);
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
	// TODO: error check this, and read more if needed
	size_t readAmt = fread(data, 1, size, file);

	glGenBuffers(1, &fontBuffer);
	glBindBuffer(GL_TEXTURE_BUFFER, fontBuffer);
	glBufferData(GL_TEXTURE_BUFFER, sizeof(char) * readAmt, (void*)data, GL_STATIC_READ);

	glGenTextures(1, &fontTextureHandle);
	glBindTexture(GL_TEXTURE_BUFFER, fontTextureHandle);

	glTexBuffer(GL_TEXTURE_BUFFER, GL_R16UI, fontBuffer);

	printf("Tex size: %u\n", readAmt);

	free(data);
}

static void initVertexAttributes()
{
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

	unsigned int ebo;
	glGenBuffers(1, &ebo);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	size_t stride = sizeof(Vertex);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(offsetof(Vertex, r)));
	glEnableVertexAttribArray(1);

	glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, stride, (void*)(offsetof(Vertex, charId)));
	glEnableVertexAttribArray(2);

	glVertexAttribIPointer(3, 2, GL_INT, stride, (void*)(offsetof(Vertex, minX)));
	glEnableVertexAttribArray(3);

	glVertexAttribIPointer(4, 2, GL_INT, stride, (void*)(offsetof(Vertex, maxX)));
	glEnableVertexAttribArray(4);

	glVertexAttribPointer(5, 2, GL_FLOAT, GL_FALSE, stride, (void*)(offsetof(Vertex, texX)));
	glEnableVertexAttribArray(5);
}

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	unsigned int fontMaxX = fontInfo.xMax;
	unsigned int fontMaxY = fontInfo.yMax;
	unsigned int fontMinX = fontInfo.xMin;
	unsigned int fontMinY = fontInfo.yMin;
	if (key == GLFW_KEY_UP && action == GLFW_PRESS)
	{
		//0.5f, 0.5f,
		//	0.5f, -0.5f,
		//	-0.5f, -0.5f
		//	- 0.5f, 0.5f,
		codepoint++;
		Truetype::Glyph glyph = Truetype::getGlyph(codepoint, fontInfo);
		vertices[0].x = ((float)glyph.xMax / (float)fontMaxX) * 1.0f;
		vertices[0].y = ((float)glyph.yMax / (float)fontMaxY) * 1.0f;

		vertices[1].x = ((float)glyph.xMax / (float)fontMaxX) * 1.0f;
		vertices[1].y = ((float)glyph.yMin / (float)fontMinY) * -1.0f;

		vertices[2].x = ((float)glyph.xMin / (float)fontMinX) * -1.0f;
		vertices[2].y = ((float)glyph.yMin / (float)fontMinY) * -1.0f;

		vertices[3].x = ((float)glyph.xMin / (float)fontMinX) * -1.0f;
		vertices[3].y = ((float)glyph.yMax / (float)fontMaxY) * 1.0;

		for (int i = 0; i < 4; i++)
		{
			vertices[i].charId = codepoint;
			vertices[i].maxX = glyph.xMax;
			vertices[i].maxY = glyph.yMax;
			vertices[i].minX = glyph.xMin;
			vertices[i].minY = glyph.yMin;
		}
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
	}
	else if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
	{
		codepoint--;
		Truetype::Glyph glyph = Truetype::getGlyph(codepoint, fontInfo);
		vertices[0].x = ((float)glyph.xMax / (float)fontMaxX) * 1.0f;
		vertices[0].y = ((float)glyph.yMax / (float)fontMaxY) * 1.0f;

		vertices[1].x = ((float)glyph.xMax / (float)fontMaxX) * 1.0f;
		vertices[1].y = ((float)glyph.yMin / (float)fontMinY) * -1.0f;

		vertices[2].x = ((float)glyph.xMin / (float)fontMinX) * -1.0f;
		vertices[2].y = ((float)glyph.yMin / (float)fontMinY) * -1.0f;

		vertices[3].x = ((float)glyph.xMin / (float)fontMinX) * -1.0f;
		vertices[3].y = ((float)glyph.yMax / (float)fontMaxY) * 1.0;
		for (int i = 0; i < 4; i++)
		{
			vertices[i].charId = codepoint;
			vertices[i].maxX = glyph.xMax;
			vertices[i].maxY = glyph.yMax;
			vertices[i].minX = glyph.xMin;
			vertices[i].minY = glyph.yMin;
		}
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
	}
}

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

	Truetype::initFont(fontInfo, data, readAmt);
	unsigned int texSize = Truetype::writeInternalFont(fontInfo, "myFont.bin");
	Truetype::Glyph glyph = Truetype::getGlyph(codepoint, fontInfo);
	for (int i = 0; i < 4; i++)
	{
		vertices[i].charId = codepoint;
		vertices[i].maxX = glyph.xMax;
		vertices[i].maxY = glyph.yMax;
		vertices[i].minX = glyph.xMin;
		vertices[i].minY = glyph.yMin;
	}

	// Launch glfw window and run some sample code
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(800, 600, "Font Rendering Examples", NULL, NULL);
	if (!window)
	{
		printf("Failed to create glfw window.\n");
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		printf("Failed to initialize GLAD.\n");
		return -1;
	}

	glfwSetKeyCallback(window, keyCallback);

	glViewport(0, 0, 800, 600);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	uploadFontAsTexture("myFont.bin");
	initVertexAttributes();
	Shader fontShader = Shader("assets/fontShader.glsl");

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	while (!glfwWindowShouldClose(window))
	{
		glClearColor(1.0f, 0.0f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		fontShader.Bind();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_BUFFER, fontTextureHandle);

		fontShader.UploadInt("uTextureSize", texSize / 2);
		fontShader.UploadInt("uFont", 0);

		glBindVertexArray(vao);

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		// check for errors
		GLenum error = glGetError();
		if (error != GL_NO_ERROR)
		{
			printf("Error: %d\n", error);
			break;
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	Truetype::freeFont(fontInfo);
	return 0;
}