/// The data specifying how something should be drawn.
/// Basically the render state, in nanovg terms.
class RenderData {
public:
	- transform
	- scissor
	- color (or gradient or texture)
};


class RenderPath {
public:
	- points and stuff
};

class RenderOperation {
public:
	- RenderData
	- RenderPath
	- stroke/fill/text
};


// Degress of freedom:
// - which buffer memory type to use (often updated?)
// - which things should be changeable at runtime
//  - e.g. if it is alwasys a rectangle use vkCmdDraw, otherwise vkCmdDrawIndirect


class MyRenderer {
public:
	void init() {
		op_ = RenderOperatoin::rect({0, 0}, {10, 10}, Color::red);
	}

	void update() {
		if(condition) {
			// some examples
		}
	}

	void record(vk::CommandBuffer cmdBuf) {
		op_.record(cmdBuf);
	}

	RenderOperation op_;
};
