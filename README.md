<body>
  <div class="wrap">
    <header>
      <div>
        <h1>Enhancing-Livestock-Tracking-for-Communal-Farmers — UNO Controller + External Wi-Fi Sensor</h1>
        <p class="small">Arduino UNO parses CSV from an ESP (ESP8266/ESP32) and controls relays + servo. Version 1.0 — MIT</p>
      </div>
    </header>
    <nav>
      <a href="#overview">Overview</a>
      <a href="#features">Features</a>
      <a href="#files">Files</a>
      <a href="#hardware">Hardware</a>
      <a href="#wiring">Wiring / Pinout</a>
      <a href="#csv">CSV Format</a>
      <a href="#uno-usage">UNO Sketch</a>
      <a href="#esp-template">ESP Template</a>
      <a href="#tuning">Thresholds & Calibration</a>
      <a href="#troubleshoot">Troubleshooting</a>
      <a href="#safety">Safety</a>
      <a href="#license">License</a>
    </nav>
    <section id="overview">
      <h2>Project Overview</h2>
      <p>This project splits sensing and Wi-Fi connectivity (ESP) from actuation/control (Arduino UNO). The ESP reads DHT, MQ-135, HX711, and other sensors, then sends newline-terminated CSV lines over serial to the UNO. The UNO parses values and drives relays and a servo to operate fans, pump, LEDs, and a mechanical actuator.</p>
    </section>
    <section id="features">
      <h2>Key Features</h2>
      <ul>
        <li>Receive <code>temperature,humidity,gas_ppm,load_grams,water_ml,light_lux</code> from Wi-Fi node (CSV).</li>
        <li>Control 4 relays: Fan1, Fan2, Pump, LEDs.</li>
        <li>Control servo based on load cell reading (HX711).</li>
        <li>Fallback to local analog water & light sensors when no CSV arrives.</li>
        <li>All thresholds configurable in UNO sketch.</li>
      </ul>
    </section>
    <section id="files">
      <h2>Files & Structure</h2>
      <table>
        <thead>
          <tr><th>Filename</th><th>Description</th></tr>
        </thead>
        <tbody>
          <tr><td><code>uno_controller.ino</code></td><td>Arduino UNO main controller — parses CSV and controls relays/servo.</td></tr>
          <tr><td><code>esp_sender.ino</code></td><td>ESP (ESP8266/ESP32) template that reads sensors and sends CSV via Serial.</td></tr>
          <tr><td><code>README.html</code></td><td>This HTML README you are viewing.</td></tr>
          <tr><td><code>wiring_diagram.png</code></td><td>Optional wiring diagram (add manually to repo).</td></tr>
        </tbody>
      </table>
    </section>
    <section id="hardware">
      <h2>Hardware Requirements</h2>
      <ul>
        <li>Arduino UNO</li>
        <li>ESP8266 or ESP32 (Wi-Fi sensor node)</li>
        <li>DHT11 / DHT22 (temp & humidity)</li>
        <li>MQ-135 (gas / CO₂ proxy)</li>
        <li>HX711 + load cell</li>
        <li>Water level analog sensor</li>
        <li>Light sensor (LDR / analog lux)</li>
        <li>4-channel relay module (or MOSFET drivers)</li>
        <li>Servo motor (signal to UNO PWM pin)</li>
        <li>Common ground and appropriate external power supplies for servos/relays</li>
      </ul>
    </section>
    <section id="wiring">
      <h2>Wiring / Pinout (UNO)</h2>
      <p class="muted">Cross TX/RX and always share common ground between UNO and ESP.</p>
      <table>
        <thead><tr><th>UNO</th><th>Connected To</th><th>Notes</th></tr></thead>
        <tbody>
          <tr><td>D10 (SoftwareSerial RX)</td><td>ESP TX</td><td>Receive CSV from ESP</td></tr>
          <tr><td>D11 (SoftwareSerial TX)</td><td>ESP RX</td><td>Optional (not required if UNO only receives)</td></tr>
          <tr><td>D9</td><td>Servo signal</td><td>Use external power for servo</td></tr>
          <tr><td>D2</td><td>Relay1 (Fan1)</td><td>Active HIGH (adjust wiring)</td></tr>
          <tr><td>D3</td><td>Relay2 (Fan2)</td><td></td></tr>
          <tr><td>D4</td><td>Relay3 (Pump)</td><td></td></tr>
          <tr><td>D5</td><td>Relay4 (LEDs)</td><td></td></tr>
          <tr><td>A0</td><td>Water sensor (fallback)</td><td>Analog read</td></tr>
          <tr><td>A1</td><td>Light sensor (fallback)</td><td>Analog read</td></tr>
        </tbody>
      </table>
      <p class="small">Power note: use separate supplies for relays and servo if they draw substantial current. Connect grounds together.</p>
    </section>
    <section id="csv">
      <h2>CSV Communication Format</h2>
      <p>The ESP sends newline-terminated CSV lines over serial. UNO expects:</p>
      <pre><code>temperature,humidity,gas_ppm,load_grams,water_ml,light_lux\n</code></pre>
      <p>Example:</p>
      <pre><code>23.4,56.7,210,600,350,80</code></pre>
      <p>Fields may be empty (e.g., <code>23.4,56.7,210,,350,80</code>) — UNO treats missing numeric fields as <code>NAN</code> for that control.</p>
    </section>
    <section id="uno-usage">
      <h2>UNO Sketch (Usage)</h2>
      <p>Open <code>uno_controller.ino</code> in Arduino IDE. Confirm SoftwareSerial pins match wiring (D10 RX). Upload to UNO. Start the ESP node so it sends CSV at the same baud (default 9600).</p>
      <h3>UNO Sketch (full)</h3>
      <pre><code>// Arduino UNO sketch — receives CSV sensor data from an external WiFi sensor (ESP) over SoftwareSerial
