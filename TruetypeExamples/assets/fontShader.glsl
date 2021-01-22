#type vertex
#version 330 core 
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in uint aCharacterId;
layout(location = 3) in ivec2 aCharMin;
layout(location = 4) in ivec2 aCharMax;
layout(location = 5) in vec2 aTexCoord;

out vec3 fColor;
out vec2 fPos;
out vec2 fTexCoord;
flat out ivec2 fCharMin;
flat out ivec2 fCharMax;
flat out uint fCharacterId;

uniform usamplerBuffer uFont;

flat out uint codepoint;
flat out int glyphOffset;
flat out uint numContours;
flat out uint numPoints;
flat out int contourTableOffset;
flat out int flagTableOffset;
flat out int xTableOffset;
flat out int yTableOffset;

void main() 
{
	fColor = aColor;
	fCharacterId = aCharacterId;
	fPos = aPos.xy;
	fCharMin = aCharMin;
	fCharMax = aCharMax;
	fTexCoord = aTexCoord;
	gl_Position = vec4(aPos, 1.0);

	// Divide by 2 since offset is in bytes, but we are indexing as shorts (8 bit vs 16 bit)
	// 66 is the codepoint for the character 'A' + 1
	codepoint = uint(fCharacterId)+1U;
	uint glyphOffsetU = (texelFetch(uFont, int(codepoint * 2U)).r | (texelFetch(uFont, int(codepoint * 2U + 1U)).r << 8)) >> 1;
	glyphOffset = int(glyphOffsetU);
	numContours = texelFetch(uFont, glyphOffset).r;
	numPoints = texelFetch(uFont, glyphOffset + int(numContours) + 1).r;
	contourTableOffset = glyphOffset + 1;
	flagTableOffset = contourTableOffset + int(numContours) + 1;
	xTableOffset = flagTableOffset + int(numPoints);
	yTableOffset = xTableOffset + int(numPoints);
}

#type fragment
#version 330 core 
out vec4 color;

in vec2 fPos;
in vec3 fColor;
in vec2 fTexCoord;
flat in uint fCharacterId;
flat in ivec2 fCharMin;
flat in ivec2 fCharMax;

flat in uint codepoint;
flat in int glyphOffset;
flat in uint numContours;
flat in uint numPoints;
flat in int contourTableOffset;
flat in int flagTableOffset;
flat in int xTableOffset;
flat in int yTableOffset;

uniform uint numGlyphs;
uniform usamplerBuffer uFont;

void parametric(in int p0, in int p1, in int p2, in float t, inout int result)
{
	// Calculate the parametric equation for bezier curve using cValue as the parameter 
	float tSquared = t * t;
	float oneMinusT = (1.0 - t);
	float oneMinusTSquared = oneMinusT * oneMinusT;
	float floatResult = (oneMinusTSquared * float(p0)) + (2.0 * t * float(p1) * oneMinusT) + (tSquared * float(p2));
	result = int(floatResult);
}

void calculateWindingNumber(in ivec2 p0, in ivec2 p1, in ivec2 p2, in ivec2 pixelCoords, inout float windingNumber)
{
		// Translate the points to x's local space
		p0 -= pixelCoords;
		p1 -= pixelCoords;
		p2 -= pixelCoords;

		float a = float(p0.y) - 2.0 * float(p1.y) + float(p2.y);
		float b = float(p0.y) - float(p1.y);
		float c = float(p0.y);

		float squareRootOperand = max(b * b - a * c, 0);
		float squareRoot = sqrt(squareRootOperand);

		float t0 = (b - squareRoot) / a;
		float t1 = (b + squareRoot) / a;
		if (abs(a) < 0.0001)
		{
			// If a is nearly 0, solve for a linear equation instead of a quadratic equation
			t0 = t1 = c / (2.0 * b);
		}

		uint magicNumber = 0x2E74U;
		uint shiftAmount = ((p0.y > 0) ? 2U : 0U) + ((p1.y > 0) ? 4U : 0U) + ((p2.y > 0) ? 8U : 0U);
		uint shiftedMagicNumber = magicNumber >> shiftAmount;

		int cxt0 = -1;
		parametric(p0.x, p1.x, p2.x, t0, cxt0);
		int cxt1 = -1;
		parametric(p0.x, p1.x, p2.x, t1, cxt1);

		if ((shiftedMagicNumber & 1U) != 0U && cxt0 >= 0)
		{
			windingNumber += clamp(0.08 * cxt0 + 0.5, 0, 1);
		}
		if ((shiftedMagicNumber & 2U) != 0U && cxt1 >= 0)
		{
			windingNumber -= clamp(0.08 * cxt1 + 0.5, 0, 1);
		}
}

