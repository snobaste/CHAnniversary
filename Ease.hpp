#pragma once

template <typename T>
class Ease {
public:
	Ease(T start, T end, float duration) :
		start(start),
		end(end),
		duration(duration) {
		passed = 0.0f;
	}

	T Advance(float delta) {
		passed += delta;
		value = start + EaseInCubic(passed / duration) * end;
		return value;
	}

	const T Start() const {
		return start;
	}

	const T End() const {
		return end;
	}

	const T Value() const {
		return value;
	}

private:
	float EaseInCubic(float x) {
		return x * x * x;
	}

	T start;
	T end;
	T value;

	float duration = 0.0f;
	float passed = 0.0f;
};