#include &lt;SoftwareSerial.h&gt;
#include &lt;Servo.h&gt;

// SoftwareSerial pins (UNO). Connect ESP TX -> UNO 10, ESP RX -> UNO 11
SoftwareSerial wifiSerial(10, 11);

#define SERVO_PIN 9     // UNO PWM pin for servo
#define RELAY1_PIN 2    // Fan 1
#define RELAY2_PIN 3    // Fan 2
#define RELAY3_PIN 4    // Pump
#define RELAY4_PIN 5    // LEDs

// Local analog sensors (fallback)
#define WATER_SENSOR_PIN A0
#define LIGHT_SENSOR_PIN A1

// Thresholds (same logic as original)
const float TEMP_THRESHOLD_C    = 25.0;   // Temperature °C
const int   HUM_THRESHOLD       = 60;     // Humidity %
const int   GAS_THRESHOLD_PPM   = 200;    // Gas ppm
const long  LOAD_CELL_THRESHOLD = 500;    // grams
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
// Returns true if a full line read into `out` (null-terminated).
bool readCsvLine(String &out, unsigned long timeoutMs = 500) {
  unsigned long start = millis();
  out = "";
  while (millis() - start &lt; timeoutMs) {
    while (wifiSerial.available()) {
      char c = (char)wifiSerial.read();
      if (c == '\r') continue;
      if (c == '\n') {
        if (out.length() &gt; 0) return true;
      } else {
        out += c;
        if (out.length() &gt; 200) return true;
      }
    }
  }
  return false; // timeout or no complete line
}

// Parse CSV: temp,hum,gas,load,water,light
void parseCsvValues(const String &amp;line, float &amp;temp, float &amp;hum, float &amp;gas, float &amp;loadVal, float &amp;waterMl, float &amp;lightLux) {
  temp = hum = gas = loadVal = waterMl = lightLux = NAN;
  int idx = 0;
  int start = 0;
  int len = line.length();
  for (int i = 0; i &lt;= len; ++i) {
    if (i == len || line.charAt(i) == ',') {
      String token = line.substring(start, i);
      token.trim();
      float v = token.toFloat();
      if (token.length() == 0) v = NAN;
      switch (idx) {
        case 0: temp = v; break;
        case 1: hum  = v; break;
        case 2: gas  = v; break;
        case 3: loadVal = v; break;
        case 4: waterMl = v; break;
        case 5: lightLux = v; break;
      }
      idx++;
      start = i + 1;
    }
  }
}

