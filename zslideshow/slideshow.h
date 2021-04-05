#pragma once

#ifndef SLIDESHOW_H_INCLUDED
#define SLIDESHOW_H_INCLUDED

#include <Windows.h>
#include <gl/GL.h>
#include <cassert>
#include <iostream>
#include <filesystem>
#include <string>
#include <vector>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <map>
#include <algorithm>

#include <stb_image.h>

// types
struct ImageRaw {
	uint8_t* raw_data = nullptr;
	GLuint tex_name = 0;
	int width = 0;
	int height = 0;
	int bpp = 0;

	bool is_loaded_ = false;
	bool delete_gl_tex = false;

	bool is_loaded() const noexcept { return is_loaded_; }
	friend bool is_loaded(const ImageRaw& raw) noexcept { return raw.is_loaded(); }

	bool load(std::filesystem::path file_path);
	friend bool load(ImageRaw& raw, std::filesystem::path file_path) {
		raw.load(file_path);
	}

	~ImageRaw() {
		// FIXME: Unstable implementation
		bool loaded = is_loaded();
		if (raw_data != nullptr) {
			stbi_image_free(raw_data);
		}
		else if (loaded && delete_gl_tex && tex_name != 0) {
			glDeleteTextures(1, &tex_name);
		}
		else if (loaded && delete_gl_tex) {
			std::cout << "Warning: Potential error: image file appears loaded but cannot be released" << std::endl;
		}
	}
};

struct Image {
	std::string basename;
	std::filesystem::path file_path;
	std::filesystem::file_time_type last_update;
	bool update = true;

	ImageRaw raw;
};

struct SlideShowWorker;

class SlideShow {
	friend struct SlideShowWorker;

	struct GLTexData {
		GLuint tex_name; 
		size_t data_size;
		int currentImageSrc; // The Image in SlideShow that this is bound to

		bool SizeCompare(const GLTexData& other) const { return data_size < other.data_size; }
		bool ImageIndexCompare(const GLTexData& other) const { return currentImageSrc < other.currentImageSrc; }

		friend bool SizeCompare(const GLTexData* d1, const GLTexData* d2) { return d1->SizeCompare(*d2); }
		friend bool ImageIndexCompare(const GLTexData* d1, const GLTexData* d2) { return d1->ImageIndexCompare(*d2); }
	};

	std::unordered_map<std::string, int> images;
	std::vector<Image> image_list;
	std::queue<int> defer_update_list;
	std::map<size_t, std::set<GLTexData*>> texSizeIndex; // order from smallest to largest
	std::unordered_map<int, GLTexData*> texSlideIndex; // map slides to texture data
	std::vector<GLTexData> texData;
	int index = 0;

	void add_tex_size_entry(GLTexData* data_ptr)
	{
		size_t size = data_ptr->data_size;
		auto& s_index = texSizeIndex[size]; // get index or add new one
		s_index.insert(data_ptr);
	}

	void remove_tex_size_entry(GLTexData* data) {
		size_t sz = data->data_size;
		auto it = texSizeIndex.find(sz);
		if (it == end(texSizeIndex)) {
			assert(false && "Attempt to remove texData entry not in size index");
			return;
		}

		// now find the particular entry
		auto& index = it->second;
		auto result = index.extract(data);
		if (result.empty()) {
			assert(false && "Attempt to remove texData entry not in size index");
			return;
		}
		
		if (index.empty()) {
			texSizeIndex.erase(it);
		}

		// FIXME: IS something else needed here?
	}

	void add_tex_data(GLuint tex_name, const Image& image, int index)
	{
		// Assemble the entry
		GLTexData data;
		data.tex_name = tex_name;
		data.currentImageSrc = index;
		data.data_size = image.raw.width * image.raw.height * image.raw.bpp;

		// Add entry to list and indices
		texData.push_back(data);
		GLTexData* ptr = &texData.back();
		add_tex_size_entry(ptr);
		texSlideIndex.insert({ index, ptr });
	}

	// Swap target tex_name with new image data, also pass the corresponding index of the slide
	void update_tex_data(GLuint tex_name, const Image& image, int index)
	{
		using namespace std;
		// Find the relevant data for a particular tex_name; it should already be in the registry
		auto it = find_if(begin(texData), end(texData), [=](const GLTexData& x) { return x.tex_name == tex_name; });
		if (it != end(texData)) {
			assert(false && "Attempted to remove nonexistent texture from texData");
			return;
		}
		auto& data = *it;

		// Now update references in the indexing tables 
		size_t newSize = image.raw.width * image.raw.height * image.raw.bpp;
		if (data.data_size != newSize) {
			remove_tex_size_entry(&data);
			add_tex_size_entry(&data);
			data.data_size = newSize;
		}
		if (data.currentImageSrc != index) {
			texSlideIndex.extract(data.currentImageSrc);
			texSlideIndex[index] = &data;
			data.currentImageSrc = index;
		}
	}

