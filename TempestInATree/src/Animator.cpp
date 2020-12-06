#include <vector>
#include <memory>

#include "Animator.h"



LedColor ScaleColor(float scale, LedColor color)
{
	LedColor retval = 0;

	retval |= (uint8_t)((float)Red(color) * scale) << 16;
	retval |= (uint8_t)((float)Green(color) * scale) << 8;
	retval |= (uint8_t)((float)Blue(color) * scale) << 0;

	return retval;
}

void ScalePattern(float scale, std::vector<LedColor>& pattern)
{
    for(auto colorIter = pattern.begin(); colorIter != pattern.end(); ++colorIter)
    {
        *colorIter = ScaleColor(scale, *colorIter);
    }
}

AnimatorGroup::AnimatorGroup() :
	duration_override_(0)
{

}

Animator::~Animator()
{

}

AnimatorGroup::~AnimatorGroup()
{
	while (!animators_.empty())
	{
		AnimatorInfo pLast = animators_.back();
		delete pLast.pAnimator;
		animators_.pop_back();
	}
}

void AnimatorGroup::AddAnimator(Animator* pAnimator, TickCount startTime)
{
	AnimatorInfo info(pAnimator, startTime);
	animators_.push_back(info);

	// Update the duration - the end of the last animation to complete
	TickCount maxTickCount = 0;
	for (auto animatorInfo = animators_.begin(); animatorInfo != animators_.end(); ++animatorInfo)
	{
		TickCount animationEnd = animatorInfo->startTime + animatorInfo->pAnimator->duration();
		if (animationEnd > maxTickCount)
		{
			maxTickCount = animationEnd;
		}
	}

	if (duration_override_ == 0)
	{
		duration_ = maxTickCount;
	}
	else
	{
		duration_ = duration_override_;
	}
}

void AnimatorGroup::AppendAnimator(Animator* pAnimator, TickCount offset)
{
	AddAnimator(pAnimator, duration_ + offset);
}

void AnimatorGroup::OverrideDuration(TickCount forced_duration)
{
	duration_override_ = forced_duration;
	duration_ = duration_override_;
}

void AnimatorGroup::Step(TickCount localTime, LedColor* pColors)
{
	for (auto animatorInfo = animators_.begin(); animatorInfo != animators_.end(); ++animatorInfo)
	{
		if (localTime >= animatorInfo->startTime && localTime < animatorInfo->startTime + animatorInfo->pAnimator->duration())
		{
			animatorInfo->pAnimator->Step(localTime - animatorInfo->startTime, pColors);
		}
	}
}

FadeAnimator::FadeAnimator(TickCount fadeInDuration, TickCount holdDuration, TickCount fadeOutDuration, LedIndex startLedIndex, LedCount ledCount, Animator* childAnimator) :
	fadeInDuration_(fadeInDuration), holdDuration_(holdDuration), fadeOutDuration_(fadeOutDuration),
	startLedIndex_(startLedIndex), ledCount_(ledCount),
	childAnimator_(childAnimator)
{
	// Special case: holdDuration of 0 makes the whole fade animator duration be
	// the diration of the child (maybe have a seperate constructor for this...)
	if (holdDuration_ == 0)
	{
		holdDuration_ = childAnimator_->duration() - (fadeInDuration_ + fadeOutDuration_);
	}

	duration_ = fadeInDuration_ + holdDuration_ + fadeOutDuration_;
}

void FadeAnimator::Step(TickCount localTime, LedColor* pColors)
{
	childAnimator_->Step(localTime, pColors);
	if (localTime < fadeInDuration_)
	{
		float scale = (float)localTime / (float)fadeInDuration_;
		for (LedIndex ledIndex = startLedIndex_; ledIndex < startLedIndex_ + ledCount_; ++ledIndex)
		{
			pColors[ledIndex] = ScaleColor(scale, pColors[ledIndex]);
		}
	}
	else if (localTime < fadeInDuration_ + holdDuration_)
	{
		// nothing to do
	}
	else
	{
		float scale = 1.0f - ((float)(localTime - (fadeInDuration_ + holdDuration_)) / (float)fadeOutDuration_);
		for (LedIndex ledIndex = startLedIndex_; ledIndex < startLedIndex_ + ledCount_; ++ledIndex)
		{
			pColors[ledIndex] = ScaleColor(scale, pColors[ledIndex]);
		}
	}
}

SolidColor::SolidColor(TickCount duration, LedIndex startLedIndex, LedCount ledCount, LedColor color) :
	startLedIndex(startLedIndex),
	ledCount(ledCount),
	color(color)
{
	duration_ = duration;
}

void SolidColor::Step(TickCount localTime, LedColor* pColors)
{
	LedColor* pEnd = pColors + startLedIndex + ledCount;
	for (LedColor* pScan = pColors + startLedIndex; pScan < pEnd; pScan++)
	{
		*pScan = color;
	}
}

void ChaseAnimator::CreatePattern(LedColor color, LedCount width, LedCount space, std::vector<LedColor>& pattern)
{
	int rampCount = width / 2;
	float slope = 1.0f / (rampCount + 1.0f);

	for (int i = 0; i < rampCount; ++i)
	{
		pattern.push_back(ScaleColor((i + 1) * slope, color));
	}
	pattern.push_back(color);
	for (int i = 0; i < rampCount; ++i)
	{
		pattern.push_back(ScaleColor((i + 1) * (1.0f - slope), color));
	}

	for (int i = 0; i < space; ++i)
	{
		pattern.push_back(0x00000000);
	}
}