void applyControls(float temperature, float humidity, float gasValue, long loadCellValue, int waterSensorValue_ml, int lightSensorValue_lux) {
  if (!isnan(temperature) && (temperature &gt; TEMP_THRESHOLD_C) || (!isnan(humidity) && (humidity &gt; HUM_THRESHOLD))) {
    digitalWrite(RELAY1_PIN, HIGH);
    fan1State = true;
  } else {
    digitalWrite(RELAY1_PIN, LOW);
    fan1State = false;
  }

  if (!isnan(gasValue) && gasValue &gt; GAS_THRESHOLD_PPM) {
    digitalWrite(RELAY2_PIN, HIGH);
    fan2State = true;
  } else {
    digitalWrite(RELAY2_PIN, LOW);
    fan2State = false;
  }

  if (!isnan(loadCellValue) && loadCellValue &gt; LOAD_CELL_THRESHOLD) {
    myservo.write(90);
    servoState = true;
  } else {
    myservo.write(0);
    servoState = false;
  }

  if (waterSensorValue_ml &gt; WATER_THRESHOLD_ML) {
    digitalWrite(RELAY3_PIN, HIGH);
    pumpState = true;
  } else {
    digitalWrite(RELAY3_PIN, LOW);
    pumpState = false;
  }

  if (lightSensorValue_lux &lt; LIGHT_THRESHOLD_LUX) {
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
  if (loadCellValue &gt;= 0) Serial.print(loadCellValue); else Serial.print("N/A");
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

  if (readCsvLine(line, 800)) {
    parseCsvValues(line, temperature, humidity, gasValue, loadCellValF, waterMlF, lightLuxF);
    if (!isnan(loadCellValF)) loadCellValue = (long)loadCellValF;
    if (!isnan(waterMlF)) waterSensorValue_ml = (int)waterMlF;
    if (!isnan(lightLuxF)) lightSensorValue_lux = (int)lightLuxF;
    Serial.print("Received CSV: "); Serial.println(line);
  } else {
    int rawWater = analogRead(WATER_SENSOR_PIN);
    int rawLight = analogRead(LIGHT_SENSOR_PIN);
    waterSensorValue_ml = map(rawWater, 0, 1023, 0, 1000);
    lightSensorValue_lux = map(rawLight, 0, 1023, 0, 1000);
    Serial.println("No CSV from WiFi sensor — using local analog sensors for water & light.");
  }

  applyControls(temperature, humidity, gasValue, loadCellValue, waterSensorValue_ml, lightSensorValue_lux);
  printStatus(temperature, humidity, gasValue, loadCellValue, waterSensorValue_ml, lightSensorValue_lux);

  delay(3000);
}
</code></pre>
      <p class="small">Adjust thresholds and pins at top of sketch to match your hardware.</p>
    </section>
    <section id="esp-template">
      <h2>ESP Sensor Node — Template</h2>
      <p>Send CSV to UNO (connect ESP TX → UNO D10). Replace placeholder sensor reads with real library calls.</p>
      <pre><code>// Example ESP template (ESP8266 / ESP32)
// Reads sensors and prints CSV to Serial (to UNO via TX pin)

#include &lt;DHT.h&gt;
#define DHT_PIN 2
#define DHT_TYPE DHT11

DHT dht(DHT_PIN, DHT_TYPE);

void setup() {
  Serial.begin(9600); // must match UNO wifiSerial baud
  dht.begin();
  // init other sensors (MQ135, HX711) here
}

void loop() {
  float temp = NAN, hum = NAN;
  float gas_ppm = NAN;
  float load_grams = NAN;
  float water_ml = NAN;
  float light_lux = NAN;

  temp = dht.readTemperature();
  hum  = dht.readHumidity();
  // gas_ppm = readMQ135();
  // load_grams = readHX711();
  // water_ml = map(analogRead(waterPin), 0, 1023, 0, 1000);
  // light_lux = map(analogRead(lightPin), 0, 1023, 0, 1000);

  if (isnan(temp)) temp = 25.0;
  if (isnan(hum))  hum  = 50.0;
  if (isnan(gas_ppm)) gas_ppm = 100.0;
  if (isnan(load_grams)) load_grams = 0;
  if (isnan(water_ml)) water_ml = 200;
  if (isnan(light_lux)) light_lux = 300;

  Serial.print(temp); Serial.print(",");
  Serial.print(hum);  Serial.print(",");
  Serial.print(gas_ppm); Serial.print(",");
  Serial.print(load_grams); Serial.print(",");
  Serial.print(water_ml); Serial.print(",");
  Serial.println(light_lux);

  delay(3000);
}
</code></pre>
    </section>
    <section id="tuning">
      <h2>Thresholds, Mapping & Calibration</h2>
      <p>Default thresholds in UNO sketch:</p>
      <pre><code>const float TEMP_THRESHOLD_C    = 25.0;
const int   HUM_THRESHOLD       = 60;
const int   GAS_THRESHOLD_PPM   = 200;
const long  LOAD_CELL_THRESHOLD = 500;
const int   WATER_THRESHOLD_ML  = 300;
const int   LIGHT_THRESHOLD_LUX = 100;</code></pre>
      <h3>Calibration tips</h3>
      <ul>
        <li><strong>HX711:</strong> Use calibration sketch to compute <em>calibration_factor</em> — convert raw to grams.</li>
        <li><strong>MQ-135:</strong> Warm up the sensor for 24–48 hours, calibrate baseline in fresh air; treat as relative indicator.</li>
        <li><strong>Water & Light:</strong> Adjust <code>map()</code> ranges to match actual sensor outputs and physical units.</li>
      </ul>
    </section>
    <section id="troubleshoot">
      <h2>Troubleshooting</h2>
      <ul>
        <li><strong>No CSV received:</strong> Check ESP TX → UNO D10, common ground, and that ESP prints newline <code>\n</code>. Baud default: 9600.</li>
        <li><strong>Relays click but device not powered:</strong> Check external power for relays and loads.</li>
        <li><strong>UNO resets:</strong> Servo or relay inrush — use separate power supply and add decoupling capacitors.</li>
        <li><strong>NAN values:</strong> Sensor read failed on ESP — verify sensor wiring and libraries.</li>
      </ul>
    </section>
    <section id="safety">
      <h2>Safety & Best Practices</h2>
      <ul>
        <li>Do not power motors/large loads from UNO 5V pin.</li>
        <li>Use opto-isolated relays or proper isolation when switching mains.</li>
        <li>Use flyback diodes on inductive loads and proper fusing for main circuits.</li>
        <li>Always share ground between ESP and UNO when using serial communication.</li>
      </ul>
    </section>
    <section id="license">
      <h2>License & Contributors</h2>
      <p class="muted"><strong>License:</strong> MIT — include a <code>LICENSE</code> file with full MIT text in your repository.</p>
      <p class="muted"><strong>Contributors:</strong> You / Your Name (primary). Libraries: <code>DHT</code>, <code>HX711</code>, <code>MQ135</code> (if used), <code>Servo</code>, <code>SoftwareSerial</code>.</p>
    </section>
    <footer>
      <p class="small">Need help generating the actual wiring diagram, creating the ESP sensor code with MQ-135 & HX711 integrations, or making a dashboard? I can add those next.</p>
      <p>&copy; 2025 — Smart Greenhouse Project</p>
    </footer>
  </div>
</body>
