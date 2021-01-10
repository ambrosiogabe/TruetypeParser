#type vertex
#version 330 core 
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in uint aCharacterId;

out vec3 fColor;
out vec2 fPos;
flat out uint fCharacterId;

void main() 
{
	fColor = aColor;
	fCharacterId = aCharacterId;
	fPos = aPos.xy;
	gl_Position = vec4(aPos, 1.0);
}

#type fragment
#version 330 core 
out vec4 color;

in vec2 fPos;
in vec3 fColor;
flat in uint fCharacterId;

uniform uint uTextureSize;
uniform usamplerBuffer uFont;

void parametric(in int p0, in int p1, in int p2, in float t, inout int result)
{
	// Calculate the parametric equation for bezier curve using cValue as the parameter 
	float floatResult = ((1 - t) * (1 - t)) * float(p0) + 2 * t * float(p1) * (1 - t) + (t * t) * float(p2);
	result = int(floatResult);
}

void getIntersection(in ivec2 p0, in ivec2 p1, in ivec2 p2, in float t, inout int windingNumber)
{
	float midpoint = t / 2.0;
	if (midpoint < 0.0)
	{
		midpoint = (1 - t) / 2.0;

		int y = 0;
		parametric(p0.x, p1.x, p2.x, t, y);

		if (y < 0) { windingNumber++; }
		else if (y > 0) { windingNumber--; }
	}
	else if (midpoint < 1)
	{
		int y = 0;
		parametric(p0.x, p1.x, p2.x, t, y);
		
		if (y < 0) { windingNumber++; }
		else if (y > 0) { windingNumber--; }
	}
}

void calculateWindingNumber(in ivec2 p0, in ivec2 p1, in ivec2 p2, in ivec2 x, inout int windingNumber)
{
	// Translate the points to x's local space
	p0 -= x;
	p1 -= x;
	p2 -= x;

	float a = float(p0.y) - 2.0 * float(p1.y) + float(p2.y);
	float b = float(p0.y) - float(p1.y);
	float c = float(p0.y);
	
	float squareRootOperand = b * b - a * c;
	if (squareRootOperand < 0)
	{
		//windingNumber += 10;
		return;
	}

	float squareRoot = sqrt(squareRootOperand);
	float t0 = (b - squareRoot) / a;
	float t1 = (b + squareRoot) / a;

	if (t0 >= 0 && t0 < 1 && !(t1 >= 0 && t1 < 1))
	{
		// T0 is the only root and we have intersected 
		getIntersection(p0, p1, p2, t0, windingNumber);
	}
	else if (t1 >= 0 && t1 < 1 && !(t0 >= 0 && t0 < 1))
	{
		// T1 is the only root and we have intersected 
		getIntersection(p0, p1, p2, t1, windingNumber);
	}
	else if (t0 >= 0 && t0 < 1 && t1 >= 0 && t1 < 1)
	{
		// T0 and T1 are roots and have intersected
		getIntersection(p0, p1, p2, t0, windingNumber);
		getIntersection(p0, p1, p2, t1, windingNumber);
	}
}

void main() 
{	
	float newX = (fPos.x - (-0.5)) / (0.5 - (-0.5)) * (1052 - 74) + 74;
	float newY = (fPos.y - (-0.5)) / (0.5 - (-0.5)) * (1080 - (-24)) + (-24);
	ivec2 pixelCoords = ivec2(int(newX), int(newY));

	uint numGlyphs = texelFetch(uFont, 0).r | (texelFetch(uFont, 1).r << 8);
	// Divide by 2 since offset is in bytes, but we are indexing as shorts (8 bit vs 16 bit)
	// 98 is the codepoint for the character 'a' + 1
	uint aOffsetUint = (texelFetch(uFont, 98 * 2).r | (texelFetch(uFont, 98 * 2 + 1).r << 8)) >> 1;
	int aOffset = int(aOffsetUint);
	uint numContours = texelFetch(uFont, aOffset).r;
	uint numPoints = texelFetch(uFont, aOffset + int(numContours) + 1).r;
	int flagTableOffset = aOffset + int(numContours) + 2;
	int xTableOffset = flagTableOffset + int(numPoints);
	int yTableOffset = xTableOffset + int(numPoints);

	// If the winding number is 0, pixel is off, otherwise pixel is on
	int windingNumber = int(0);
	ivec2 previousPoint = ivec2(0, 0);
	for (uint i=uint(0); i < numPoints; i++) 
	{
		uint flag1 = texelFetch(uFont, flagTableOffset + int(i)).r;
		uint flag2 = texelFetch(uFont, flagTableOffset + int(i) + 1).r;
		uint flag3 = texelFetch(uFont, flagTableOffset + int(i) + 2).r;
		uint flag4 = texelFetch(uFont, flagTableOffset + int(i) + 3).r;

		int x1 = int( texelFetch(uFont, xTableOffset + int(i)).r );
		int x2 = int( texelFetch(uFont, xTableOffset + int(i) + 1).r );
		int x3 = int( texelFetch(uFont, xTableOffset + int(i) + 2).r );
		int x4 = int( texelFetch(uFont, xTableOffset + int(i) + 3).r );

		int y1 = int( texelFetch(uFont, yTableOffset + int(i)).r );
		int y2 = int( texelFetch(uFont, yTableOffset + int(i) + 1).r );
		int y3 = int( texelFetch(uFont, yTableOffset + int(i) + 2).r );
		int y4 = int( texelFetch(uFont, yTableOffset + int(i) + 3).r );

		ivec2 p1 = ivec2(x1, y1);
		ivec2 p2 = ivec2(x2, y2);
		ivec2 p3 = ivec2(x3, y3);
		ivec2 p4 = ivec2(x4, y4);

		// 0x01 is the flag for ON_CURVE_POINT
		// Two on points means it's a straight line, so generate a fake point in between
		if ((flag1 & uint(1)) != uint(0) && (flag2 & uint(1)) != uint(0)) 
		{
			ivec2 fakeP2 = ivec2(p1.x + p2.x / int(2), p1.y + p2.y / int(2));
			calculateWindingNumber(p1, fakeP2, p2, pixelCoords, windingNumber);
			previousPoint = p2;
		}
		// On point, off point, on point is a standard bezier curve
		else if ((flag1 & uint(1)) != uint(0) && (flag2 & uint(1)) == uint(0) && (flag3 & uint(1)) != uint(0))
		{
			calculateWindingNumber(p1, p2, p3, pixelCoords, windingNumber);
			previousPoint = p3;
			i += uint(1);
		}
		// Off point, off point implies ghost point between the two off points
		else if ((flag1 & uint(1)) == uint(0) && (flag2 & uint(1)) == uint(0))
		{
			ivec2 fakeMidpoint = ivec2(p1.x + p2.x / int(2), p1.y + p2.y / int(2));
			ivec2 nextPoint = p3;
			if ((flag3 & uint(1)) == uint(0))
			{
				// If the next point is also off point, create another fake midpoint
				nextPoint = ivec2(p2.x + p3.x / int(2), p2.y + p3.y / int(2));
			}

			calculateWindingNumber(previousPoint, p1, fakeMidpoint, pixelCoords, windingNumber);
			calculateWindingNumber(fakeMidpoint, p1, nextPoint, pixelCoords, windingNumber);
			previousPoint = nextPoint;
			i += uint(2);
		}
	}

	float wColor = float(abs(windingNumber)) / 2.0;
	color = vec4(wColor, wColor, wColor, 1);
//	if (windingNumber == 0) {
//		color = vec4(0, 0, 0, 1);
//	} else {
//		color = vec4(1, 1, 1, 1);	
//	}
}