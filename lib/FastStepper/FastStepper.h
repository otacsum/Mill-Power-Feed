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
        unsigned long microsPerStep = 60606;

        // Acceleration parameters
        unsigned long prevMillis = 0;
        long currentStepsPerSec = 0;
        long setStepsPerSec = 0;

        // State management
        bool doOnceWhenStopped = false;
        int maxInchesPerMin = 0;

        

        // Hardware states from config
        int revolutionsPerInch;
        int stepsPerRevolution;

        // Stepper Driver Pins
        // [0] = PULSE
        // [1] = DIRECTION
        // [2] = ENABLE
        int controlPins[3];

    public:

        // State management accessed by other classes
        float currentInchesPerMin = 0.0;
        bool stepperEnabled = false;
        bool paused = false;


    private:

        long calcStepsPerSec(float inchesPerMin) {
            // Using minutes because the truncated remainders of 
            // larger numbers are less significant.
            unsigned long RPM = inchesPerMin * this->revolutionsPerInch;
            unsigned long stepsPerMin = RPM * this->stepsPerRevolution;
            unsigned long stepsPerSec = stepsPerMin / secondsPerMin;

            return stepsPerSec;
        }
        
        void microsBetweenSteps() {
            if (this->currentStepsPerSec > 0) {
                noInterrupts();
                this->microsPerStep = microsPerSec / this->currentStepsPerSec;
                if (this->microsPerStep < minMicrosPerStep) { // If it's trying to run faster than max.
                    this->microsPerStep = microsPerStep; // Error catching for runaway math.
                }
                interrupts();
            }
            else {  // Somehow steps/sec got negative, error correct to slowest speed.
                noInterrupts();
                this->microsPerStep = startMicrosPerStep;  // Error catching, rarely if ever executes
                interrupts();
            }
        }

        void accelerateTo(long stepsPerSec) {
            unsigned long currLoopMillis = millis();
            noInterrupts();
            long timeSinceLastAccel = currLoopMillis - this->prevMillis;
            if (timeSinceLastAccel >= accelInterval) { 
                //Increment timers any time we accelerate
                //this->prevMillis = currLoopMillis;
                this->prevMillis += accelInterval;
                
                // Current speed is zero, or too slow to accel with torque so..
                // jerk up to minimum speed then start accelerating.
                if (this->microsPerStep < startMicrosPerStep) {
                    this->microsPerStep = startMicrosPerStep; // Set minimum startup speed
                    digitalWriteFast(this->controlPins[2], LOW); // Enable the driver once we have step delay
                }
                // Accelerate until maxed
                else if (this->currentStepsPerSec < stepsPerSec) {
                    this->currentStepsPerSec += accelRate;
                }
                
                if (this->currentStepsPerSec > stepsPerSec) {
                    // we overshot slightly on the last loop, correct it
                    this->currentStepsPerSec = stepsPerSec;
                }

                if (DEBUG) {
                    Serial.print("Accel: ");
                    Serial.print(this->currentStepsPerSec);
                    Serial.println(" Steps/sec");
                } 
                
                this->microsBetweenSteps(); // Calc and store the delay between pulses
                interrupts();
            }
        }

        void decelerateTo(long stepsPerSec, bool fast = false) {
            if (millis() - this->prevMillis >= accelInterval) { 
                //Increment timers any time we accelerate
                this->prevMillis = millis();

                // Decelerate until set speed or 0
                if (this->currentStepsPerSec < stepsPerSec) {
                    // we overshot slightly on the last loop, correct it
                    this->currentStepsPerSec = stepsPerSec;
                }
                else {
                    /* if (fast) {
                        this->currentStepsPerSec -= (accelRate * 2);
                    } 
                    else {*/
                        this->currentStepsPerSec -= accelRate * 2;
                    //}
                    
                }

                // Once stopped, shut things down.
                if (this->currentStepsPerSec <= 0) {
                    this->currentStepsPerSec = 0; // Catch for overshooting stop.
                    this->microsPerStep = minMicrosPerStep;  // Reset to min speed delay.
                    //TODO: Redundant digitalWriteFast(this->controlPins[2], HIGH); // Disable the driver once we stop
                }
                else {
                    this->microsBetweenSteps(); // Calc and store the delay between pulses
                }

                if (DEBUG) {
                    Serial.print("Decel: ");
                    Serial.print(this->currentStepsPerSec);
                    Serial.println(" Steps/sec");
                }
                
                
                
            } 
        }

        void step() {
            long timeSinceLastPulse = micros() - this->prevMicros;
            noInterrupts();
            // If the elapsed time since last step is enough, or the timer rolled over, step again
            if (timeSinceLastPulse >= signed(this->microsPerStep) || timeSinceLastPulse <= 0) { 
                //Increment timers any time we step
                if (timeSinceLastPulse < 0) {
                    this->prevMicros = micros(); // Rollover handling, restart the timer.
                }
                else {
                    this->prevMicros += this->microsPerStep; // Increment the delay
                }

                
                if (this->currentStepsPerSec > 0 && this->microsPerStep > minMicrosPerStep) {
                    //Pulse the driver
                    digitalWriteFast(this->controlPins[0], HIGH);
                    delayMicroseconds(pulseWidthMicroseconds);
                    digitalWriteFast(this->controlPins[0], LOW);
                }
                
            }
            interrupts();
        }


    public:

        //Constructor
        FastStepper(int maxIPM, int revsPerInch, int stepsPerRev) {
            this->maxInchesPerMin = maxIPM;
            this->revolutionsPerInch = revsPerInch;
            this->stepsPerRevolution = stepsPerRev;
        }

        void begin(int pins[]) {
            // Set pin default states
            for (int i = 0; i < 3; i++) {
                pinModeFast(pins[i], OUTPUT);
                this->controlPins[i] = pins[i];
            }

            // Pull the pulse pin low
	        digitalWriteFast(this->controlPins[0], LOW);

            // Pull the direction pin low to set the default direction
	        digitalWriteFast(this->controlPins[1], LOW);

            // Pull the enable pin low to enable the driver for a single pulse
            digitalWriteFast(this->controlPins[2], LOW);

            // Pulse the motor pin once to reduce ambient noise
	        digitalWriteFast(this->controlPins[0], HIGH);
            delay(5);
	        digitalWriteFast(this->controlPins[0], LOW);

            // Set default speed (0).
            this->setSpeed(encodedInchesPerMin);
            this->stop();
	
            // Pull the enable pin high to disable the driver by default
            digitalWriteFast(this->controlPins[2], HIGH);
        }

        void setSpeed(float inchesPerMin) {
            if (this->paused) {  // Reset paused state when speed changes
                this->paused = !this->paused;
            }
            this->setStepsPerSec = this->calcStepsPerSec(inchesPerMin);
            lcdMessage.writeSpeed(inchesPerMin);
            if (DEBUG) {
                Serial.print("Speed Set: ");
                Serial.print(inchesPerMin);
                Serial.print(" IPM | ");
                Serial.print(this->setStepsPerSec);
                Serial.println(" Steps/sec");
            } 
        }

        void setDirection(bool direction) {
            if (!direction) { // Any falsy value deenergizes the direction pin, let's call this the default direction.
                digitalWriteFast(this->controlPins[1], LOW);
                
                if (DEBUG) {
                    Serial.println("Direction: Right");
                }   
            }
            else {  
                digitalWriteFast(this->controlPins[1], HIGH);

                if (DEBUG) {
                    Serial.println("Direction: Left");
                }  
            }
        }

        void run() {
            if (!this->stepperEnabled) {
                this->stepperEnabled = true;  // State management only
                // Set initial speed once based on encoded value
                this->setSpeed(encodedInchesPerMin);

                if (DEBUG) {
                    Serial.println("RUN:");
                    Serial.print("Max Steps / Sec: "); Serial.println(maxStepsPerSec);
                    Serial.print("Set Steps / Sec: "); Serial.println(this->setStepsPerSec);
                    Serial.print("Enabled: "); Serial.println(this->stepperEnabled);
                    Serial.println();
                }                
            }

            // Stopped but set for a speed OR running but not at full speed
            // Accelerate to, and maintain speed
            if (this->setStepsPerSec > 0 && this->currentStepsPerSec <= this->setStepsPerSec) {
                this->accelerateTo(this->setStepsPerSec);    
            }
            // If a new slower speed was entered, decelerate to it
            else if (this->currentStepsPerSec > 0 && this->currentStepsPerSec > this->setStepsPerSec) {
                this->decelerateTo(this->setStepsPerSec);
            }

            if (!DEBUG) {
                this->step(); // Stepping is pointless when debugging, too many blocking signals.
            }
                
        }

        void stop(bool fast = false) {
            // Manage states on change
            if (this->stepperEnabled) {  
                this->doOnceWhenStopped = true;  // Set flag, used to reduce redundant processing.
                this->stepperEnabled = false;
            }
            
            // If it's still moving, decelerate to 0
            if (this->currentStepsPerSec > 0) {
                this->decelerateTo(0, fast);
                if (!DEBUG) {
                    this->step(); // Stepping is pointless when debugging, too many blocking signals.
                }
            }
            else if (this->doOnceWhenStopped && this->currentStepsPerSec <= 0) { // Stopped, do these things once.
                this->doOnceWhenStopped = false;  // Reset flag, used to reduce redundant processing.
                digitalWriteFast(this->controlPins[2], HIGH); // Disable the driver
                digitalWriteFast(this->controlPins[0], LOW); // Make sure the pulse pin is off
                
                if (this->paused) {  // Reset paused state
                    this->paused = !this->paused;
                    lcdMessage.writeSpeed(encodedInchesPerMin);
                }

                if (DEBUG) {
                    Serial.println("STOP:");
                    Serial.print("Max Steps / Sec: "); Serial.println(maxStepsPerSec);
                    Serial.print("Set Steps / Sec: "); Serial.println(this->setStepsPerSec);
                    Serial.print("Enabled: "); Serial.println(this->stepperEnabled);
                    Serial.println();
                } 
            } 
        }
};