#include <SoftwareSerial.h>
#include <Servo.h>

SoftwareSerial wifiSerial(10, 11); // UNO D10 = RX (from ESP TX), D11 = TX (to ESP RX, optional)

#define SERVO_PIN 9     // UNO PWM pin for servo signal
#define RELAY1_PIN 2    // Fan 1
#define RELAY2_PIN 3    // Fan 2
#define RELAY3_PIN 4    // Pump
#define RELAY4_PIN 5    // LEDs

// Local analog sensors (fallback)
#define WATER_SENSOR_PIN A0
#define LIGHT_SENSOR_PIN A1

// Thresholds (adjust to suit)
const float TEMP_THRESHOLD_C    = 25.0;   // Temperature °C
const int   HUM_THRESHOLD       = 60;     // Humidity %
const int   GAS_THRESHOLD_PPM   = 200;    // Gas ppm
const long  LOAD_CELL_THRESHOLD = 500;    // grams (expect ESP to send calibrated grams)
const int   WATER_THRESHOLD_ML  = 300;    // ml
const int   LIGHT_THRESHOLD_LUX = 100;    // lux

// Device states
bool fan1State = false;
bool fan2State = false;
bool pumpState = false;
bool servoState = false;
bool ledsState = false;

Servo myservo;

void setup() {
  Serial.begin(115200);         // USB serial for debug
  wifiSerial.begin(9600);      // Serial from external WiFi sensor (ESP)

  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(RELAY3_PIN, OUTPUT);
  pinMode(RELAY4_PIN, OUTPUT);
  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RELAY2_PIN, LOW);
  digitalWrite(RELAY3_PIN, LOW);
  digitalWrite(RELAY4_PIN, LOW);

  myservo.attach(SERVO_PIN);
  myservo.write(0);

  Serial.println("UNO controller started. Waiting for CSV from WiFi sensor (pin10 RX)...");
}

// Try to read a single newline-terminated line from wifiSerial.
// Returns true if a full line read into `out`.
bool readCsvLine(String &out, unsigned long timeoutMs = 800) {
  unsigned long start = millis();
  out = "";
  while (millis() - start < timeoutMs) {
    while (wifiSerial.available()) {
      char c = (char)wifiSerial.read();
      if (c == '\r') continue;
      if (c == '\n') {
        if (out.length() > 0) return true;
      } else {
        out += c;
        if (out.length() > 300) return true; // safety guard
      }
    }
  }
  return false; // timeout or no complete line
}

// Parse CSV: temp,hum,gas,load,water,light
// If a field is missing, the variable will remain NAN (or -1 for load if unchanged)
void parseCsvValues(const String &line, float &temp, float &hum, float &gas, float &loadVal, float &waterMl, float &lightLux) {
  temp = hum = gas = loadVal = waterMl = lightLux = NAN;
  int idx = 0;
  int start = 0;
  int len = line.length();
  for (int i = 0; i <= len; ++i) {
    if (i == len || line.charAt(i) == ',') {
      String token = line.substring(start, i);
      token.trim();
      float v = NAN;
      if (token.length() > 0) {
        v = token.toFloat(); // returns 0.0 for invalid text; we accept 0 if token is "0" etc.
        // toFloat returns 0.0 for invalid strings; to detect invalid non-zero we could check more,
        // but we assume sensor sends numeric or empty.
      }
      switch (idx) {
        case 0: if (token.length()>0) temp = v; break;
        case 1: if (token.length()>0) hum = v;  break;
        case 2: if (token.length()>0) gas = v;  break;
        case 3: if (token.length()>0) loadVal = v; break;
        case 4: if (token.length()>0) waterMl = v; break;
        case 5: if (token.length()>0) lightLux = v; break;
      }
      idx++;
      start = i + 1;
    }
  }
}

