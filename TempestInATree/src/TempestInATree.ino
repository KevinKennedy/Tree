/*
 * Project TempestInATree
 * Description:
 * Author:
 * Date:
 */

#include <vector>
#include <memory>

#include "Animator.h"
#include "GameEngine.h"
#include <neopixel.h>

SerialLogHandler logHandler;

const int FireButtonPin = D0;
unsigned long lastFireButtonChangedTime = 0;
int lastFireButtonChangedState = HIGH;

const int StartButtonPin = D1;
unsigned long lastStartButtonChangedTime = 0;
int lastStartButtonChangedState = HIGH;

const int EncoderAPin = D5;
const int EncoderBPin = D6;
unsigned int oldEncoderState;
int encoderValue;

const int encoderClicksPerLED = 4; // 400 encoder clicks per full rotation


#define PIXEL_COUNT 400
#define PIXEL_PIN D2
#define PIXEL_TYPE WS2812B

Adafruit_NeoPixel strip(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

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
  GS_LED_INDEX_MODE,
  GS_ANIMATING,
  GS_PLAYING,
};

GAME_STATE gameState = GS_PLAYING;
//GAME_STATE gameState = GS_LED_INDEX_MODE;

int encoderHomeValue;

// Scale factor to compensate a litte for the power problems on the long string of leds
const float powerScale = 0.4f;

#ifdef NO
const LedColor color_dark_red = ScaleColor(powerScale, color_red);
const LedColor color_dark_green = ScaleColor(powerScale, color_green);
const LedColor color_dark_blue = ScaleColor(powerScale, color_blue);
const LedColor color_dark_white = ScaleColor(powerScale, color_white);
#endif

GameEngine gameEngine;

// Used only in GS_LED_INDEX_MODE
int testLedIndex = 0;

int frameCount = 0;
TickCount nextFPSReportTime = 0;




// setup() runs once, when the device is first turned on.
void setup()
{
  // Setup the rotary encoder
  pinMode(EncoderAPin, INPUT_PULLUP);                                     
  pinMode(EncoderBPin, INPUT_PULLUP);                                     
  oldEncoderState = GetEncoderState();
  encoderValue = 0;
  encoderHomeValue = 0;
  attachInterrupt(EncoderAPin, EncoderChangeInterrupt, CHANGE);      
  attachInterrupt(EncoderBPin, EncoderChangeInterrupt, CHANGE); 

  pinMode(FireButtonPin, INPUT_PULLUP);
  pinMode(StartButtonPin, INPUT_PULLUP);

  // Setup the LED strip
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  // TODO - Put us in GS_LED_INDEX_MODE if some button is pressed at startup

  gameEngine.Start(millis2());

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
}

// loop() runs over and over again, as quickly as it can execute.
void loop()
{
  EncoderStateUpdate();
  bool firePressed = GetButtonState(FireButtonPin, millis2(), &lastFireButtonChangedTime, &lastFireButtonChangedState);
  bool startPressed = GetButtonState(StartButtonPin, millis2(), &lastStartButtonChangedTime, &lastStartButtonChangedState);

  
  switch(gameState)
  {
    case GS_LED_INDEX_MODE:
        {
          testLedIndex = ((encoderValue - encoderHomeValue) / encoderClicksPerLED) % PIXEL_COUNT;
          if(testLedIndex < 0)
          {
            testLedIndex = PIXEL_COUNT + testLedIndex;
          }
          leds[testLedIndex] = color_yellow;
        }
      break;

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
        encoderHomeValue = encoderValue;
        //gameState = GS_SLIDER;
      }
    }
    break;

    case GS_PLAYING:
      int playerPosition = ((encoderValue - encoderHomeValue) / encoderClicksPerLED) % gameEngine.GetPathLedCount();
      if(playerPosition < 0) playerPosition += gameEngine.GetPathLedCount();
      gameEngine.Step(millis2(), playerPosition, firePressed);
      gameEngine.SetLeds(leds.data());

      if(startPressed)
      {
        gameEngine.Start(millis2());
      }
      break;

  }

  unsigned int pixelIndex = 0;
  for(auto c = leds.begin(); c != leds.end(); ++c, ++pixelIndex)
  {
      strip.setPixelColor(pixelIndex, *c);
  }
  strip.show();
  fill(leds.begin(), leds.end(), 0x00000000);


  frameCount++;
  if(millis2() > nextFPSReportTime)
  {
    Log.info("FPS: %d", frameCount);
    frameCount = 0;
    nextFPSReportTime = millis2() + 1000;

    if(gameState == GS_LED_INDEX_MODE)
    {
      Log.info("LED Index: %d", testLedIndex);
    }

    //Log.info("Fire button %s", firePressed ? "down":"up");
  }
}

unsigned long millis2()
{
    return millis(); // * 4;
}

const unsigned long buttonBounceTime = 50; // in milliseconds

// true for pressed, false for not pressed.  Input is pulled-up and active low
bool GetButtonState(int inputPin, unsigned long now, unsigned long* pLastChangeTime, int* pLastChangeState)
{
  int currentState = digitalRead(inputPin);

  if(now - *pLastChangeTime > buttonBounceTime)
  {
    // it's been long enough since the last time the button changed states
    // so allow checking for a new state
    if(currentState != *pLastChangeState)
    {
      // The button state changed so start a debounce
      *pLastChangeState = currentState;
      *pLastChangeTime = now;
    }
  }
  
  return LOW == *pLastChangeState;
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