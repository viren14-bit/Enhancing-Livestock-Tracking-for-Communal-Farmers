#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- OLED setup ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// --- Pins ---
#define DHTPIN 2        
#define DHTTYPE DHT22
#define FAN_PIN 3       
#define IR_PIN 4        // IR sensor (digital output module)
#define WATER_PIN 5     // Switch simulating water sensor (digital)
#define PUMP_PIN 6      

DHT dht(DHTPIN, DHTTYPE);

int henCount = 0;
bool henPreviouslyPresent = false;

void setup() {
  Serial.begin(9600);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("[ERROR] SSD1306 allocation failed"));
    for(;;);
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 20);
  display.println("Chicken");
  display.setCursor(10, 40);
  display.println("Farming");
  display.display();
  delay(2000);

  dht.begin();
  pinMode(FAN_PIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(IR_PIN, INPUT);
  pinMode(WATER_PIN, INPUT_PULLUP);  // button/switch: LOW when pressed

  Serial.println(F("=== Smart Poultry Automation Simulation Started ==="));
}

void loop() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // ---- 1. Temperature + Fan Control ----
  float temp = dht.readTemperature();

  if (isnan(temp)) {
    Serial.println(F("[ERROR] Failed to read from DHT sensor!"));
    display.setCursor(0, 0);
    display.println("Temp: ERROR");
  } else {
    Serial.print(F("[DHT] Temperature: "));
    Serial.print(temp);
    Serial.println(F(" C"));

    display.setCursor(0, 0);
    display.print("Temp: ");
    display.print(temp);
    display.println(" C");

    if (temp > 30) {
      digitalWrite(FAN_PIN, HIGH);
      Serial.println(F("[FAN] Fan ON (Temperature > 30C)"));
      display.setCursor(0, 10);
      display.println("Fan: ON");
    } else {
      digitalWrite(FAN_PIN, LOW);
      Serial.println(F("[FAN] Fan OFF (Temperature <= 30C)"));
      display.setCursor(0, 10);
      display.println("Fan: OFF");
    }
  }

  // ---- 2. Hen Presence Detection (IR used as RFID) ----
  int irState = digitalRead(IR_PIN);
  display.setCursor(0, 25);

  if (irState == 0) {   // IR detects object
    Serial.println(F("[RFID/IR] Hen Present (Object Detected)"));
    display.println("Hen: Present");

    if (!henPreviouslyPresent) {
      henCount++;
      henPreviouslyPresent = true;
    }
  } else {
    Serial.println(F("[RFID/IR] Hen Not Present"));
    display.println("Hen: Not Present");
    henPreviouslyPresent = false;
  }

  // Display hen count
  display.setCursor(0, 35);
  display.print("Hen Count: ");
  display.println(henCount);

  // ---- 3. Water Pump Control ----
  int waterState = digitalRead(WATER_PIN);
  display.setCursor(0, 50);

  if (waterState == LOW) {   // pressed/triggered = low water
    digitalWrite(PUMP_PIN, HIGH);
    Serial.println(F("[PUMP] Pump ON (Water LOW)"));
    display.println("Pump: ON");
  } else {
    digitalWrite(PUMP_PIN, LOW);
    Serial.println(F("[PUMP] Pump OFF (Water OK)"));
    display.println("Pump: OFF");
  }

  display.display();
  Serial.println(F("--------------------------------------------------"));
  delay(2000);
}