void applyControls(float temperature, float humidity, float gasValue, long loadCellValue, int waterSensorValue_ml, int lightSensorValue_lux) {
  // Fan 1: temperature OR humidity
  if ( (!isnan(temperature) && (temperature > TEMP_THRESHOLD_C)) || (!isnan(humidity) && (humidity > HUM_THRESHOLD)) ) {
    digitalWrite(RELAY1_PIN, HIGH);
    fan1State = true;
  } else {
    digitalWrite(RELAY1_PIN, LOW);
    fan1State = false;
  }

  // Fan 2: gas threshold
  if ( !isnan(gasValue) && (gasValue > GAS_THRESHOLD_PPM) ) {
    digitalWrite(RELAY2_PIN, HIGH);
    fan2State = true;
  } else {
    digitalWrite(RELAY2_PIN, LOW);
    fan2State = false;
  }

  // Servo: based on load cell
  if ( (loadCellValue >= 0) && (loadCellValue > LOAD_CELL_THRESHOLD) ) {
    myservo.write(90);
    servoState = true;
  } else {
    myservo.write(0);
    servoState = false;
  }

  // Pump: water level (ml)
  if ( waterSensorValue_ml > WATER_THRESHOLD_ML ) {
    digitalWrite(RELAY3_PIN, HIGH);
    pumpState = true;
  } else {
    digitalWrite(RELAY3_PIN, LOW);
    pumpState = false;
  }

  // LEDs: light
  if ( lightSensorValue_lux < LIGHT_THRESHOLD_LUX ) {
    digitalWrite(RELAY4_PIN, HIGH);
    ledsState = true;
  } else {
    digitalWrite(RELAY4_PIN, LOW);
    ledsState = false;
  }
}

void printStatus(float temperature, float humidity, float gasValue, long loadCellValue, int waterSensorValue_ml, int lightSensorValue_lux) {
  Serial.print("Temperature: ");
  if (!isnan(temperature)) Serial.print(temperature); else Serial.print("N/A");
  Serial.print(" °C, Humidity: ");
  if (!isnan(humidity)) Serial.print(humidity); else Serial.print("N/A");
  Serial.print(" %, Gas: ");
  if (!isnan(gasValue)) Serial.print(gasValue); else Serial.print("N/A");
  Serial.print(" ppm, Load: ");
  if (loadCellValue >= 0) Serial.print(loadCellValue); else Serial.print("N/A");
  Serial.print(" g, Water: ");
  Serial.print(waterSensorValue_ml);
  Serial.print(" ml, Light: ");
  Serial.print(lightSensorValue_lux);
  Serial.println(" lux");

  Serial.print("States - Fan1: "); Serial.print(fan1State ? "On" : "Off");
  Serial.print(", Fan2: "); Serial.print(fan2State ? "On" : "Off");
  Serial.print(", Pump: "); Serial.print(pumpState ? "On" : "Off");
  Serial.print(", Servo: "); Serial.print(servoState ? "On" : "Off");
  Serial.print(", LEDs: "); Serial.println(ledsState ? "On" : "Off");
  Serial.println("---------------------------------------------------");
}

void loop() {
  String line;
  float temperature = NAN, humidity = NAN, gasValue = NAN, loadCellValF = NAN, waterMlF = NAN, lightLuxF = NAN;
  long loadCellValue = -1;
  int waterSensorValue_ml = 0;
  int lightSensorValue_lux = 0;

  // Try to read CSV from external WiFi sensor (ESP)
  if (readCsvLine(line, 800)) {
    // Expected CSV: temp,hum,gas,load,water,light
    parseCsvValues(line, temperature, humidity, gasValue, loadCellValF, waterMlF, lightLuxF);
    if (!isnan(loadCellValF)) loadCellValue = (long)loadCellValF;
    if (!isnan(waterMlF)) waterSensorValue_ml = (int)waterMlF;
    if (!isnan(lightLuxF)) lightSensorValue_lux = (int)lightLuxF;
    Serial.print("Received CSV: "); Serial.println(line);
  } else {
    // Fallback to local analog sensors (water & light)
    int rawWater = analogRead(WATER_SENSOR_PIN);   // 0..1023
    int rawLight = analogRead(LIGHT_SENSOR_PIN);   // 0..1023
    // Map analog to physical units (adjust as required for your sensors)
    waterSensorValue_ml = map(rawWater, 0, 1023, 0, 1000);  // assume 0..1000 ml range
    lightSensorValue_lux = map(rawLight, 0, 1023, 0, 1000); // assume 0..1000 lux range
    Serial.println("No CSV from WiFi sensor — using local analog sensors for water & light.");
  }

  applyControls(temperature, humidity, gasValue, loadCellValue, waterSensorValue_ml, lightSensorValue_lux);
  printStatus(temperature, humidity, gasValue, loadCellValue, waterSensorValue_ml, lightSensorValue_lux);

  delay(3000);
}
