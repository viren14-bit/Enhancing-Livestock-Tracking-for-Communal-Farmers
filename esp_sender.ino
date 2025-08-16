#include <DHT.h>
#include "HX711.h"
#include <MQ135.h>

// ------------------ Pin configuration (change as needed) ------------------
#define DHT_PIN         4    // ESP digital pin for DHT data (change as per board)
#define DHT_TYPE        DHT11

#define MQ_PIN          A0   // analog pin for MQ-135 (ESP8266 ADC or ESP32 ADC pin)
#define HX711_DOUT_PIN  14   // HX711 DOUT (digital)
#define HX711_SCK_PIN   12   // HX711 SCK  (digital)

#define WATER_ANALOG_PIN A0  // If your board has separate ADC, pick unique pin (for ESP32 you'd use different ADC)
#define LIGHT_ANALOG_PIN A2  // Example for ESP32 (adjust if using ESP8266: only A0 available) 

// ------------------ Calibration & tuning ------------------
const float HX711_CALIBRATION_FACTOR = 2280.0; // <-- CHANGE THIS to match your load cell calibration
// HX711 calibration: use known weight and adjust this factor until readings match grams.

DHT dht(DHT_PIN, DHT_TYPE);
HX711 scale;
MQ135 gasSensor(MQ_PIN);

void setup() {
  // Serial used to communicate to UNO (TX -> UNO RX). Default TX on many ESPs is Serial (USB).
  Serial.begin(9600);
  delay(200);

  dht.begin();

  // Initialize HX711
  scale.begin(HX711_DOUT_PIN, HX711_SCK_PIN);
  scale.set_scale(HX711_CALIBRATION_FACTOR); // set calibration factor
  scale.tare(); // reset the scale to 0

  // MQ135 setup: some libraries may need calibration/warm-up
  // No extra init required for many MQ libs, but allow warm-up time
  delay(2000);
  Serial.println("ESP sensor node started. Sending CSV to UNO at 9600 baud.");
}

// Helper: read HX711 and return grams (average)
float readLoadCellGrams() {
  if (!scale.is_ready()) {
    // Try to recover by returning NAN; scale.is_ready checks for data availability
    return NAN;
  }
  // get_units(10) returns value converted by calibration factor
  float val = scale.get_units(10); // average 10 readings
  // if you find negative values, adjust tare or wiring
  return val;
}

// Helper: read MQ135 PPM (many MQ libraries provide getPPM(); else derive approximations)
float readGasPPM() {
  // MQ135 library provides getPPM() if available; else provide analog read fallback
  #ifdef MQ135_h
    float ppm = gasSensor.getPPM(); // if library supports
    if (isnan(ppm) || ppm <= 0) {
      // fallback to analog reading scaled roughly
      int raw = analogRead(MQ_PIN);
      ppm = map(raw, 0, 4095, 0, 1000); // for ESP32 12-bit ADC; adjust if needed
    }
    return ppm;
  #else
    // Fallback: read raw analog and map to 0..1000
    int rawv = analogRead(MQ_PIN);
    #if defined(ARDUINO_ARCH_ESP32)
      // ESP32 ADC is 0..4095 by default
      return (float)map(rawv, 0, 4095, 0, 1000);
    #else
      // ESP8266 ADC is 0..1023
      return (float)map(rawv, 0, 1023, 0, 1000);
    #endif
  #endif
}

// Helper: read analog sensor and map to unit (0..1000)
int analogToRange(int raw, int rawMax = 1023, int outMax = 1000) {
  long v = raw;
  long mapped = map(v, 0, rawMax, 0, outMax);
  if (mapped < 0) mapped = 0;
  if (mapped > outMax) mapped = outMax;
  return (int)mapped;
}

void loop() {
  // Read temperature & humidity
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Read gas (ppm)
  float gas_ppm = readGasPPM();

  // Read load (grams)
  float load_grams = readLoadCellGrams();

  // Read water & light analog (map to ml/lux approximation)
  int rawWater = 0;
  int rawLight = 0;
  #if defined(ARDUINO_ARCH_ESP32)
    // Example pins for ESP32 — replace with your wiring
    rawWater = analogRead(WATER_ANALOG_PIN); // 0..4095
    rawLight = analogRead(LIGHT_ANALOG_PIN); // 0..4095
    int water_ml = analogToRange(rawWater, 4095, 1000);
    int light_lux = analogToRange(rawLight, 4095, 1000);

    // Ensure fallback if pin mapping isn't available
    // Note: If using same A0 for MQ135 on ESP8266, avoid conflict and use different sensors or send blanks
  #else
    // ESP8266 has only A0; if you need multiple analog sensors, consider using ADC multiplexer or send raw values differently
    rawWater = analogRead(A0);
    rawLight = analogRead(A0);
    int water_ml = analogToRange(rawWater, 1023, 1000);
    int light_lux = analogToRange(rawLight, 1023, 1000);
  #endif

  // Build values to send. If a reading is invalid (NaN), we send empty field.
  // Format: temp,humidity,gas_ppm,load_grams,water_ml,light_lux\n

  // Prepare each token string (no spaces)
  String tokenTemp = isnan(temperature) ? "" : String(temperature, 2);
  String tokenHum  = isnan(humidity) ? "" : String(humidity, 2);
  String tokenGas  = isnan(gas_ppm) ? "" : String(gas_ppm, 1);
  String tokenLoad = isnan(load_grams) ? "" : String(load_grams, 1);

  // Map analogs properly (reuse code above)
  int water_ml = 0;
  int light_lux = 0;
  #if defined(ARDUINO_ARCH_ESP32)
    water_ml = analogToRange(rawWater, 4095, 1000);
    light_lux = analogToRange(rawLight, 4095, 1000);
  #else
    water_ml = analogToRange(rawWater, 1023, 1000);
    light_lux = analogToRange(rawLight, 1023, 1000);
  #endif

  String tokenWater = String(water_ml);
  String tokenLight = String(light_lux);

  // Build CSV line
  String csv = tokenTemp + "," + tokenHum + "," + tokenGas + "," + tokenLoad + "," + tokenWater + "," + tokenLight;

  // Send CSV to UNO
  Serial.println(csv); // UNO should read this on its software serial RX pin (UNO D10)

  // Debug print to USB serial (if connected)
  // On some boards Serial prints to both USB and TX0; if you use Serial to UNO physically, consider using Serial1 for USB debug on ESP32

  // Short delay between sends
  delay(3000);
}
