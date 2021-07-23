class LCDMessage {
    private:
        String line1String;
        String line2String;
        String speedPrefix = "Inch/min: ";

        bool isDisplayingSpeed = false;

        int directionState = 3;

        void writeLCD() {
            lcd.setCursor(0,0); // First Row
            lcd.print(this->line1String);
            lcd.setCursor(0,1); // Second Row
            lcd.print(this->line2String);
        }

    public:
        // Constructor
        LCDMessage() {
            lcd.clear();
        };
        
        // Boot Up Message
        void welcomeMessage() {
            this->isDisplayingSpeed = false;
            this->line1String = "-- POWER FEED --"; 
            this->line2String = "---- READY! ----";
            this->writeLCD();
            delay(3000);
            lcd.clear();
        }

        void bootError() {
            this->isDisplayingSpeed = false;
            this->line1String = "***  ERROR  ****"; 
            this->line2String = "* RESET SWITCH *";
            this->writeLCD();
        }

        void rapidMessage() {
            this->isDisplayingSpeed = false;
            this->line1String = "---- RAPID ---- "; 
            this->writeLCD();
        }

        void pausedMessage() {
            this->isDisplayingSpeed = false;
            this->line1String = "---- PAUSED ----"; 
            this->writeLCD();
        }

        void writeSpeed(float speed) {
            String speedStr = String(speed, 2) + "  ";

            this->line1String = this->speedPrefix + speedStr;
            
            if (this->isDisplayingSpeed) {  // Only print the first line
                lcd.setCursor(10,0); // Replace just the chars changed
                lcd.print(speedStr);
            }
            else {  // Create the arrows and print the whole thing.
                this->isDisplayingSpeed = true;
                this->printArrows(this->directionState); 
            }              
        }

        void printArrows(int direction) {
            this->directionState = direction;
            switch (direction) {
                case LOW:
                    this->line2String = "         >>>>   ";
                    break;
                case HIGH:
                    this->line2String = "   <<<<         ";
                    break;
                default:
                    this->line2String = "  \xff STOPPED \xff   ";
                    break;
            }
            this->writeLCD();
        }
        
};

