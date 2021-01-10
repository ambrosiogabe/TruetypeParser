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

void parametric(in int p0, in int p1, in float t, inout int result)
{
	// Calculate the parametric equation for bezier curve using cValue as the parameter 
	float floatResult = float(p0) + ( (float(p1) - float(p0)) * t);
	result = int(floatResult);
}

void getIntersection(in ivec2 p0, in ivec2 p1, in float t, inout int windingNumber)
{
	float midpoint = t / 2.0;
	int y = 0;
	parametric(p0.y, p1.y, midpoint, y);
		
	if (y > 0) { windingNumber--; }
	else if (y < 0) { windingNumber++; }
}

void getIntersection(in ivec2 p0, in ivec2 p1, in ivec2 p2, in float t, in float minX, inout int windingNumber)
{
	int cx = -1;
	parametric(p0.x, p1.x, p2.x, t, cx);
	if (cx < minX) { return; }

	float midpoint = t / 2.0;
	int y = 0;
	parametric(p0.y, p1.y, p2.y, midpoint, y);
		
	if (y > 0) { windingNumber--; }
	else if (y < 0) { windingNumber++; }
}

void calculateWindingNumber(in ivec2 p0, in ivec2 p1, in ivec2 x, inout int windingNumber)
{
	// Translate the points to x's local space
	p0 -= x;
	p1 -= x;

	float t = (-float(p0.y)) / (float(p1.y) - float(p0.y));
	int cx = -1;
	parametric(p0.x, p1.x, t, cx);
	if (t >= 0 && t < 1 && cx >= 0)
	{
		// T is a root and intersects the ray
		getIntersection(p0, p1, t, windingNumber);
	}
}

void calculateWindingNumber(in ivec2 p0, in ivec2 p1, in ivec2 p2, in ivec2 x, inout int windingNumber)
{
	// Translate the points to x's local space
	p0 -= x;
	p1 -= x;
	p2 -= x;

	float a = float(p1.y) - 2.0 * float(p0.y) + float(p2.y);
	float b = float(p1.y) - float(p0.y);
	float c = float(p0.y);
	
	float squareRootOperand = b * b - a * c;
	if (squareRootOperand < 0)
	{
		return;
	}

	float squareRoot = sqrt(squareRootOperand);
	float t0 = (b - squareRoot) / a;
	float t1 = (b + squareRoot) / a;

	if (a < 0.1)
	{
		t0 = c / (2.0 * b);
		t1 = c / (2.0 * b);
	}

	if (t0 >= 0 && t0 < 1 && !(t1 >= 0 && t1 < 1))
	{
		// T0 is the only root
		getIntersection(p0, p1, p2, t0, 0.0, windingNumber);
	}
	else if (t1 >= 0 && t1 < 1 && !(t0 >= 0 && t0 < 1))
	{
		// T1 is the only root and we have intersected 
		getIntersection(p0, p1, p2, t1, 0.0, windingNumber);
	}
	else if (t0 >= 0 && t0 < 1 && t1 >= 0 && t1 < 1)
	{
		// FIXME 
		// T0 and T1 are roots and have intersected
		// Only examine the t values between the roots
		getIntersection(p0, p1, p2, t0, t0, windingNumber);
		getIntersection(p0, p1, p2, t1, t1, windingNumber);
	}
}

void convert(in uint val, inout int res)
{
	res = int(val);
	if ((val & uint(0x8000)) != uint(0)) { res = -int(uint(0xFFFF) - val) - 1; }
}

