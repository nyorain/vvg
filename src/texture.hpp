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

protected:
	unsigned int id_;
	unsigned int width_;
	unsigned int height_;

	vpp::ViewableImage image_;
};

}
