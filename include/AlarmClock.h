#pragma once

#include <Arduino.h>
#include <time.h>

class AlarmClock {
 public:
  // Creates or replaces a daily alarm. The new alarm starts disabled.
  bool create(uint8_t hour, uint8_t minute);

  // Enables the alarm.
  bool start();

  // Disables the alarm without deleting its configured time.
  void stop();

  // Deletes the configured alarm.
  void clear();

  // Call frequently after time synchronization.
  // Returns true once per day when the configured minute is reached.
  bool update(const tm& timeinfo);

  bool exists() const;
  bool isEnabled() const;
  uint8_t getHour() const;
  uint8_t getMinute() const;

 private:
  uint8_t alarmHour_ = 0;
  uint8_t alarmMinute_ = 0;
  bool created_ = false;
  bool enabled_ = false;
  int lastTriggeredDay_ = -1;
};