void main() 
{	
	float maxX = 1369.0;
	float minX = -3.0;
	float maxY = 1466.0;
	float minY = 0.0;

	float newX = (fPos.x - (-0.5)) / (0.5 - (-0.5)) * (maxX - minX) + minX;
	float newY = (fPos.y - (-0.5)) / (0.5 - (-0.5)) * (maxY - minY) + minY;
	ivec2 pixelCoords = ivec2(int(newX), int(newY));

	uint numGlyphs = texelFetch(uFont, 0).r | (texelFetch(uFont, 1).r << 8);
	// Divide by 2 since offset is in bytes, but we are indexing as shorts (8 bit vs 16 bit)
	// 66 is the codepoint for the character 'A' + 1
	uint aOffsetUint = (texelFetch(uFont, 66 * 2).r | (texelFetch(uFont, 66 * 2 + 1).r << 8)) >> 1;
	int aOffset = int(aOffsetUint);
	uint numContours = texelFetch(uFont, aOffset).r;
	uint numPoints = texelFetch(uFont, aOffset + int(numContours) + 1).r;
	int contourTableOffset = aOffset + 1;
	int flagTableOffset = contourTableOffset + int(numContours) + 1;
	int xTableOffset = flagTableOffset + int(numPoints);
	int yTableOffset = xTableOffset + int(numPoints);

	// If the winding number is 0, pixel is off, otherwise pixel is on
	int windingNumber = int(0);
	uint point = uint(0);
	for (uint i=uint(0); i <= numContours; i++)
	{
		uint offset = point;
		uint contourEnd = texelFetch(uFont, contourTableOffset + int(i)).r + uint(1);
		for (;point < contourEnd; point++)
		{
			int i0NotMod = int(point);
			int i0 = i0NotMod;
			if (i0NotMod >= int(contourEnd)) { i0 = int(mod(i0NotMod, contourEnd) + offset); }

			int i1NotMod = int(point) + 1;
			int i1 = i1NotMod;
			if (i1NotMod >= int(contourEnd)) { i1 = int(mod(i1NotMod, contourEnd) + offset); }

			int i2NotMod = int(point) + 2;
			int i2 = i2NotMod;
			if (i2NotMod >= int(contourEnd)) { i2 = int(mod(i2NotMod, contourEnd) + offset); }

			uint flag1 = texelFetch(uFont, flagTableOffset + i0).r;
			uint flag2 = texelFetch(uFont, flagTableOffset + i1).r;
			uint flag3 = texelFetch(uFont, flagTableOffset + i2).r;

			int x1 = 0;
			convert(texelFetch(uFont, xTableOffset + i0).r, x1);
			int x2 = 0;
			convert(texelFetch(uFont, xTableOffset + i1).r, x2);
			int x3 = 0;
			convert(texelFetch(uFont, xTableOffset + i2).r, x3);

			int y1 = 0;
			convert(texelFetch(uFont, yTableOffset + i0).r, y1);
			int y2 = 0;
			convert(texelFetch(uFont, yTableOffset + i1).r, y2);
			int y3 = 0;
			convert(texelFetch(uFont, yTableOffset + i2).r, y3);

			ivec2 p1 = ivec2(x1, y1);
			ivec2 p2 = ivec2(x2, y2);
			ivec2 p3 = ivec2(x3, y3);

			// 0x01 is the flag for ON_CURVE_POINT
			// Two on points means it's a straight line
			if ((flag1 & uint(1)) != uint(0) && (flag2 & uint(1)) != uint(0) && i1NotMod <= int(contourEnd)) 
			{
				calculateWindingNumber(p1, p2, pixelCoords, windingNumber);
			}
			// On point, off point, on point is a standard bezier curve
			else if ((flag1 & uint(1)) != uint(0) && (flag2 & uint(1)) == uint(0) && (flag3 & uint(1)) != uint(0))
			{
				// We enter here.
				calculateWindingNumber(p1, p2, p3, pixelCoords, windingNumber);
				point++;
			}
			// On point, off point, off point, implies ghost point between p2 and p3
			else if ((flag1 & uint(1)) != uint(0) && (flag2 & uint(1)) == uint(0) && (flag3 & uint(1)) == uint(0))
			{
				// Ghost point (no ++ maybe?)
				ivec2 fakeMidpoint = ivec2(p2.x + p3.x / int(2), p2.y + p3.y / int(2));
				//calculateWindingNumber(p1, p2, fakeMidpoint, pixelCoords, windingNumber);
			}
			// Off point, off point, off point implies two ghost points 
			else if ((flag1 & uint(1)) == uint(0) && (flag2 & uint(1)) == uint(0) && (flag3 & uint(1)) == uint(0))
			{
				// Two ghost points (no ++ maybe?)
				ivec2 fakeP1 = ivec2(p1.x + p2.x / int(2), p1.y + p2.y / int(2));
				ivec2 fakeP2 = ivec2(p2.x + p3.x / int(2), p2.y + p3.y / int(2));
				//calculateWindingNumber(fakeP1, p1, fakeP2, pixelCoords, windingNumber);
			}
			// Off point, off point, on point implies one fake ghost point between p1 and p2
			else if ((flag1 & uint(1)) == uint(0) && (flag2 & uint(1)) == uint(0) && (flag3 & uint(1)) != uint(0))
			{
				// Ghost point
				ivec2 fakeMidpoint = ivec2(p1.x + p2.x / int(2), p1.y + p2.y / int(2));
				//calculateWindingNumber(p1, p2, fakeMidpoint, pixelCoords, windingNumber);
				point++;
			}

			// TODO: is this double counting?
			// Extra if to see if we need to connect the last two points
			if ((flag2 & uint(1)) != uint(0) && (flag3 & uint(1)) != uint(0) && i2NotMod < int(contourEnd) - 1)
			{
				// If p3 is also on, connect it as well 
				calculateWindingNumber(p2, p3, pixelCoords, windingNumber);
				point++;
			}
		}

		// Reset the point to the beginning of the next contour
		point = contourEnd;
	}

	if (windingNumber == 0) {
		color = vec4(0, 0, 0, 1);
	} else {
		color = vec4(1, 1, 1, 1);	
	}
}