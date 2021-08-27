#pragma once

#include <array>

namespace chip8 {

class cpu {

};

template<unsigned Width = 64, unsigned Height = 32, typename PixelType = unsigned int>
class video {
public:
	static constexpr unsigned width = Width;
	static constexpr unsigned height = Height;
	static constexpr size_t size = width * height;
	static constexpr auto aspect_ratio = static_cast<float>(width) / static_cast<float>(height);

	using pixel_t = PixelType;
	using coord_t = int;
	using framebuffer_t = std::array<pixel_t, size>;

	constexpr void set(coord_t x, coord_t y, pixel_t pixel = 0) {
		const size_t index = static_cast<size_t>(y) * width + static_cast<size_t>(x);
		if (index < _framebuffer.size()) {
			_framebuffer[index] = pixel;
		}
	}

	constexpr pixel_t* framebuffer() { return _framebuffer.data(); }

private:
	framebuffer_t _framebuffer{};
};

class audio {

};

} // namespace chip8