void calculateWindingNumber(in ivec2 p0, in ivec2 p1, in ivec2 pixelCoords, inout float windingNumber)
{
	// Treat like bezier curve with control point in linear fashion
	calculateWindingNumber(p0, (p0 + p1) / 2, p1, pixelCoords, windingNumber);
}

void convert(in uint val, inout int res)
{
	res = int(val);
	if ((val & uint(0x8000)) != uint(0)) { res = -int(uint(0xFFFF) - val) - 1; }
}

void main() 
{	
	float maxX = float(fCharMax.x);
	float maxY = float(fCharMax.y);
	float minX = float(fCharMin.x);
	float minY = float(fCharMin.y);

	float newX = fTexCoord.x * (maxX - minX) + minX;
	float newY = fTexCoord.y * (maxY - minY) + minY;
	ivec2 pixelCoords = ivec2(int(newX), int(newY));

	// If the winding number is 0, pixel is off, otherwise pixel is on
	float windingNumber = 0.0;
	uint point = 0U;
	for (uint i = 0U; i < numContours; i++)
	{
		uint contourEnd = texelFetch(uFont, contourTableOffset + int(i)).r;
		for (; point < contourEnd; point++)
		{
			bool flag1 = texelFetch(uFont, flagTableOffset + int(point + 0U)).r == 1U;
			bool flag2 = texelFetch(uFont, flagTableOffset + int(point + 1U)).r == 1U;
			bool flag3 = texelFetch(uFont, flagTableOffset + int(point + 2U)).r == 1U;

			int x1 = 0;
			convert(texelFetch(uFont, xTableOffset + int(point) + 0).r, x1);
			int x2 = 0;
			convert(texelFetch(uFont, xTableOffset + int(point) + 1).r, x2);
			int x3 = 0;
			convert(texelFetch(uFont, xTableOffset + int(point) + 2).r, x3);

			int y1 = 0;
			convert(texelFetch(uFont, yTableOffset + int(point) + 0).r, y1);
			int y2 = 0;
			convert(texelFetch(uFont, yTableOffset + int(point) + 1).r, y2);
			int y3 = 0;
			convert(texelFetch(uFont, yTableOffset + int(point) + 2).r, y3);

			ivec2 p1 = ivec2(x1, y1);
			ivec2 p2 = ivec2(x2, y2);
			ivec2 p3 = ivec2(x3, y3);

			// 0x01 is the flag for ON_CURVE_POINT
			// Two on points means it's a straight line
			if (flag1 && flag2)
			{
				if ((pixelCoords.x <= p1.x || pixelCoords.x <= p2.x) && pixelCoords.y >= min(p1.y, p2.y) && pixelCoords.y <= max(p1.y, p2.y))
				{
					calculateWindingNumber(p1, p2, pixelCoords, windingNumber);
				}
			}
			// On point, off point, on point is a standard bezier curve
			else if (flag1 && !flag2 && flag3)
			{
				if ((pixelCoords.x <= p1.x || pixelCoords.x <= p3.x) && pixelCoords.y >= min(p1.y, p3.y) && pixelCoords.y <= max(p1.y, p3.y))
				{
					calculateWindingNumber(p1, p2, p3, pixelCoords, windingNumber);
				}
				point++;
			}
		}
		point++;
	}

	// Ternary or clamp? Which is faster?
	//float c = min(max(abs(windingNumber), 0), 1);
	float c = windingNumber;
	color = vec4(fColor, 1) * vec4(c, c, c, c);
}