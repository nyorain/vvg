#pragma once

#include <vpp/buffer.hpp>
#include <vpp/pipeline.hpp>
#include <vpp/descriptor.hpp>

#include <vector>

namespace vgk
{

//minimal shader types
struct Vec2 { float x,y; };
struct Vec3 { float x,y,z; };
struct Vec4 { float x,y,z,w; };

using Mat2 = float[2][2];
using Mat3 = float[3][3];
using Mat4 = float[4][4];

struct UniformData
{
	Vec2 viewSize;
	std::uint32_t type;
	std::uint32_t texType;
	Vec4 innerColor;
	Vec4 outerColor;
	Mat4 scissorMat;
	Mat4 paintMat;
};

struct Path
{
	std::size_t fillOffset = 0;
	std::size_t fillCount = 0;
	std::size_t strokeOffset = 0;
	std::size_t strokeCount = 0;
};

struct DrawData
{
	vpp::DescriptorSet descriptorSet;
	UniformData uniformData;
	unsigned int texture = 0;

	std::vector<Path> paths;
	std::size_t triangleOffset = 0;
	std::size_t triangleCount = 0;
};

}

namespace vpp
{

template<> struct VulkanType<vgk::Vec2> : public VulkanTypeVec2 {};
template<> struct VulkanType<vgk::Vec3> : public VulkanTypeVec3 {};
template<> struct VulkanType<vgk::Vec4> : public VulkanTypeVec4 {};
template<> struct VulkanType<vgk::Mat2> : public VulkanTypeMatrix<2, 2> {};
template<> struct VulkanType<vgk::Mat3> : public VulkanTypeMatrix<3, 3> {};
template<> struct VulkanType<vgk::Mat4> : public VulkanTypeMatrix<4, 4> {};

template<> struct VulkanType<vgk::UniformData>
{
	static constexpr auto type = vpp::ShaderType::structure;
	static constexpr auto members = std::make_tuple(
		&vgk::UniformData::viewSize,
		&vgk::UniformData::type,
		&vgk::UniformData::texType,
		&vgk::UniformData::innerColor,
		&vgk::UniformData::outerColor,
		&vgk::UniformData::scissorMat,
		&vgk::UniformData::paintMat
	);
};

}
