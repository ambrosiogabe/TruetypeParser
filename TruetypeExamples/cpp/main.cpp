#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "shader.h"
#include "glyph.h"

static unsigned int fontBuffer;
static unsigned int fontTextureHandle;
static unsigned int vao;

struct Vertex
{
	float x, y, z;
	float r, g, b;
	unsigned int charId;
};

static Vertex vertices[] = {
	// positions           // colors            // character code
	{0.5f, 0.5f, 0.0f,     1.0f, 0.0f, 0.0f,    'a'},   // Top right
	{0.5f, -0.5f, 0.0f,    0.0f, 1.0f, 0.0f,    'a'},   // Bottom Right
	{-0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,    'a'},   // Bottom Left
	{-0.5f, 0.5f, 0.0f,    1.0f, 1.0f, 0.0f,    'a'},   // Top Left
};

static unsigned int indices[] = {  // note that we start from 0!
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

	unsigned int vbo;
	glGenBuffers(1, &vbo);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	unsigned int ebo;
	glGenBuffers(1, &ebo);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	size_t stride = 6 * sizeof(float) + sizeof(unsigned int);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, stride, (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);
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

	printf("'a' is %u\n", 'a');

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
	unsigned int texSize = Truetype::writeInternalFont(fontInfo, "myFont.bin");
	Truetype::freeFont(fontInfo);

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

	glViewport(0, 0, 800, 600);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	uploadFontAsTexture("myFont.bin");
	initVertexAttributes();
	Shader fontShader = Shader("assets/fontShader.glsl");

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
	return 0;
}