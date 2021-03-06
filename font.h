/*
Copyright 2013-2015 Rohit Nirmal

This file is part of Clonepoint.

Clonepoint is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Clonepoint is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Clonepoint.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef FONT_H
#define FONT_H
#include <string>
#include <SDL2/SDL_opengl.h>
#include "stb_truetype.h"

class Font
{
public:
	Font(std::string filename, float size);
	~Font();
	stbtt_bakedchar* data();
	GLuint getTexture();
	float getSize();
private:
	GLuint _atlas;
	stbtt_bakedchar _cdata[96]; //used to help draw text
	float _size;
};
#endif