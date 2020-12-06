#pragma once

typedef uint16_t LedIndex;
typedef uint16_t LedCount;
typedef uint32_t LedColor;
typedef uint32_t TickCount;

const TickCount TicksPerSecond = 1000;
const TickCount TickCountMax = UINT32_MAX;

inline uint8_t Red(LedColor color) { return (color >> 16) & 0xFF; }
inline uint8_t Green(LedColor color) { return (color >> 8) & 0xFF; }
inline uint8_t Blue(LedColor color) { return (color >> 0) & 0xFF; }
LedColor ScaleColor(float scale, LedColor color);
void ScalePattern(float scale, std::vector<LedColor>& pattern);

const LedColor color_red = 0x0000FF00;
const LedColor color_green = 0x00FF0000;
const LedColor color_blue = 0x000000FF;
const LedColor color_white = 0x00FFFFFF;
const LedColor color_pink = 0x0099FFFFF;
const LedColor color_yellow = 0x00FFFF33;
const LedColor color_cyan = 0x00FF00FF;
const LedColor color_black = 0;

class Animator
{
public:
	// localTime - always between 0 and the duration
	virtual void Step(TickCount localTime, LedColor* pColors) = 0;

	TickCount duration() const { return duration_; }

	Animator() : duration_(0) {}
	virtual ~Animator();
protected:
	TickCount duration_;
};

class AnimatorGroup : public Animator
{
private:
	struct AnimatorInfo
	{
		AnimatorInfo(Animator* pa, TickCount st) : pAnimator(pa), startTime(st) {}

		Animator* pAnimator;
		TickCount startTime;
	};

	TickCount duration_override_;
	std::vector<AnimatorInfo> animators_;

public:
	AnimatorGroup();
	~AnimatorGroup();
	void AddAnimator(Animator* pAnimator, TickCount startTime);
	void AppendAnimator(Animator* pAnimator, TickCount offset = 0);
	void OverrideDuration(TickCount forced_duration);
	virtual void Step(TickCount localTime, LedColor* pColors);
};

class FadeAnimator : public Animator
{
private:
	TickCount fadeInDuration_;
	TickCount holdDuration_;
	TickCount fadeOutDuration_;
	LedIndex startLedIndex_;
	LedCount ledCount_;
	std::unique_ptr<Animator> childAnimator_;

public:
	FadeAnimator(TickCount fadeInDuration, TickCount holdDuration, TickCount fadeOutDuration, LedIndex startLedIndex, LedCount ledCount, Animator* childAnimator);
	virtual void Step(TickCount localTime, LedColor* pColors);
};

class SolidColor : public Animator
{
private:
	LedIndex startLedIndex;
	LedCount ledCount;
	LedColor color;

public:
	SolidColor(TickCount duration, LedIndex startLedIndex, LedCount ledCount, LedColor color);
	virtual void Step(TickCount localTime, LedColor* pColors);
};

class ChaseAnimator : public Animator
{
private:
	LedIndex startLedIndex;
	LedCount ledCount;
	LedColor color;
	LedCount width_;
	LedCount space_;
	TickCount step_time_;
	std::vector<LedColor> pattern;

	static void CreatePattern(LedColor color, LedCount width, LedCount space, std::vector<LedColor>& pattern);

public:
	ChaseAnimator(TickCount duration, LedIndex startLedIndex, LedCount ledCount, LedColor color, LedCount width, LedCount space, TickCount step_time);
	virtual void Step(TickCount localTime, LedColor* pColors);
};


class SingleChaseAnimator : public Animator
{
private:
	LedIndex startLedIndex;
	LedCount ledCount;
	LedColor color;
	LedCount width_;
	std::vector<LedColor> pattern;

	static void CreatePattern(LedColor color, LedCount width, std::vector<LedColor>& pattern);

public:
	SingleChaseAnimator(TickCount duration, LedIndex startLedIndex, LedCount ledCount, LedColor color, LedCount width);
	virtual void Step(TickCount localTime, LedColor* pColors);
};

class FillInAnimator : public Animator
{
private:
	LedIndex startLedIndex;
	LedCount ledCount;
	LedColor color;

public:

	FillInAnimator(TickCount duration, LedIndex startLedIndex, LedCount ledCount, LedColor color);
	virtual void Step(TickCount localTime, LedColor* pColors);
};

class RangeAnimator : public Animator
{
private:
	struct Range
	{
		LedIndex ledIndex;
		LedCount ledCount;
	};

	std::vector<Range> ranges;
	std::vector<LedColor> colors;
	TickCount stepDuration_;
public:
	RangeAnimator(TickCount duration, TickCount stepDuration);
	void AddRange(LedIndex ledIndex, LedCount ledCount);
	void AddColor(LedColor color);
	virtual void Step(TickCount localTime, LedColor* pColors);
};

class RepeatedPatternAnimator : public Animator
{
private:
	LedIndex startLedIndex;
	LedCount ledCount;
	std::vector<LedColor> pattern;

	static void CreatePattern(LedColor color, LedCount width, LedCount space, std::vector<LedColor>& pattern);

public:
	RepeatedPatternAnimator(TickCount duration, LedIndex startLedIndex, LedCount ledCount, const std::vector<LedColor>& pattern);
	virtual void Step(TickCount localTime, LedColor* pColors);
};

