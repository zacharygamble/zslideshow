#include <Windows.h>
#include <gl/GL.h>
#include <iostream>

#include "utility.h"

using namespace std;

static const int DEF_WIDTH = 8;
static const int DEF_HEIGHT = 8;

static uint8_t* create_default_texture_data()
{
	struct rgb_t {
		uint8_t r, g, b;
	} const black{ 0,0,0 },  magenta{255,2,219};
	const int w = DEF_WIDTH;
	const int h = DEF_HEIGHT;

	uint8_t* raw_data = new uint8_t[sizeof(rgb_t) * w * h];
	if (!raw_data) {
		cout << "create_default_texture_data: allocation failure" << endl;
		return nullptr;
	}

	rgb_t* color_arr = reinterpret_cast<rgb_t*>(raw_data);
	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {
			// alternate black/magenta on both row and column
			if ((i + j) % 2 == 0)
				color_arr[w * i + j] = black;
			else 
				color_arr[w * i + j] = magenta;
		}
	}

	return raw_data;
}

GLuint create_default_texture()
{
	uint8_t* data = create_default_texture_data();

	GLuint tex;
	glGenTextures(1, &tex);
	if (tex == 0)
		return 0;

	clear_gl_errors();

	glBindTexture(GL_TEXTURE_2D, tex);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // packed
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, DEF_WIDTH, DEF_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

	if (glGetError() != GL_NO_ERROR) {
		cout << "OpenGL default texture init failed" << endl;
		glDeleteTextures(1, &tex);
		return 0;
	}

	return tex;
}

ImageRaw create_default_image()
{
	ImageRaw raw;
	raw.bpp = 3;
	raw.width = DEF_WIDTH;
	raw.height = DEF_HEIGHT;
	raw.tex_name = create_default_texture();
	if (raw.tex_name == 0)
		return raw; // is_loaded_ will be false, signalling a problem

	raw.is_loaded_ = true;
	raw.delete_gl_tex = true;

	return raw;
}

bool load_single_texture(ImageRaw& raw, GLuint tex_name) 
{
	glBindTexture(GL_TEXTURE_2D, tex_name);
	GLenum format; 
	if (raw.bpp == 1)
		format = GL_LUMINANCE;
	else if (raw.bpp == 2)
		format = GL_LUMINANCE_ALPHA;
	else if (raw.bpp == 3)
		format = GL_RGB;
	else if (raw.bpp == 4) 
		format = GL_RGBA;
	else {
		assert(false && "Image bpp invalid");
		return false;
	}

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // stb_image doesn't use padding on scanlines
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, raw.width, raw.height, 0, format, GL_UNSIGNED_BYTE, raw.raw_data);

	return true;
}
