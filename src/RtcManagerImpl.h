#ifndef RTCMANAGERIMPL_H
#define RTCMANAGERIMPL_H

#include <cstring>
#include <memory>
#include <RTClib.h>
#include "../include/RtcManager.h"
#include "PlaybackLoggerImpl.h"

class RtcManagerImpl : public RtcManager {
private:
  RTC_DS3231 rtc;
  bool available = false;
  time_t lastCachedTime = 0;
  char lastError[256] = {0};
  PlaybackLogger* logger = nullptr;

public:
  RtcManagerImpl(PlaybackLogger* logger = nullptr) : logger(logger) {}
  ~RtcManagerImpl() override = default;

  bool initialize(TwoWire* bus) override;
  bool isAvailable() const override;
  time_t now() const override;
  bool setTime(time_t epoch) override;
  const char* getLastError() const override;
};

#endif // RTCMANAGERIMPL_H
