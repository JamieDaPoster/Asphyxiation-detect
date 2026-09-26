/*
  Child-in-Car Detection System
  -----------------------------
  Detects a child (or pet) left unattended in a vehicle and raises a local
  alarm plus an SMS alert.

  Hardware (typical wiring, adjust pins to your board):
    - Seat pressure/weight sensor (FSR or load cell + comparator) -> SEAT_SENSOR_PIN (digital, HIGH = occupied)
    - PIR motion sensor                                           -> PIR_SENSOR_PIN (digital, HIGH = motion)
    - DHT22 temperature & humidity sensor                         -> DHT_PIN
    - Door reed switch                                            -> DOOR_SENSOR_PIN (digital, LOW = closed)
    - Ignition/accessory sense (via voltage divider or relay)     -> IGNITION_PIN (digital, HIGH = engine on)
    - Buzzer                                                      -> BUZZER_PIN
    - SIM800L / SIM900 GSM module                                 -> Serial (RX/TX, see GSM_SERIAL)

  Board: Arduino Uno/Nano or ESP32 (works with either; SoftwareSerial is
  only used on AVR boards without a spare hardware UART).

  Logic:
    1. A child is considered "left in car" when the seat sensor reports
       occupancy, the door has been closed, the ignition has been turned
       off, and no motion has been detected for MOTION_TIMEOUT_MS.
    2. When triggered, the buzzer sounds immediately and an SMS alert is
       sent once to the configured phone number.
    3. Cabin temperature is monitored continuously; if it crosses
       HIGH_TEMP_THRESHOLD_C while the seat is occupied, an emergency
       alert is sent regardless of the motion/ignition state, since heat
       buildup is the primary danger.
*/

#include <DHT.h>

#if defined(ARDUINO_ARCH_AVR)
  #include <SoftwareSerial.h>
  #define GSM_RX_PIN 7
  #define GSM_TX_PIN 8
  SoftwareSerial gsmSerial(GSM_RX_PIN, GSM_TX_PIN);
  #define GSM_SERIAL gsmSerial
#else
  // ESP32 / boards with a spare hardware serial port
  #define GSM_SERIAL Serial2
#endif

// ---------- Pin configuration ----------
const uint8_t SEAT_SENSOR_PIN = 2;
const uint8_t PIR_SENSOR_PIN  = 3;
const uint8_t DOOR_SENSOR_PIN = 4;
const uint8_t IGNITION_PIN    = 5;
const uint8_t BUZZER_PIN      = 6;
const uint8_t DHT_PIN         = 9;
#define DHT_TYPE DHT22

// ---------- Thresholds ----------
const unsigned long MOTION_TIMEOUT_MS   = 60UL * 1000UL;   // no motion for 60s -> suspicious
const unsigned long SENSOR_POLL_MS      = 500UL;           // sensor read interval
const float HIGH_TEMP_THRESHOLD_C       = 40.0f;           // cabin temp danger threshold
const char *ALERT_PHONE_NUMBER          = "+10000000000";  // set to guardian's number

DHT dht(DHT_PIN, DHT_TYPE);

unsigned long lastMotionTime = 0;
unsigned long lastPollTime = 0;
bool alarmActive = false;
bool alertSent = false;
bool doorWasOpenSinceOccupied = true;

void setup() {
  Serial.begin(115200);
  GSM_SERIAL.begin(9600);

  pinMode(SEAT_SENSOR_PIN, INPUT);
  pinMode(PIR_SENSOR_PIN, INPUT);
  pinMode(DOOR_SENSOR_PIN, INPUT_PULLUP);
  pinMode(IGNITION_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  dht.begin();
  lastMotionTime = millis();

  Serial.println(F("Child-in-car detection system starting..."));
  initGSM();
}

void loop() {
  unsigned long now = millis();
  if (now - lastPollTime < SENSOR_POLL_MS) {
    return;
  }
  lastPollTime = now;

  bool seatOccupied = digitalRead(SEAT_SENSOR_PIN) == HIGH;
  bool motionDetected = digitalRead(PIR_SENSOR_PIN) == HIGH;
  bool doorClosed = digitalRead(DOOR_SENSOR_PIN) == LOW;
  bool ignitionOn = digitalRead(IGNITION_PIN) == HIGH;
  float temperatureC = dht.readTemperature();

  if (motionDetected) {
    lastMotionTime = now;
  }

  if (!doorClosed) {
    // Door opened: treat as an adult attending to the seat; reset state.
    doorWasOpenSinceOccupied = true;
    resetAlarm();
  } else if (seatOccupied) {
    doorWasOpenSinceOccupied = false;
  }

  bool noMotionTooLong = (now - lastMotionTime) >= MOTION_TIMEOUT_MS;
  bool childLeftBehind = seatOccupied && doorClosed && !ignitionOn &&
                         noMotionTooLong && !doorWasOpenSinceOccupied;

  bool temperatureDanger = seatOccupied && !isnan(temperatureC) &&
                            temperatureC >= HIGH_TEMP_THRESHOLD_C;

  if (childLeftBehind || temperatureDanger) {
    triggerAlarm(temperatureDanger, temperatureC);
  } else if (!seatOccupied) {
    resetAlarm();
  }

  logStatus(seatOccupied, motionDetected, doorClosed, ignitionOn, temperatureC);
}

void triggerAlarm(bool dueToTemperature, float temperatureC) {
  alarmActive = true;
  digitalWrite(BUZZER_PIN, HIGH);

  if (!alertSent) {
    String reason = dueToTemperature
      ? "High cabin temperature (" + String(temperatureC, 1) + " C) with occupant detected!"
      : "Occupant left unattended with no motion detected!";
    sendSMSAlert(reason);
    alertSent = true;
  }
}

void resetAlarm() {
  if (alarmActive) {
    digitalWrite(BUZZER_PIN, LOW);
    alarmActive = false;
  }
  alertSent = false;
}

void initGSM() {
  GSM_SERIAL.println("AT");
  delay(500);
  GSM_SERIAL.println("AT+CMGF=1"); // set SMS text mode
  delay(500);
}

void sendSMSAlert(const String &message) {
  Serial.print(F("Sending SMS alert: "));
  Serial.println(message);

  GSM_SERIAL.println("AT+CMGF=1");
  delay(200);
  GSM_SERIAL.print("AT+CMGS=\"");
  GSM_SERIAL.print(ALERT_PHONE_NUMBER);
  GSM_SERIAL.println("\"");
  delay(200);
  GSM_SERIAL.print("ALERT: Child/pet detected left in vehicle. ");
  GSM_SERIAL.print(message);
  GSM_SERIAL.write(26); // Ctrl+Z sends the SMS
  delay(500);
}

void logStatus(bool seatOccupied, bool motionDetected, bool doorClosed,
               bool ignitionOn, float temperatureC) {
  Serial.print(F("Seat: "));
  Serial.print(seatOccupied ? "occupied" : "empty");
  Serial.print(F(" | Motion: "));
  Serial.print(motionDetected ? "yes" : "no");
  Serial.print(F(" | Door: "));
  Serial.print(doorClosed ? "closed" : "open");
  Serial.print(F(" | Ignition: "));
  Serial.print(ignitionOn ? "on" : "off");
  Serial.print(F(" | Temp: "));
  Serial.print(isnan(temperatureC) ? -1 : temperatureC);
  Serial.print(F(" C | Alarm: "));
  Serial.println(alarmActive ? "ACTIVE" : "off");
}
