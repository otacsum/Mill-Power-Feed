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

        // State management (Pullup defaults high)
        int lastButtonState = UNPRESSED;
        int currButtonState = UNPRESSED;

        // Modes [0 = Rapid Movement, 1 = Pause Function, 2 = Change Units]
        int buttonMode;

        // Hardware config
        int INPUT_PIN;

        void rapidFeed() {
            if (directionSwitch.directionSwitchOn) {
                if (this->currButtonState == PRESSED) {
                    if (DEBUG) {
                        Serial.print("RAPID: ");
                    } 

                    lcdMessage.rapidMessage();
                    stepper->setSpeedInUs(stepperUtils.rapidMicrosPerStep);
                    stepper->runForward();
                }
                else {
                    if (DEBUG) {
                        Serial.print("SLOW: ");
                    }

                    if (stepperUtils.paused || encodedInchesPerMin <= 0) {
                        lcdMessage.pausedMessage();
                        stepper->setSpeedInUs(stepperUtils.microsPerStep);
                        stepper->stopMove();
                    }
                    else {
                        lcdMessage.writeSpeed(encodedInchesPerMin);
                        stepper->setSpeedInUs(stepperUtils.microsPerStep);
                        stepper->runForward();
                    }
                }
            }
        }

        void pauseFeed() { // This seems backward, but it's correct for a press-then-release switch
            if (directionSwitch.directionSwitchOn) {
                if (this->currButtonState == UNPRESSED) {
                    if (!stepperUtils.paused) {
                        if (DEBUG) {
                            Serial.print("PAUSE: ");
                        } 

                        lcdMessage.pausedMessage();
                        stepper->stopMove();
                        
                    }
                    else {
                        if (DEBUG) {
                            Serial.print("RUN: ");
                        }

                        lcdMessage.writeSpeed(encodedInchesPerMin);
                        stepper->setSpeedInUs(stepperUtils.microsPerStep);
                        stepper->runForward();
                    }
                    stepperUtils.paused = !stepperUtils.paused; // Invert the state
                }
            }
        }

    public:

        // Constructor
        /** Modes 
         *      0 = Rapid Movement
         *      1 = Pause Function
         *      2 = Change Units
         */
        MomentarySwitch(unsigned long delay, int mode) {
            this->debounceDelay = delay;
            this->buttonMode = mode;
        };

        void begin(int pin) {
            // Set pin default state
            this->INPUT_PIN = pin;
            pinModeFast(this->INPUT_PIN, INPUT_PULLUP);
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

                        switch(this->buttonMode) {
                        case 0:
                            this->rapidFeed();
                            break;
                        
                        case 1:
                            this->pauseFeed();
                            break;
                        
                        default:
                            break;
                        }

                        
                    }
                }
                // Save the state
                this->lastButtonState = buttonReading;
            }
        }
};
