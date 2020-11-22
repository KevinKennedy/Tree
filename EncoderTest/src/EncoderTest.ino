/*
 * Project EncoderTest
 * Description:
 *  Quick app to read my rotary encoder - a Taiss/AB 2 Phase Incremental Rotary Encoder ordered off of Amazon
 *  on a Particle Photon
 *  Probably many efficiency gains to be had here :-)
 * Author: Kevin Kennedy
 * Date:
 */

const int EncoderAPin = D5;
const int EncoderBPin = D6;

unsigned int oldEncoderState;
int encoderValue;


SerialLogHandler logHandler;

void setup()
{
  pinMode(EncoderAPin, INPUT_PULLUP);                                     
  pinMode(EncoderBPin, INPUT_PULLUP);                                     
  oldEncoderState = GetEncoderState();
  encoderValue = 0;
  attachInterrupt(EncoderAPin, EncoderChangeInterrupt, CHANGE);      
  attachInterrupt(EncoderBPin, EncoderChangeInterrupt, CHANGE); 
}

void loop()
{

  delay(100);
  Log.info("oldEncoderState: %d   encoderValue: %d", oldEncoderState, encoderValue);

}

// Faster way to read bits?  PORTD?
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