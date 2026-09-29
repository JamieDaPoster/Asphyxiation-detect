// Pure decision logic for the child-in-car detector (no Arduino dependencies,
// so it can be unit-tested on a PC). Feed it one Inputs sample per poll.
#pragma once
#include <stdint.h>
#include <math.h>

struct Inputs {
  bool seatOccupied;
  bool motion;
  bool doorClosed;
  bool ignitionOn;
  float tempC;  // NAN if the sensor read failed
};

enum class Alarm : uint8_t { None, Unattended, Heat };

struct Config {
  uint32_t motionTimeoutMs = 60000;
  float highTempC = 40.0f;
  float tempHysteresisC = 2.0f;      // must cool this far below highTempC to clear
  uint32_t resendIntervalMs = 5UL * 60UL * 1000UL;  // repeat SMS while alarm persists
};

class Detector {
 public:
  explicit Detector(const Config &c = Config()) : cfg_(c) {}

  // Returns the current alarm state; sets *sendSms when an SMS should go out now.
  Alarm update(const Inputs &in, uint32_t now, bool *sendSms) {
    *sendSms = false;
    if (in.motion) lastMotion_ = now;

    if (!in.doorClosed) doorOpened_ = true;          // an adult is present
    else if (in.seatOccupied) doorOpened_ = false;   // door shut on an occupied seat

    // Heat latches with hysteresis; it ignores door/ignition on purpose.
    bool tempValid = !isnan(in.tempC);
    if (in.seatOccupied && tempValid) {
      if (in.tempC >= cfg_.highTempC) heat_ = true;
      else if (in.tempC <= cfg_.highTempC - cfg_.tempHysteresisC) heat_ = false;
    } else if (!in.seatOccupied) {
      heat_ = false;
    }

    bool unattended = in.seatOccupied && in.doorClosed && !in.ignitionOn &&
                      !doorOpened_ && (now - lastMotion_) >= cfg_.motionTimeoutMs;

    Alarm a = heat_ ? Alarm::Heat : (unattended ? Alarm::Unattended : Alarm::None);

    if (a == Alarm::None) {
      active_ = false;
    } else if (!active_ || (now - lastSms_) >= cfg_.resendIntervalMs) {
      *sendSms = true;
      lastSms_ = now;
      active_ = true;
    }
    return a;
  }

 private:
  Config cfg_;
  uint32_t lastMotion_ = 0, lastSms_ = 0;
  bool doorOpened_ = true, heat_ = false, active_ = false;
};
