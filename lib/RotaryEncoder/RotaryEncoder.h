/*******Interrupt-based Rotary Encoder Logic *******
* Based on code by Simon Merrett, who based his sketch on insight from 
* Oleg Mazurov, Nick Gammon, rt, Steve Spence
* Retreived from: https://www.instructables.com/Improved-Arduino-Rotary-Encoder-Reading/
*/

// Lets us know when we're expecting a rising edge on 
// rotaryPinA to signal that the encoder has arrived at a detent
volatile byte aFlag = 0;

// Lets us know when we're expecting a rising edge on 
// rotaryPinB to signal that the encoder has arrived at a detent 
// (opposite direction to when aFlag is set)
volatile byte bFlag = 0; 

// This variable stores our current value of encoder position. 
// Change to int or uin16_t instead of byte if you want to record a larger range than 0-255
volatile byte encoderPos = 0; 

// Stores the last encoder position value so we can compare to 
// the current reading and see if it has changed 
// (so we know when to print to the serial monitor)
volatile byte oldEncPos = 0; 

// Somewhere to store the direct values we read from our 
// interrupt pins before checking to see if we have moved a whole detent
volatile byte reading = 0; 

void PinA() {
  cli(); //stop interrupts happening before we read pin values
  reading = PIND & 0xC; // read all eight pin values then strip away all but rotaryPinA and rotaryPinB's values
  if(reading == B00001100 && aFlag) { //check that we have both pins at detent (HIGH) and that we are expecting detent on this pin's rising edge
    encoderPos --; //decrement the encoder's position count

    encodedInchesPerMin -= 0.25;
    // Lower bounds
    if (encodedInchesPerMin <= 0.00) {
      encodedInchesPerMin = 0.00;
    }
    stepper.setSpeed(encodedInchesPerMin);

    bFlag = 0; //reset flags for the next turn
    aFlag = 0; //reset flags for the next turn
  }
  else if (reading == B00000100) bFlag = 1; //signal that we're expecting rotaryPinB to signal the transition to detent from free rotation
  sei(); //restart interrupts
}

void PinB() {
  cli(); //stop interrupts happening before we read pin values
  reading = PIND & 0xC; //read all eight pin values then strip away all but rotaryPinA and rotaryPinB's values
  if (reading == B00001100 && bFlag) { //check that we have both pins at detent (HIGH) and that we are expecting detent on this pin's rising edge
    encoderPos ++; //increment the encoder's position count
    
    encodedInchesPerMin += 0.25;
    // Upper bounds
    if (encodedInchesPerMin >= MAXINCHESPERMIN) {
      encodedInchesPerMin = MAXINCHESPERMIN;
    }
    stepper.setSpeed(encodedInchesPerMin);

    bFlag = 0; //reset flags for the next turn
    aFlag = 0; //reset flags for the next turn
  }
  else if (reading == B00001000) aFlag = 1; //signal that we're expecting rotaryPinA to signal the transition to detent from free rotation
  sei(); //restart interrupts
}

void beginRotaryEncoder() {
  pinMode(rotaryPinA, INPUT_PULLUP); // set rotaryPinA as an input, pulled HIGH to the logic voltage (5V or 3.3V for most cases)
  pinMode(rotaryPinB, INPUT_PULLUP); // set rotaryPinB as an input, pulled HIGH to the logic voltage (5V or 3.3V for most cases)
  attachInterrupt(digitalPinToInterrupt(rotaryPinA), PinA, RISING); // set an interrupt on PinA, looking for a rising edge signal and executing the "PinA" Interrupt Service Routine (above)
  attachInterrupt(digitalPinToInterrupt(rotaryPinB), PinB, RISING); // set an interrupt on PinB, looking for a rising edge signal and executing the "PinB" Interrupt Service Routine (above)
}

/* void logRotaryEncoder() {
  if (DEBUG) {
    if(oldEncPos != encoderPos) {
      Serial.print(encodedInchesPerMin, 2);
      Serial.println(" Inches/Min");
      oldEncPos = encoderPos;
    }
  }
} */

