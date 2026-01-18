#pragma once

#include "Commander.h"
#include <Arduino.h>

/**
 * @class SerialHandler
 * @brief Manages incoming serial data and buffers it into complete command
 * lines.
 *
 * This class reads bytes from the Hardware Serial port, accumulating them in a
 * fixed-size buffer until a newline or carriage return is detected. Once a
 * complete line is received, it is passed to the Commander for parsing.
 */
class SerialHandler {
public:
  /**
   * @brief Constructs the handler with a reference to the command processor.
   */
  SerialHandler(Commander &commander);

  /**
   * @brief Reads available bytes from Serial. Should be called frequently in
   * loop().
   */
  void update();

private:
  Commander &commander_; ///< Reference to the system's command processor.

  static constexpr size_t BUFFER_SIZE =
      128;                     ///< Maximum length of a single command.
  char rxBuffer_[BUFFER_SIZE]; ///< Internal buffer for accumulating characters.
  size_t rxIndex_ = 0;         ///< Current write position in the buffer.

  /**
   * @brief Forwards the accumulated buffer to the Commander.
   */
  void processLine();
};
