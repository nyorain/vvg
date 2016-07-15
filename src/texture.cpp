#include <vgk.hpp>
#include <texture.hpp>
#include <nanovg/nanovg.h>

#include <vpp/device.hpp>
#include <vpp/image.hpp>
#include <vpp/defs.hpp>

namespace vgk
{

Texture::Texture(Renderer& renderer, unsigned int xid, unsigned int format, unsigned int w,
	unsigned int h, int flags, const std::uint8_t& data) : id_(xid), width_(w), height_(h)
{
	vk::Format vkformat = vk::Format::r8g8b8a8Unorm;
	components_ = 4;
	if(format == NVG_TEXTURE_ALPHA)
	{
		vkformat = vk::Format::r8Unorm;
		components_ = 1;
	}

	vk::Extent3D extent {w, h, 1};

	auto info = vpp::ViewableImage::defaultColor2D();
	info.imgInfo.extent = extent;
	info.imgInfo.initialLayout = vk::ImageLayout::preinitialized;
	info.imgInfo.tiling = vk::ImageTiling::linear;
	info.imgInfo.format = vk::Format::r8g8b8a8Unorm;
	info.viewInfo.format = vk::Format::r8g8b8a8Unorm;
	info.memoryFlags = vk::MemoryPropertyBits::hostVisible;
	image_ = vpp::ViewableImage(renderer.device(), info);

	vpp::fill(image_.image(), data, vk::ImageLayout::preinitialized, extent,
		{vk::ImageAspectBits::color, 0, 0, 1}, w * h * components_);
}

void Texture::update(unsigned int x, unsigned int y, unsigned int w, unsigned int h,
	const std::uint8_t& data)
{
	vk::Extent3D extent {w, h, 1};
	vk::Extent3D offset {x, y, 0};

	///XXX: offset!
	vpp::fill(image_.image(), data, vk::ImageLayout::preinitialized, extent,
		{vk::ImageAspectBits::color, 0, 0, 1}, w * h * components_);
}

}