	// Completely remove the texture data corresponding to a particular name
	// TODO At the moment, this could invalidate existing references elsewhere in the program
	void remove_tex_data(GLuint tex_name) 
	{
		using namespace std;
		auto it = find_if(begin(texData), end(texData), [=](const GLTexData& x) { return x.tex_name == tex_name; });
		if (it != end(texData)) {
			assert(false && "Attempted to remove nonexistent texture from texData");
			return;
		}
		auto data = *it;
		
		// clear the entry
		remove_tex_size_entry(&data);
		texSlideIndex.extract(data.currentImageSrc);
		texData.erase(it);
	}

	void add_image(const std::filesystem::path& path, bool defer_load)
	{
		namespace fs = std::filesystem;

		// add to the image list
		Image& img = image_list.emplace_back();
		img.file_path = path;
		img.basename = path.filename().string();
		img.last_update = fs::last_write_time(path);
		img.update = true;

		// Add to the image map
		int index = image_list.size() - 1;
		images.emplace(path.string(), index);

		if (defer_load) {
			defer_update_list.emplace(index);
		}
		else {
			update_image(img);
		}
	}

	void update_image(Image& image);

	void update_image(const std::filesystem::path& path) 
	{
		auto it = images.find(path.string());
		if (it == images.end()) {
			assert(false); // this should not happen
			return; // simply don't load it; hope nothing goes wrong
		}

		update_image(image_list[it->second]);
	}
public:
	void add_or_update(const std::filesystem::path& path, bool defer_load = false)
	{
		using namespace std;

		// First check if the image even exists
		string filepath = path.string();
		auto it = images.find(filepath);
		if (it == images.end()) {
			add_image(path, defer_load);
			return;
		}

		// It exists, check if it needs to be refreshed
		int i = it->second;
		Image& img = image_list[i];
		auto last_update = last_write_time(path);
		if (img.last_update < last_update) {
			img.update = true;
			img.last_update = last_update;
			if (defer_load)
				defer_update_list.emplace(i);
		}
	}

	void apply_deferred()
	{
		while (defer_update_list.size()) {
			int i = defer_update_list.front();
			defer_update_list.pop();

			Image& img = image_list[i];
			if (img.file_path.empty()) {
				assert(false && "Update called on built-in image"); // this should not be called on a built-in image...
				continue;
			}

			if (img.raw.load(img.file_path)) {
				update_image(img);
			}
			else {
				// FIXME determine failure action
			}
		}

		if (index == 0)
			index = 1;
	}
	
	void init();
	
	void next_slide()
	{
		int start = index;
		while (++index < image_list.size()) {
			if (image_list[index].raw.tex_name != 0)
				return;
		}
		index = 0;
		while (++index < start) {
			if (image_list[index].raw.tex_name != 0)
				return;
		}

		index = 0; // nothing loaded
	}

	void prev_slide()
	{
		index--;
		if (index < 1)
			index = image_list.size() - 1;
	}

	Image& current_slide() {
		if (index >= (int)image_list.size() || index < 0) {
			assert(false && "Index outside of range");
			index = 0;
		}
		return image_list[index];
	}

	// Keep only those images with file paths specified
	void prune(const std::unordered_set<std::string>& keep_paths)
	{
		using namespace std;
		set<int> remove_indices;
		// Find all images not in keep_paths; remove the indices from map
		for (auto& [k, v] : images) {
			if (keep_paths.count(k) != 1) {
				// remove image
				remove_indices.insert(v);
				images.extract(k);
			}
		}

		// For each index, remove the image at that index
		auto it = remove_indices.rbegin();
		for (; it != remove_indices.rend(); it++) {
			image_list.erase(image_list.begin() + *it);
		}

		// adjust index
		if (remove_indices.count(index)) {
			if (image_list.size() > 0)
				index = 1;
			else
				index = 0;
		}
		else {
			// over-engineered... don't move off current slide
			auto it = upper_bound(remove_indices.begin(), remove_indices.end(), index);
			int shift = distance(remove_indices.begin(), it);
			index -= shift;
		}
	}

	int slide_count() const { return images.size(); }
};

struct SlideShowWorker {
	HWND window;
};

#endif //SLIDESHOW_H_INCLUDED
