# TruetypeParser

I will most likely archive this code since it did not lead to anything that would be useful in a production environment. This is just a truetype parser/vector graphics 
renderer built using OpenGL and C++. Most of the code is very hacky since I was just experimenting with this concept, but I got it to a point where it could
parse some TTF fonts and display the fonts using a custom shader (albeit with a lot of lag). 

I based a lot of the parsing off of [Sean Barrett's Truetype parser](https://github.com/nothings/stb/blob/master/stb_truetype.h). The rendering was based off of a research
paper that I found talking about how to render fonts using the bezier curve data directly. The way I stored the bezier curve data in binary was formatted like this:


### MainFile                    
| Data Type         | Name       |
| ----------------- | ---------- |
| uint32            | numGlyphs  |
| uint32[numGlyphs] | offsets    |
| Glyph[numGlyphs]  | Glyphs     |

### Glyph Data  
| Data Type         | Name       |
| ----------------- | ---------- |
| numContours		    | uint16 // If numContours is 0, this glyph is empty and the next glyph starts immediately after |
| contourEnds[numContours] |  uint16 |
| numPoints		| uint16 |
| flags[numPoints]	| uint16 |
| xCoords[numPoints]	| int16 |
| yCoords[numPoints]	| int16 |

Where the Main File is the overall structure of the binary data, and the Glyph Data is how each glyph structure in the Glyph array was formatted. I may update this README with 
more information about how this process worked in more depth in the future.