ChaseAnimator::ChaseAnimator(TickCount duration, LedIndex startLedIndex, LedCount ledCount, LedColor color, LedCount width, LedCount space, TickCount step_time) :
	startLedIndex(startLedIndex),
	ledCount(ledCount),
	color(color),
	width_(width),
	space_(space),
	step_time_(step_time)
{
	duration_ = duration;

	// Width needs to be odd - center LED will be the full color
	if (width_ % 2 == 0)
	{
		++width_;
	}

	CreatePattern(color, width_, space_, pattern);
}

void ChaseAnimator::Step(TickCount localTime, LedColor* pColors)
{
	uint32_t step_index = localTime / step_time_;

	LedIndex patternIndex = (pattern.size() - 1) - (step_index % pattern.size());
	for (LedIndex ledIndex = startLedIndex; ledIndex < startLedIndex + ledCount; ++ledIndex)
	{
		pColors[ledIndex] = pattern[patternIndex];
		patternIndex = (patternIndex + 1) % pattern.size();
	}
}

void SingleChaseAnimator::CreatePattern(LedColor color, LedCount width, std::vector<LedColor>& pattern)
{
	int rampCount = width / 2;
	float slope = 1.0f / (rampCount + 1.0f);

	for (int i = 0; i < rampCount; ++i)
	{
		pattern.push_back(ScaleColor((i + 1) * slope, color));
	}
	pattern.push_back(color);
	for (int i = 0; i < rampCount; ++i)
	{
		pattern.push_back(ScaleColor((i + 1) * (1.0f - slope), color));
	}
}

SingleChaseAnimator::SingleChaseAnimator(TickCount duration, LedIndex startLedIndex, LedCount ledCount, LedColor color, LedCount width) :
	startLedIndex(startLedIndex),
	ledCount(ledCount),
	color(color),
	width_(width)
{
	duration_ = duration;

	// Width needs to be odd - center LED will be the full color
	if (width_ % 2 == 0)
	{
		++width_;
	}

	CreatePattern(color, width_, pattern);
}

void SingleChaseAnimator::Step(TickCount localTime, LedColor* pColors)
{
	int startOffset = -(static_cast<int>(width_) / 2);
	int stepCount = ledCount + width_;
	int ticksPerStep = duration_ / stepCount;
	if (ticksPerStep == 0)
	{
		ticksPerStep = 1;
	}

	int stepIndex = localTime / ticksPerStep;
	int patternPosition = startOffset + stepIndex;

	for (unsigned int iPatternLed = 0; iPatternLed < pattern.size(); ++iPatternLed)
	{
		int iDestination = patternPosition + iPatternLed;
		if (iDestination >= 0 && iDestination < ledCount)
		{
			pColors[startLedIndex + iDestination] = pattern[iPatternLed];
		}
	}
}


FillInAnimator::FillInAnimator(TickCount duration, LedIndex startLedIndex, LedCount ledCount, LedColor color) :
	startLedIndex(startLedIndex),
	ledCount(ledCount),
	color(color)
{
	duration_ = duration;
}

void FillInAnimator::Step(TickCount localTime, LedColor* pColors)
{
	int ticksPerStep = duration_ / ledCount;
	if (ticksPerStep == 0)
	{
		ticksPerStep = 1;
	}

	int stepIndex = localTime / ticksPerStep;

	for (int iLed = 0; iLed < stepIndex; ++iLed)
	{
		pColors[iLed + startLedIndex] = color;
	}
}

RangeAnimator::RangeAnimator(TickCount duration, TickCount stepDuration) :
	stepDuration_(stepDuration)
{
	duration_ = duration;
}

void RangeAnimator::AddRange(LedIndex ledIndex, LedCount ledCount)
{
	Range range{ ledIndex, ledCount };
	ranges.push_back(range);
}

void RangeAnimator::AddColor(LedColor color)
{
	colors.push_back(color);
}

void RangeAnimator::Step(TickCount localTime, LedColor* pColors)
{
	int colorIndex = localTime / stepDuration_;

	for (auto range = ranges.begin(); range != ranges.end(); ++range)
	{
		LedColor color = colors[colorIndex % colors.size()];

		for (LedIndex ledIndex = range->ledIndex; ledIndex < range->ledIndex + range->ledCount; ++ledIndex)
		{
			pColors[ledIndex] = color;
		}

		colorIndex = (colorIndex + 1) % colors.size();
	}
}

RepeatedPatternAnimator::RepeatedPatternAnimator(TickCount duration, LedIndex startLedIndex, LedCount ledCount, const std::vector<LedColor>& patternParam) :
	startLedIndex(startLedIndex),
	ledCount(ledCount),
	pattern(patternParam)
{
	duration_ = duration;
}

void RepeatedPatternAnimator::Step(TickCount localTime, LedColor* pColors)
{

	LedIndex patternIndex = 0;
	for (LedIndex ledIndex = startLedIndex; ledIndex < startLedIndex + ledCount; ++ledIndex)
	{
		pColors[ledIndex] = pattern[patternIndex];
		patternIndex = (patternIndex + 1) % pattern.size();
	}
}

