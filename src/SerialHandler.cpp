#include "SerialHandler.h"

SerialHandler::SerialHandler(Commander &commander) : commander_(commander) {
  rxIndex_ = 0;
  memset(rxBuffer_, 0, BUFFER_SIZE);
}

void SerialHandler::update() {
  // Read all available bytes from the serial hardware buffer
  while (Serial.available() > 0) {
    int c = Serial.read();

    // Prevent buffer overflow by resetting index if limit is hit
    if (rxIndex_ >= (BUFFER_SIZE - 1)) {
      rxIndex_ = 0;
      Serial.println(
          "Error: Serial command buffer overflow (exceeded 128 bytes)");
    }

    // Check for "End of Line" characters
    if (c == '\n' || c == '\r') {
      if (rxIndex_ > 0) {
        rxBuffer_[rxIndex_] = '\0'; // Null-terminate the string
        processLine();              // Send to Commander
        rxIndex_ = 0;               // Reset for next command
      }
    } else {
      // Append printable characters to the buffer
      rxBuffer_[rxIndex_++] = (char)c;
    }
  }
}

void SerialHandler::processLine() { commander_.handleCommand(rxBuffer_); }
