// Core Arduino libraries
#include <Arduino.h>
// Faster implementation of the pin states for fast stepper movement.
#include <digitalWriteFast.h> 

// Hardware and user config parameters.
#include "configuration.h"


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
        unsigned long microsPerStep; 
        long maxStepsPerSec;

        // Acceleration parameters
        unsigned long prevMillis = 0;
        long currentStepsPerSec = 0;
        long setStepsPerSec = 0;
        long startingStepsPerSec = 5;

        // State management
        bool stepperEnabled = false;
        bool doOnceWhenStopped = false;
        int maxInchesPerMin;

        // Time standards
        long secondsPerMin = 60;
        unsigned long microsPerSec = 1000000;

        // Hardware states from config
        int revolutionsPerInch;
        int stepsPerRevolution;

        // Stepper Driver Pins
        // [0] = PULSE
        // [1] = DIRECTION
        // [2] = ENABLE
        int controlPins[3];


    public:

        // Velocity & Accel States
        float currentInchesPerMin;


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
        
        void microsBetweenSteps() {
            this->microsPerStep = this->microsPerSec / this->currentStepsPerSec;
        }

        void accelerateTo(long stepsPerSec) {
            if (millis() - this->prevMillis >= accelInterval) { 
                //Increment timers any time we accelerate
                this->prevMillis = millis();

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
            if (millis() - this->prevMillis >= accelInterval) { 
                //Increment timers any time we accelerate
                this->prevMillis = millis();

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
            if (micros() - this->prevMicros 
                        >= 
                        this->microsPerStep - calibrationMicros) {
                //Increment timers any time we step
                this->prevMicros += this->microsPerStep;

                if (this->setStepsPerSec > 0) {
                    //Pulse the driver
                    digitalWriteFast(this->controlPins[0], HIGH);
                    delayMicroseconds(pulseWidthMicroseconds);
                    digitalWriteFast(this->controlPins[0], LOW);
                }
                
            }
        }


    public:

        //Constructor
        FastStepper(int maxIPM, int revsPerInch, int stepsPerRev) {
            this->maxInchesPerMin = maxIPM;
            this->revolutionsPerInch = revsPerInch;
            this->stepsPerRevolution = stepsPerRev;
            this->maxStepsPerSec = this->getMaxStepsPerSec(maxIPM);
        }

        void begin(int pins[]) {
            // Set pin default states
            for (int i = 0; i < 3; i++) {
                pinModeFast(pins[i], OUTPUT);
                this->controlPins[i] = pins[i];
            }

            // Pull the pulse pin low to set the default state
	        digitalWriteFast(this->controlPins[0], LOW);

            // Pull the direction pin low to set the default direction
	        digitalWriteFast(this->controlPins[1], LOW);
	
            // Pull the enable pin high to disable the driver by default
            digitalWriteFast(this->controlPins[2], HIGH);
        }

        void setSpeed(float inchesPerMin) {
            this->setStepsPerSec = this->calcStepsPerSec(inchesPerMin);
            if (DEBUG) {
                Serial.print("Speed Initialized: ");
                Serial.print(inchesPerMin);
                Serial.println(" IPM");
            } 
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

        void run() {
            if (!this->stepperEnabled) {
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
    private:
        // Don't read on every loop
        unsigned long lastReadMillis = 0;
        
        // Debounce params
        unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
        unsigned long debounceDelay;    // millis, increase during init if still bouncy

        // State management
        int lastSwitchState = LOW;
        int currSwitchState = LOW;

        // Safety Flag - Cannot start if switch is on at boot
        bool runEnabled = false;

        // Stepper Object References
        FastStepper* feedMotor;

        // Hardware config
        int LEFT_PIN;
        int RIGHT_PIN;

    public:
        // Constructor
        ThreeWaySwitch(FastStepper* motor, int delay) {
            this->feedMotor = motor;
            this->debounceDelay = delay;
        }

        void begin(int pins[]) {
            String readyState;

            // Set pins' default states
            this->LEFT_PIN = pins[0];
            pinModeFast(this->LEFT_PIN, INPUT);

            this->RIGHT_PIN = pins[1];
            pinModeFast(this->RIGHT_PIN, INPUT);

            // Safety Interlock - Do not start at boot
            // User must switch to middle (disabled) direction 
            // position before motor will run.
            if (digitalReadFast(this->RIGHT_PIN) != HIGH
                    &&
                    digitalReadFast(this->LEFT_PIN) != HIGH) {
                readyState = "Ready";
                this->runEnabled = true;
            } 
            else {  // Switch is on at boot, disable switch until reset.
                readyState = "Please Set Direction to Middle";
            }

            if (DEBUG) {
                Serial.println("Direction Initialized: " + readyState);
            } 
        }

        void read() {
            if ((millis() - this->lastReadMillis) > SWITCHREADDELAY) {
                this->lastReadMillis += SWITCHREADDELAY;

                // Debounce doesn't care which side it's switched to
                // We're looking for change in state from low to high
                // on either pin
                int switchReading;
                
                // Using elseif here due to delays caused by sequential reads
                if (digitalReadFast(this->RIGHT_PIN) == HIGH
                        || 
                        digitalReadFast(this->LEFT_PIN) == HIGH) {
                    switchReading = HIGH;
                }
                else {
                    switchReading = LOW;
                }

                // Switch state changed, is it bouncing?
                if (switchReading != this->lastSwitchState) {
                    this->lastDebounceTime = millis(); // Reset the timer
                    if (DEBUG) {
                        Serial.print(".");  // Visualize bouncing in monitor
                    } 
                }
    
                if ((millis() - this->lastDebounceTime) > this->debounceDelay) {
                    // Button has changed, not bouncing anymore
                    if (switchReading != this->currSwitchState) {
                        this->currSwitchState = switchReading; // Reset the state

                        if (this->currSwitchState == HIGH) { // Switch is on
                            // TODO: Add LCD Status Symbols
                            
                            if (digitalReadFast(this->RIGHT_PIN) == HIGH) {
                                this->feedMotor->setDirection(1); // Clockwise (relative)
                            }
                            else if (digitalReadFast(this->LEFT_PIN) == HIGH) {
                                this->feedMotor->setDirection(0); // Counter-Clockwise (relative)
                            }

                            if (DEBUG) {
                                if (this->runEnabled) {
                                    Serial.println("Direction Switch: ON");
                                }
                                else {
                                    Serial.println("Direction Switch: Suppressed for Safety");
                                }
                            } 
                        }
                        else { // Switch is off
                            // Enable safety flag if it was previously suppressed
                            if (!this->runEnabled) {
                                this->runEnabled = true;
                            }
                            
                            if (DEBUG) {
                                Serial.println("Direction Switch: OFF");
                            }
                        }
                    }
                } 
            this->lastSwitchState = switchReading;  // Store the state
            }
        }

        void run() {
            if (this->runEnabled && this->currSwitchState == HIGH) { // Safety Flag is safe and switch is on, not bouncing.
                this->feedMotor->run();
            }
            else {
                this->feedMotor->stop();
            }
        }
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
        // Don't read on every loop
        unsigned long lastReadMillis = 0;

        // Debounce params
        unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
        unsigned long debounceDelay;    // millis, increase during init if still bouncy

        // State management
        int lastButtonState = LOW;
        int currButtonState = LOW;

        // Stepper Object References
        FastStepper* feedMotor;

        // Velocity state management
        // Temporary: This will eventually be managed by the 
        // rotary encoder/LCD class
        float inchesPerMin;

        // Hardware config
        int INPUT_PIN;

    public:
        // Constructor
        MomentarySwitch(FastStepper* motor, int delay) {
            this->feedMotor = motor;
            this->debounceDelay = delay;
        };

        void begin(int pin, float inchesPerMin) {
            // Set pin default state
            this->INPUT_PIN = pin;
            pinModeFast(this->INPUT_PIN, INPUT);
            
            // Cache velocity as set
            this->inchesPerMin = inchesPerMin;

            if (DEBUG) {
                Serial.print("Rapid Initialized: ");
                Serial.print(MAXINCHESPERMIN);
                Serial.println(" IPM");
            } 
        }

        void read() {
            // Only read once every N millis, not on every loop
            if ((millis() - this->lastReadMillis) > SWITCHREADDELAY) {
                this->lastReadMillis += SWITCHREADDELAY;

                int buttonReading = digitalReadFast(this->INPUT_PIN);

                // Switch state changed, is it bouncing?
                if (buttonReading != this->lastButtonState) {
                    this->lastDebounceTime = millis(); // Reset the timer
                    if (DEBUG) {
                        Serial.print(".");
                    } 
                }

                if ((millis() - this->lastDebounceTime) > this->debounceDelay) {
                    // Button has changed, not bouncing anymore
                    if (buttonReading != this->currButtonState) {
                        this->currButtonState = buttonReading; // Reset the state

                        if (this->currButtonState == HIGH) {
                            this->feedMotor->setSpeed(MAXINCHESPERMIN);

                            if (DEBUG) {
                                Serial.println("Speed: RAPID");
                            } 
                        }
                        else {
                            this->feedMotor->setSpeed(inchesPerMin);
                            
                            if (DEBUG) {
                                Serial.println("Speed: SLOW");
                            }
                        }
                    }
                }
                // Save the state
                this->lastButtonState = buttonReading;
            }
        }
};

FastStepper stepper(MAXINCHESPERMIN, REVSPERINCH, STEPSPERREV);
ThreeWaySwitch directionSwitch(&stepper, DEBOUNCEMILLIS3WAY);
MomentarySwitch rapidButton(&stepper, DEBOUNCEMILLISMOMENTARY);

// Interrupt-Based Rotary Encoder Functions
#include <RotaryEncoder.h>

void setup() {
    if (DEBUG) { // Log Events to Serial Monitor
        Serial.begin(9600);  
    }

    // Set defaults
    stepper.setSpeed(encodedInchesPerMin);

    // Initialize the pin outputs/inputs
    stepper.begin(stepperControlPins);
    directionSwitch.begin(threeWayPins);
    rapidButton.begin(RAPID_PIN, encodedInchesPerMin);

    // Set up Rotary Encoder Interrupts
    beginRotaryEncoder();
}


void loop() { 
    directionSwitch.run();
    directionSwitch.read();
    rapidButton.read();
    logRotaryEncoder();
}