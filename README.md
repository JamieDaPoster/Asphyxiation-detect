# Asphyxiation-detect

A child-in-car detection system for Arduino/ESP32. It detects a child (or pet) left unattended in a parked vehicle and raises a local buzzer alarm plus an SMS alert to a guardian, helping prevent heatstroke and asphyxiation.

## How it works

A child is considered "left in car" when all of these are true:

1. The seat sensor reports occupancy.
2. The door has been closed since the seat became occupied.
3. The ignition is off.
4. No motion has been detected for 60 seconds.

When triggered, the buzzer sounds immediately and one SMS is sent to the configured number. Cabin temperature is also monitored: if it reaches 40 °C while the seat is occupied, an emergency alert fires regardless of motion or ignition state. Opening the door resets the alarm.

## Hardware

| Component | Default pin |
|---|---|
| Seat pressure/weight sensor (HIGH = occupied) | 2 |
| PIR motion sensor | 3 |
| Door reed switch (LOW = closed) | 4 |
| Ignition sense (HIGH = engine on) | 5 |
| Buzzer | 6 |
| DHT22 temperature/humidity sensor | 9 |
| SIM800L / SIM900 GSM module | RX 7 / TX 8 (AVR) or `Serial2` (ESP32) |

Works on Arduino Uno/Nano or ESP32.

## Setup

1. Install the **DHT sensor library** (Adafruit) in the Arduino IDE.
2. Open `child_car_detector.ino`.
3. Set `ALERT_PHONE_NUMBER` to the guardian's number.
4. Adjust pins and thresholds (`MOTION_TIMEOUT_MS`, `HIGH_TEMP_THRESHOLD_C`) if needed.
5. Upload to your board.

## Disclaimer

This is a hobby/educational project and not a certified safety device. Never rely on it as the only safeguard — always check the back seat before leaving a vehicle.
