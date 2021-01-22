#type vertex
#version 330 core 
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec2 aTexCoords;

out vec3 fColor;
out vec2 fTexCoords;

uniform mat4 uProjection;

void main() 
{
	fColor = aColor;
	fTexCoords = aTexCoords;
	gl_Position = uProjection * vec4(aPos, 1.0);
}

#type fragment
#version 330 core 
out vec4 color;

uniform sampler2D uFont;

in vec2 fTexCoords;
in vec3 fColor;

void main()
{
	float c = texture(uFont, fTexCoords).g;
	if (c >= 0.5)
		color = vec4(0, 0, 0, 1);
	else if (c >= 0.48)
	{
		c = 1.0 - smoothstep(0.48, 0.5, c);
		color = vec4(c, c, c, 1);
	}
	else
		color = vec4(1, 1, 1, 0);
	//color = vec4(1, 1, 0, 1);
}