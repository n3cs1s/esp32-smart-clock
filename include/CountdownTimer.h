#pragma once

#include <Arduino.h>

class CountdownTimer {
 public:
  // Creates or replaces a timer. A duration of zero is invalid.
  bool create(uint32_t seconds);

  // Starts counting down from the remaining duration.
  bool start();

  // Pauses the timer without clearing the remaining duration.
  void stop();

  // Removes the timer and resets all timer state.
  void clear();

  // Call frequently from loop(). Returns true once when the timer finishes.
  bool update();

  bool exists() const;
  bool isRunning() const;
  uint32_t getRemaining() const;

 private:
  uint32_t remainingSeconds_ = 0;
  bool running_ = false;
  bool created_ = false;
  unsigned long lastTick_ = 0;
};
