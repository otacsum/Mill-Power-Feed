void readRotaryEncoder() {
  long newEncoderPosition = rotaryEncoder.read();
  // Set raw-reading lower and upper limits
  newEncoderPosition = constrain(newEncoderPosition, 0, maxEncoderPosition);
  // Re-write object state when limits reached.
  if (newEncoderPosition == 0 || newEncoderPosition == maxEncoderPosition) {
    rotaryEncoder.write(newEncoderPosition);
  }

  // Reduce it to nominal steps per detent for incremental comparison
  newEncoderPosition /= encoderStepsPerDetent;

  // If it has changed, write the new value to the stepper object.
  if (newEncoderPosition != oldEncoderPosition) {
    encodedInchesPerMin = newEncoderPosition * SPEEDINCREMENT;
    stepper.setSpeed(encodedInchesPerMin);
    oldEncoderPosition = newEncoderPosition; // State management
  }  
}