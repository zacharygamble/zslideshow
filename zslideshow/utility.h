#pragma once

#ifndef UTILITY_H_INCLUDED
#define UTILITY_H_INCLUDED

#include <Windows.h>
#include <gl/GL.h>

#include "slideshow.h"

inline
void clear_gl_errors()
{
	while (glGetError() != GL_NO_ERROR);
}

GLuint create_default_texture();
ImageRaw create_default_image();
bool load_single_texture(ImageRaw& raw, GLuint tex_name);

#include "util/utility.h"
// FIXME decouple

#endif // UTILITY_H_INCLUDED
