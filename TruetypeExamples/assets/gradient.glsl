#type vertex
#version 330 core 
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;

out vec3 fColor;
out vec2 fPos;

void main() 
{
	fColor = aColor;
	fPos = aPos.xy;
	gl_Position = vec4(aPos, 1.0);
}

#type fragment
#version 330 core 
out vec4 color;

in vec2 fPos;
in vec3 fColor;

void main()
{
	color = vec4(fColor, 1);
}