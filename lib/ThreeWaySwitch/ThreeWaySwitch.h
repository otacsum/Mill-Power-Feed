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
        int lastSwitchState = UNPRESSED;
        int currSwitchState = UNPRESSED;

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
            pinModeFast(this->LEFT_PIN, INPUT_PULLUP);

            this->RIGHT_PIN = pins[1];
            pinModeFast(this->RIGHT_PIN, INPUT_PULLUP);

            // Safety Interlock - Do not start at boot
            // User must switch to middle (disabled) direction 
            // position before motor will run.
            if (digitalReadFast(this->RIGHT_PIN) != PRESSED
                    &&
                    digitalReadFast(this->LEFT_PIN) != PRESSED) {
                readyState = "Ready";
                this->runEnabled = true;
            } 
            else {  // Switch is on at boot, disable switch until reset.
                readyState = "Please Set Direction to Middle";
                lcdMessage.bootError();
                // Keep polling, do not continue initialization until the switch goes low
                while (digitalReadFast(this->RIGHT_PIN) == PRESSED
                    ||
                    digitalReadFast(this->LEFT_PIN) == PRESSED) {
                        delay(1);
                    }
                
                // Enable safety flag if it was previously suppressed
                if (!this->runEnabled) {
                    this->runEnabled = true;
                }
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
                
                if (digitalReadFast(this->RIGHT_PIN) == PRESSED
                        || 
                        digitalReadFast(this->LEFT_PIN) == PRESSED) {
                    switchReading = PRESSED;
                }
                else {
                    switchReading = UNPRESSED;
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

                        if (this->currSwitchState == PRESSED) { // Switch is on
                            
                            if (digitalReadFast(this->RIGHT_PIN) == PRESSED) {
                                this->feedMotor->setDirection(1); // Clockwise (relative)
                                lcdMessage.printArrows(1);
                            }
                            else if (digitalReadFast(this->LEFT_PIN) == PRESSED) {
                                this->feedMotor->setDirection(0); // Counter-Clockwise (relative)
                                lcdMessage.printArrows(0);
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

                            lcdMessage.printArrows(3); // "STOPPED"
                            
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
            /* if (DEBUG) {
                Serial.print("In/Min: "); Serial.print(this->feedMotor->currentInchesPerMin);
                Serial.print(" | MSS: "); Serial.print(this->feedMotor->maxStepsPerSec);
                Serial.print(" | SSS: "); Serial.print(this->feedMotor->setStepsPerSec);
                Serial.print(" | CSS: "); Serial.print(this->feedMotor->currentStepsPerSec);
                Serial.print(" | MicS: "); Serial.println(this->feedMotor->microsPerStep);
            } */

            if (this->runEnabled && this->currSwitchState == PRESSED) { // Safety Flag is safe and switch is on, not bouncing.
                this->feedMotor->run();
            }
            else {
                this->feedMotor->stop();
            }
        }
};
