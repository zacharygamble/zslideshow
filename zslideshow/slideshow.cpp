#include "slideshow.h"

#include "utility.h"

bool ImageRaw::load(std::filesystem::path file_path) {
	using namespace std;
	cout << "Loading image: " << file_path << endl;
	// FIXME if the raw image was created another way, things might really break here...

	if (raw_data) {
		stbi_image_free(raw_data);
	}

	string str = file_path.string();
	const char* filename = str.c_str();
	raw_data = stbi_load(filename, &width, &height, &bpp, 0);
	if (!raw_data) {
		cout << stbi_failure_reason() << endl;
		is_loaded_ = false;
		return false;
	}
	is_loaded_ = true;

	return true;
}


void SlideShow::update_image(Image& image)
{
	using namespace std;
	bool allocNewTexture = true;

	image.update = false;
	auto& raw = image.raw;
	//FIXME: Currently slideshow depends on all slides having space in VRAM
	if (!is_loaded(raw)) 
		assert(false && "Image was not loaded first");

	// See if this texture already has entry
	auto it = images.find(image.file_path.string());
	assert(it != end(images) && "Image not in list");

	int index = it->second;
	auto it2 = texSlideIndex.find(index);
	if (it2 != end(texSlideIndex)) {
		auto& data = *it2->second;
		allocNewTexture = false; //?? // FIXME
		raw.tex_name = data.tex_name;
		data.data_size = raw.width * raw.height * raw.bpp;
	}
	else {
		glGenTextures(1, &raw.tex_name);
		allocNewTexture = true;
	}

	// Move the loaded texture into the gl texture
	cout << "Loading image" << endl;
	bool load = load_single_texture(raw, raw.tex_name);
	if (!load && allocNewTexture) {
		cout << "Image load failed" << endl;
		// FIXME error handling
		if (allocNewTexture)
			glDeleteTextures(1, &raw.tex_name);
		return;
	} 

	if (allocNewTexture) {
		add_tex_data(raw.tex_name, image, index);
	}
}

void SlideShow::init()
{
	assert(image_list.size() == 0);
	auto& image = image_list.emplace_back();
	auto& raw = image.raw;
	raw = create_default_image();
}