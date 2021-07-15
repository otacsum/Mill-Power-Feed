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