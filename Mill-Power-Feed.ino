// Hardware and user config parameters.
#include "configuration.h"

// Faster implementation of the pin states for fast stepper movement.
#include <digitalWriteFast.h> 

/**
* Fast stepper class for driving the power feed.
* ----------------------------------------------
*
* Unsigned long and long ints are often used due to the large numbers
* used in microsecond integer math.  Since this is not CNC precision, we
* are not worried about remainders or floats, since the math for these is so
* expensive in contrast to integer math.
* All to ensure the motor can rapid quickly, as much as 1,200 RPM or higher
* even when microstepping (within some limitations.)
*/
class FastStepper {

    private: 
        // Timing and pulse variables
        unsigned long prevMicros = 0;
        long microsPerStep; 
        long maxStepsPerSec;

        // Acceleration parameters
        unsigned long prevMillis = 0;
        long currentStepsPerSec = 0;
        long setStepsPerSec = 0;
        long startingStepsPerSec = 50;

        // State management
        bool stepperEnabled = false;
        bool doOnceWhenStopped = false;
        int maxInchesPerMin;

        // Time standards
        long secondsPerMin = 60;
        long microsPerSec = 1000000;

        // Hardware states from config
        int revolutionsPerInch;
        int stepsPerRevolution;

        // Stepper Driver Pins
        // [0] = PULSE
        // [1] = DIRECTION
        // [2] = ENABLE
        int controlPins[3];


    public:
        // Timing and acceleration timers
        unsigned long curMicros;
        unsigned long curMillis;

        // Velocity & Accel States
        float currentInchesPerMin;



    //Constructor
    FastStepper(int maxIPM, int revsPerInch, int stepsPerRev) {
        this->maxInchesPerMin = maxIPM;
        this->revolutionsPerInch = revsPerInch;
        this->stepsPerRevolution = stepsPerRev;
        this->maxStepsPerSec = this->getMaxStepsPerSec(maxIPM);
    }

    private:

        long getMaxStepsPerSec(float maxIPM) {
            return this->calcStepsPerSec(maxIPM);
        }

        long calcStepsPerSec(float inchesPerMin) {
            // Using minutes because the truncated remainders of 
            // larger numbers are less significant.
            unsigned long RPM = inchesPerMin * this->revolutionsPerInch;
            unsigned long stepsPerMin = RPM * this->stepsPerRevolution;
            unsigned long stepsPerSec = stepsPerMin / this->secondsPerMin;

            return stepsPerSec;
        }
        
        unsigned long microsBetweenSteps() {
            this->microsPerStep = this->microsPerSec / this->currentStepsPerSec;
        }

        void accelerateTo(long stepsPerSec) {
            if (this->curMillis - this->prevMillis >= accelInterval) { 
                //Increment timers any time we accelerate
                this->prevMillis = this->curMillis;

                // Accelerate until maxed
                if (this->currentStepsPerSec > stepsPerSec) {
                    // we overshot slightly on the last loop, correct it
                    this->currentStepsPerSec = stepsPerSec;
                }
                else {
                    this->currentStepsPerSec += accelRate;
                }
                
                this->microsBetweenSteps();
            }
        }

        void decelerateTo(long stepsPerSec) {
            if (this->curMillis - this->prevMillis >= accelInterval) { 
                //Increment timers any time we accelerate
                this->prevMillis = this->curMillis;

                // Decelerate until set speed or 0
                if (this->currentStepsPerSec < stepsPerSec) {
                    // we overshot slightly on the last loop, correct it
                    this->currentStepsPerSec = stepsPerSec;
                }
                else {
                    this->currentStepsPerSec -= accelRate;
                }
                
                this->microsBetweenSteps();
            }
        }

        void step() {
            // If the elapsed time since last step is enough, step again
            if (this->curMicros - this->prevMicros 
                        >= 
                        this->microsPerStep - calibrationMicros) {
                //Increment timers any time we step
                this->prevMicros = this->curMicros;

                //Pulse the driver
                digitalWriteFast(this->controlPins[0], HIGH);
                delayMicroseconds(pulseWidthMicroseconds);
                digitalWriteFast(this->controlPins[0], LOW);
            }
        }

    public:

        void begin(int pins[]) {
            // Set pin default states
            for (int i = 0; i < 3; i++) {
                pinModeFast(pins[i], OUTPUT);
                this->controlPins[i] = pins[i];
            }
        }

        void setSpeed(float inchesPerMin) {
            this->setStepsPerSec = this->calcStepsPerSec(inchesPerMin);
        }

        void setDirection(bool direction) {
            if (!direction) { // Any falsy value deenergizes the direction pin, let's call this the default direction.
                digitalWriteFast(this->controlPins[1], LOW);
                
                if (DEBUG) {
                    Serial.println("Direction: Left");
                }   
            }
            else {  
                digitalWriteFast(this->controlPins[1], HIGH);

                if (DEBUG) {
                    Serial.println("Direction: Right");
                }  
            }
        }

