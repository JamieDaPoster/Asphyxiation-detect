#include "../detector_logic.h"
#include <cassert>
#include <cstdio>

static Inputs base() { return {true, false, true, false, 25.0f}; }

int main() {
  bool sms;
  { // Unattended: needs door to have been opened then closed on occupied seat
    Detector d; Inputs i = base(); i.doorClosed = false;
    d.update(i, 0, &sms);
    i.doorClosed = true;
    assert(d.update(i, 1000, &sms) == Alarm::None);
    assert(d.update(i, 61000, &sms) == Alarm::Unattended && sms);
    assert(d.update(i, 62000, &sms) == Alarm::Unattended && !sms);   // no spam
    assert(d.update(i, 61000 + 300000, &sms) == Alarm::Unattended && sms); // resend
  }
  { // Motion resets the timer; ignition suppresses
    Detector d; Inputs i = base(); i.doorClosed = false; d.update(i, 0, &sms);
    i.doorClosed = true; i.motion = true; d.update(i, 50000, &sms);
    i.motion = false;
    assert(d.update(i, 100000, &sms) == Alarm::None);
    i.ignitionOn = true;
    assert(d.update(i, 200000, &sms) == Alarm::None);
  }
  { // Heat fires with door open/ignition on, and clears only with hysteresis
    Detector d; Inputs i = base(); i.ignitionOn = true; i.tempC = 41;
    assert(d.update(i, 0, &sms) == Alarm::Heat && sms);
    i.tempC = 39;
    assert(d.update(i, 1000, &sms) == Alarm::Heat);
    i.tempC = 37.9f;
    assert(d.update(i, 2000, &sms) == Alarm::None);
  }
  { // Sensor failure does not clear or trigger heat
    Detector d; Inputs i = base(); i.tempC = NAN;
    assert(d.update(i, 0, &sms) == Alarm::None);
  }
  puts("all tests passed");
}
