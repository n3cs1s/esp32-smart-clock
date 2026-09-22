#include "CountdownTimer.h"

bool CountdownTimer::create(uint32_t seconds) {
  if (seconds == 0) {
    return false;
  }

  remainingSeconds_ = seconds;
  running_ = false;
  created_ = true;
  lastTick_ = millis();
  return true;
}

bool CountdownTimer::start() {
  if (!created_ || remainingSeconds_ == 0) {
    return false;
  }

  running_ = true;
  lastTick_ = millis();
  return true;
}

void CountdownTimer::stop() {
  running_ = false;
}

void CountdownTimer::clear() {
  remainingSeconds_ = 0;
  running_ = false;
  created_ = false;
}

bool CountdownTimer::update() {
  if (!running_ || remainingSeconds_ == 0) {
    return false;
  }

  const unsigned long now = millis();
  const uint32_t elapsed = (now - lastTick_) / 1000UL;

  if (elapsed == 0) {
    return false;
  }

  lastTick_ += elapsed * 1000UL;

  if (elapsed >= remainingSeconds_) {
    remainingSeconds_ = 0;
    running_ = false;
    return true;
  }

  remainingSeconds_ -= elapsed;
  return false;
}

bool CountdownTimer::exists() const {
  return created_;
}

bool CountdownTimer::isRunning() const {
  return running_;
}

uint32_t CountdownTimer::getRemaining() const {
  return remainingSeconds_;
}