        void run(bool direction) {
            if (!this->stepperEnabled) {
                //delay(50); //Debounce on state change, for bouncy switches
                this->setDirection(direction);
                this->stepperEnabled = true;
                if (this->currentStepsPerSec < this->startingStepsPerSec) {
                    this->currentStepsPerSec = this->startingStepsPerSec; // Set minimum startup speed
                }
                
                digitalWriteFast(this->controlPins[2], LOW); // Enable the driver

                if (DEBUG) {
                    Serial.println("RUN:");
                    Serial.print("Max Steps / Sec: "); Serial.println(this->maxStepsPerSec);
                    Serial.print("Set Steps / Sec: "); Serial.println(this->setStepsPerSec);
                    Serial.print("Enabled: "); Serial.println(this->stepperEnabled);
                    Serial.println();
                }                
            }

            // If it's not already at the set speed, keep accelerating
            if (this->currentStepsPerSec < this->setStepsPerSec) {
                this->accelerateTo(this->setStepsPerSec);      
            }
            // If a new slower speed was entered, decelerate to it
            else if (this->currentStepsPerSec > this->setStepsPerSec) {
                this->decelerateTo(this->setStepsPerSec);
            }
                
            this->step();           
        }

        void stop() {
            // Manage states on change
            if (this->stepperEnabled) {  
                this->doOnceWhenStopped = true;  // Set flag, used to reduce redundant processing.
                this->stepperEnabled = false;
            }
            
            // If it's still moving, decelerate to 0
            if (this->currentStepsPerSec > 0) {
                this->decelerateTo(0);
                this->step();
            }
            else if (this->doOnceWhenStopped && this->currentStepsPerSec <= 0) { // Stopped, do these things once.
                this->doOnceWhenStopped = false;  // Reset flag, used to reduce redundant processing.
                digitalWriteFast(this->controlPins[2], HIGH); // Disable the driver
                digitalWriteFast(this->controlPins[0], LOW); // Make sure the pulse pin is off

                if (DEBUG) {
                    Serial.println("STOP:");
                    Serial.print("Max Steps / Sec: "); Serial.println(this->maxStepsPerSec);
                    Serial.print("Set Steps / Sec: "); Serial.println(this->setStepsPerSec);
                    Serial.print("Enabled: "); Serial.println(this->stepperEnabled);
                    Serial.println();
                } 
            } 
        }
};

/**
 * Three-Way-Switch class for changing the direction of the power feed
 * --------------------------------------------------------------------
 * 
 * The particular switch I'm using is very bouncy, and needs async monitoring
 * to debounce it and mitigate any bugs related to bouncing.
 */ 
class ThreeWaySwitch {


};

/**
 * Momentary-Switch class for changing RPMs to max for 'rapid' function
 * --------------------------------------------------------------------
 * 
 * Real-time monitoring of the momentary switch has negative consequences to
 * stepper pulse and RPM accuracy.  Containing states of this switch in an
 * object mitigates these performance concerns.
 */ 
class MomentarySwitch {
    private:
        // Debounce params
        unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
        unsigned long debounceDelay = 50;    // millis, increase if still bouncy

        // State management
        int currButtonState;
        int prevButtonState = LOW;

        // Stepper Object References
        FastStepper &feedMotor;
        float inchesPerMin;

        // Hardware config
        int INPUT_PIN;

    public:
        // Debounce params
        unsigned long curMillis;

        // Constructor
        MomentarySwitch() {};

        void begin(int pin, FastStepper* motor, float inchesPerMin) {
            // Set pin default state
            pinModeFast(pin, INPUT);
            this->INPUT_PIN = pin;

            // Set object references
            this->feedMotor = *motor;
            this->inchesPerMin = inchesPerMin;
        }

        void readButton() {
            int buttonReading = digitalReadFast(INPUT_PIN);

            // Switch state changed, is it bouncing?
            if (buttonReading != prevButtonState) {
                lastDebounceTime = millis(); // Reset the timer
            }

            if ((millis() - lastDebounceTime) > debounceDelay) {
                // Button has changed, not bouncing anymore
                if (buttonReading != currButtonState) {
                    currButtonState = buttonReading; // Reset the state

                    if (currButtonState == HIGH) {
                        feedMotor.setSpeed(MAXINCHESPERMIN);

                        if (DEBUG) {
                            Serial.println("Speed: RAPID");
                        } 
                    }
                    else {
                        feedMotor.setSpeed(inchesPerMin);
                        
                        if (DEBUG) {
                            Serial.println("Speed: RAPID");
                        }
                    }
                }
            }
        }
};

FastStepper feedMotor(MAXINCHESPERMIN, REVSPERINCH, STEPSPERREV);
MomentarySwitch rapidButton;


float encodedInchesPerMin = 2;

void setup() {
    if (DEBUG) { // Debugging
        Serial.begin(9600);  
    }

    // Set defaults
    feedMotor.setSpeed(encodedInchesPerMin);

    //Initialize the pin outputs
    feedMotor.begin(controlPins);
    rapidButton.begin(RAPID_PIN, &feedMotor, encodedInchesPerMin);

    // Set pin default states
	pinModeFast(MOVELEFT_PIN, INPUT);
	pinModeFast(MOVERIGHT_PIN, INPUT);
	
    // Pull the enable pin high to disable the driver by default
	digitalWriteFast(ENABLE_PIN, HIGH);

	// Pull the direction pin low to set the default direction
	digitalWriteFast(DIRECTION_PIN, LOW);

    // Pull the pulse pin low to set the default state
	digitalWriteFast(PULSE_PIN, LOW);
}


void loop() { 
    feedMotor.curMicros = micros();
    feedMotor.curMillis = millis();
    readSwitches();
    rapidButton.readButton();
}

void readSwitches() {    
    if (digitalReadFast(MOVERIGHT_PIN) == HIGH) {
        feedMotor.run(1); // Clockwise
    }
    else if (digitalReadFast(MOVELEFT_PIN) == HIGH) {
        feedMotor.run(0); // Counter-Clockwise
    }
    else {  // Disable the motor when the switch is centered and the rapid isn't pressed.
        feedMotor.stop();
    }
}