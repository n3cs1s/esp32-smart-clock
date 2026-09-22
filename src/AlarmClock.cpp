#include "AlarmClock.h"

bool AlarmClock::create(uint8_t hour, uint8_t minute) {
  if (hour > 23 || minute > 59) {
    return false;
  }

  alarmHour_ = hour;
  alarmMinute_ = minute;
  created_ = true;
  enabled_ = false;
  lastTriggeredDay_ = -1;
  return true;
}

bool AlarmClock::start() {
  if (!created_) {
    return false;
  }

  enabled_ = true;
  return true;
}

void AlarmClock::stop() {
  enabled_ = false;
}

void AlarmClock::clear() {
  created_ = false;
  enabled_ = false;
  lastTriggeredDay_ = -1;
}

bool AlarmClock::update(const tm& timeinfo) {
  if (!created_ || !enabled_) {
    return false;
  }

  const int today = timeinfo.tm_year * 366 + timeinfo.tm_yday;

  if (timeinfo.tm_hour == alarmHour_ &&
      timeinfo.tm_min == alarmMinute_ &&
      lastTriggeredDay_ != today) {
    lastTriggeredDay_ = today;
    return true;
  }

  return false;
}

bool AlarmClock::exists() const {
  return created_;
}

bool AlarmClock::isEnabled() const {
  return enabled_;
}

uint8_t AlarmClock::getHour() const {
  return alarmHour_;
}

uint8_t AlarmClock::getMinute() const {
  return alarmMinute_;
}
