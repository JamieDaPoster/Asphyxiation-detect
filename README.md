# Asphyxiation-detect

Child/pet-left-in-car detector (Arduino/ESP32). See `child_car_detector.ino`.

- `detector_logic.h` – hardware-independent alarm logic (hysteresis on heat alarm,
  SMS repeat interval, sensor-failure safe). Can be dropped into the sketch:
  build an `Inputs` each poll, call `Detector::update()`, drive buzzer/SMS from the result.
- `test/test_detector.cpp` – run with `g++ -std=c++11 test/test_detector.cpp -o /tmp/t && /tmp/t`.

Safety note: this is a hobby prototype, not a certified safety device.
