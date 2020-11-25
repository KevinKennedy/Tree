/*
 * Project TempestInATree
 * Description:
 * Author:
 * Date:
 */

#include "Animator.h"
#include "GameEngine.h"
#include <neopixel.h>

SerialLogHandler logHandler;


const int EncoderAPin = D5;
const int EncoderBPin = D6;
unsigned int oldEncoderState;
int encoderValue;



#define PIXEL_COUNT 50 //500
#define PIXEL_PIN D2
#define PIXEL_TYPE WS2812B

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

AnimatorGroup* rootAnimator;
TickCount rootDuration;
TickCount localTimeOffset;
std::vector<LedColor> leds(PIXEL_COUNT, 0x00000000);

const LedCount pixelCount = PIXEL_COUNT;
const LedCount sectionLength = 50; // length of one string of LEDs
const LedCount sectionCount = pixelCount / sectionLength;
const LedCount halfSectionLength = sectionLength / 2;



enum GAME_STATE
{
  GS_ANIMATING,
  GS_SLIDER,
};

GAME_STATE gameState = GS_ANIMATING;

int sliderStartEncoderValue;

// Scale factor to compensate a litte for the power problems on the long string of leds
const float powerScale = 0.4f;

#ifdef NO
const LedColor color_dark_red = ScaleColor(powerScale, color_red);
const LedColor color_dark_green = ScaleColor(powerScale, color_green);
const LedColor color_dark_blue = ScaleColor(powerScale, color_blue);
const LedColor color_dark_white = ScaleColor(powerScale, color_white);
#endif

GameEngine* pGameEngine;



// setup() runs once, when the device is first turned on.
void setup()
{
  // Setup the rotary encoder
  pinMode(EncoderAPin, INPUT_PULLUP);                                     
  pinMode(EncoderBPin, INPUT_PULLUP);                                     
  oldEncoderState = GetEncoderState();
  encoderValue = 0;
  attachInterrupt(EncoderAPin, EncoderChangeInterrupt, CHANGE);      
  attachInterrupt(EncoderBPin, EncoderChangeInterrupt, CHANGE); 


  // Setup the LED strip
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'


  // setup the animations for when we aren't playing the game
  rootAnimator = new AnimatorGroup();
  const bool test = false;

  // Pretend to be normal christmas lights
  {
    std::vector<LedColor> pattern = {color_white, color_black, color_black, color_green, color_black, color_black, color_blue, color_black, color_black, color_red, color_black, color_black};
    ScalePattern(powerScale, pattern);
    const TickCount duration = (test ? 1 : 30) * 1000;
      
    auto pRep = new RepeatedPatternAnimator(duration, 0, pixelCount, pattern);
    auto pFade = new FadeAnimator(1000, duration - 2000, 1000, 0, pixelCount, pRep);
    rootAnimator->AppendAnimator(pFade);
  }
  
  // All leds black for a second
  rootAnimator->AppendAnimator(new SolidColor(1000, color_black, 0, pixelCount));

  rootDuration = rootAnimator->duration();
  localTimeOffset = millis2();


  pGameEngine = new GameEngine();

}

// loop() runs over and over again, as quickly as it can execute.
void loop()
{
  EncoderStateUpdate();
  
  switch(gameState)
  {
    case GS_ANIMATING:
    {
      unsigned long localTime = millis2() - localTimeOffset;
      if(localTime > rootDuration)
      {
          localTimeOffset = millis2();
          localTime = 0;
      }

      rootAnimator->Step(localTime, leds.data());

      if(EncoderMoved())
      {
        sliderStartEncoderValue = encoderValue;
        gameState = GS_SLIDER;
      }
    }
    break;

    case GS_SLIDER:
    {
      int ledIndex = (encoderValue - sliderStartEncoderValue) % 40;
      if(ledIndex < 0)
      {
        ledIndex = 40 + ledIndex;
      }
      leds[ledIndex] = color_yellow;

      if(EncoderIdle())
      {
        gameState = GS_ANIMATING;
      }
    }
  }

  unsigned int pixelIndex = 0;
  for(auto c = leds.begin(); c != leds.end(); ++c, ++pixelIndex)
  {
      strip.setPixelColor(pixelIndex, *c);
  }
  strip.show();
  fill(leds.begin(), leds.end(), 0x00000000);

}

unsigned long millis2()
{
    return millis() * 4;
}



void AddColorWipe(AnimatorGroup* pGroup, TickCount fillDuration, LedColor startColor, LedColor endColor)
{
    //const TickCount fillDuration = (test ? 1 : 4) * 1000;
    //const float ticksPerLed = (float) fillDuration / (float)pixelCount;
    AnimatorGroup* pOne = new AnimatorGroup();
      
    pOne->AddAnimator(new SolidColor(fillDuration, 0, pixelCount, startColor), 0);
    pOne->AddAnimator(new FillInAnimator(fillDuration, 0, pixelCount, endColor), 0);

    pGroup->AppendAnimator(pOne);
}

inline unsigned int GetEncoderState() {return digitalRead(EncoderAPin) | (digitalRead(EncoderBPin) << 1);}

void EncoderChangeInterrupt()
{                                
  // No debouncing code here - this requires the encoder input to be clean
  // Also assumes we never miss a transition

  unsigned int newEncoderState = GetEncoderState();
  
  if(newEncoderState == oldEncoderState)
    return;

  int delta = -1;
  switch(oldEncoderState)
  {
    case 0:
      if(1 == newEncoderState) delta = 1;
      break;
    case 1:
      if(3 == newEncoderState) delta = 1;
      break;
    case 2:
      if(0 == newEncoderState) delta = 1;
      break;
    case 3:
      if(2 == newEncoderState) delta = 1;
      break;
  }

  encoderValue += delta;
  oldEncoderState = newEncoderState;
}

int lastEncoderValue = 0;
unsigned long lastEncoderMoveTime = 0;
const int minEncoderMoveDelta = 4; // could probably be 1 with the encoder we're using
const unsigned long encoderIdleDelta = 1000 * 4;
bool encoderMoved = false;

void EncoderStateUpdate()
{
  encoderMoved = false;
  int encoderDelta = abs(lastEncoderValue - encoderValue);
  if(encoderDelta > minEncoderMoveDelta)
  {
    encoderMoved = true;
    lastEncoderMoveTime = millis2();
    lastEncoderValue = encoderValue;
  }
}

bool EncoderMoved()
{
  return encoderMoved;
}

bool EncoderIdle()
{
  return lastEncoderMoveTime + encoderIdleDelta < millis2();
}