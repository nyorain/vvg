#pragma once

#include <cstdint>
#include <vpp/image.hpp>

namespace vgk
{

class Renderer;
class Texture
{
public:
	Texture(Renderer& renderer, unsigned int xid, unsigned int format, unsigned int w,
		unsigned int h, int flags, const std::uint8_t& data);
	~Texture() = default;

	Texture(Texture&& other) noexcept = default;
	Texture& operator=(Texture&& other) noexcept = default;

	unsigned int id() const { return id_; }
	unsigned int width() const { return width_; }
	unsigned int height() const { return height_; }
	const vpp::ViewableImage& image() const { return image_; }

	void update(unsigned int x, unsigned int y, unsigned int w, unsigned int h,
		const std::uint8_t& data);

protected:
	unsigned int id_;
	unsigned int width_;
	unsigned int height_;

	vpp::ViewableImage image_;
	unsigned int components_;
};

}
