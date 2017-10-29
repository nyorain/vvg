// TODO: add possibility to defer updates

template<typename T>
class Parameter {
public:
	Parameter() = defualt;
	Parameter(const T& value) : value_(value) {}
	Parameter(T&& value) : value_(std::move(value)) {}
	virtual ~Parameter() = default;

	virtual bool dynamic() const noexcept { return false; }
	virtual const T& get() const noexcept { return value_; }

protected:
	T value_;
};

template<typename T>
class DynamicParameter : public Parameter<T> {
public:
	using Parameter::Parameter;

	void set(const T& value) { value_ = value; updated(); }
	void set(T&& value) { value_ = value; updated(); }
	void updated() { if(observer_) observer_->changed(*this, value_); }

	ParameterObserver<T>* observer() const { return observer_; }
	void observer(ParameterObserver<T>* obs) const { return observer_ = obs; }

	virtual bool dynamic() const noexcept { return true; }

protected:
	ParameterObserver<T>* observer_ {};
};

template<typename T>
class ParameterObserver {
	virtual void change(DynamicParameter<T>& obj, const T& value);
};

class Renderer {
public:
	void rect(const Parameter<Vec2ui>& pos, const Parameter<Vec2ui>& size);
};




// static drawing
renderer.rect({0, 0}, {100, 100});
renderer.fill();
renderer.record();

every frame {
	renderer.draw();
}


// dynamic drawing
DyanmicParameter<Vec2ui> position;
DyanmicParameter<Vec2ui> size;

renderer.rect(position, size);
renderer.fill();
renderer.record();

every frame {
	updateRectAnimation(position, size);
	renderer.draw();
}
