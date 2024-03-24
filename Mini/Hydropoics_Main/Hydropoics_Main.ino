#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <BlynkSimpleEsp8266.h>

// Pin definitions
#define RELAY_PIN D8
#define LED_STRIP_PIN D0
#define LDR_PIN A0
#define TEMP_SENSOR_PIN D3
#define TDS_SENSOR_PIN D4
#define TRIGGER_PIN D5
#define ECHO_PIN D6
#define MICROPUMP_PIN D7
//Blynk IOT
#define BLYNK_TEMPLATE_ID "       " // Add your BLYNK_TEMPLATE_ID
#define BLYNK_TEMPLATE_NAME "     " // Add your BLYNK_TEMPLATE_NAME

char auth[] = "         "; // Add your Authentication ID
char ssid[] = "      "; // Add Your Wifi SSID
char pass[] = "     "; // Add your Wifi password

namespace device {
  float aref = 4.1;
}
 
namespace sensor {
  float ec = 0;
  unsigned int tds = 0;
  float waterTemp = 0;
  float ecCalibration = 0.5;
}
// Sensor objects
OneWire oneWire(TEMP_SENSOR_PIN);
DallasTemperature tempSensor(&oneWire);
float tempCelsius;
float tempFahrenheit;

// OLED display object
Adafruit_SSD1306 display(128, 64, &Wire, -1);

//Nutrients value
int NutrientsValue;

// Function prototypes
void turnOnWaterPump();
void turnOffWaterPump();
void turnOnLEDStrip();
void turnOffLEDStrip();
float readTemperature();
float convertToFahrenheit(float celsius);
float readTDS();
float readWaterLevel();
void addNutrients();
 
  // This function will be called when the device initially connects to the Blynk app
  BLYNK_CONNECTED() {
    // Synchronizes values on connection
    Blynk.syncVirtual(V0);
    Blynk.syncVirtual(V1);
    Blynk.syncVirtual(V2);
    Blynk.syncVirtual(V3);
    Blynk.syncVirtual(V4);
    Blynk.syncVirtual(V5);
    Blynk.syncVirtual(V6);
    Blynk.syncVirtual(V7);
  }

// Control Switch using Blynk
BLYNK_WRITE(V3) {   // called when the Water Pump switch widget changes state
  int state = param.asInt();
  if (state == HIGH) {
    turnOnWaterPump();
  } else {
    turnOffWaterPump();
  }
}

BLYNK_WRITE(V4) {   // called when the LED Lights switch widget changes state
  int state = param.asInt();
  if (state == HIGH) {
    turnOnLEDStrip();
  } else {
    turnOffLEDStrip();
  }
}

BLYNK_WRITE(V5) {   // called when the Nutrients Valve switch widget changes state
  int state = param.asInt();
  if (state == HIGH) {
    addNutrients();
  }
}

BLYNK_WRITE(V8) {
  NutrientsValue = param.asInt();
  Serial.print("Nutrients value: ");
  Serial.println(NutrientsValue);
}

void setup() {
  // Blynk connection
  Blynk.begin(auth, ssid, pass);

  // Initialize serial port
  Serial.begin(115200);

  // Initialize WiFi
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  WiFi.setSleepMode(WIFI_NONE_SLEEP);


  // Initialize pins
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_STRIP_PIN, OUTPUT);
  pinMode(MICROPUMP_PIN, OUTPUT);
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(TEMP_SENSOR_PIN, INPUT);
  pinMode(TDS_SENSOR_PIN, INPUT);

  // Initialize sensor objects
  tempSensor.begin();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  // Display initialization message on OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Initializing...");
  display.display();

  // Wait for 5 seconds to let the OLED display the message
  delay(5000);

}

void loop() {
   // Blynk.run()
   Blynk.run();
   // Turn on water pump
   turnOnWaterPump();

   display.clearDisplay();
   display.setTextSize(1);
   display.setTextColor(WHITE);
   display.setCursor(0, 0);
   display.println("PUMP ON...");
   display.display();

   Blynk.run();

   // Wait for 20 minutes
   delay(1*60*1000);

   // Turn off water pump and turn on relay
    turnOffWaterPump();

   display.clearDisplay();
   display.setTextSize(1);
   display.setTextColor(WHITE);
   display.setCursor(0, 0);
   display.println("PUMP OFF...");
   display.display();

   delay(3000);

   Blynk.run();

   // Read and display TDS, EC, and PPM
    float ppm = readTDS();
    float ec = ppm / 1000.0;
    float cf = ppm / 10.0;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("TDS/CF/EC:");
    display.setTextSize(2);
    display.setCursor(0, 10);
    display.print(ppm);
    display.print("ppm");
    display.setCursor(0, 30);
    display.print(cf);
    display.print("cF");
    display.setCursor(0, 50);
    display.print(ec);
    display.print("S/cm");
    display.display();
    delay(5000);

    // Check for electrical conductivity and add nutrients if necessary
    if (ppm < 1000.0) {
      digitalWrite(MICROPUMP_PIN, HIGH);  // Open solenoid valve to add nutrients
      delay(5000);  // Wait for nutrients to mix
      digitalWrite(MICROPUMP_PIN, LOW);  // Close solenoid valve
      // Read and display TDS, EC, and PPM
      float ppm = readTDS();
      float ec = ppm / 1000.0;
      float cf = ppm / 10.0;
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(0, 0);
      display.println("DONE");
      display.setTextSize(2);
      display.setCursor(0, 10);
      display.print(ppm);
      display.print("ppm");
      display.setCursor(0, 30);
      display.print(cf);
      display.print("cF");
      display.setCursor(0, 50);
      display.print(ec);
      display.print("S/cm");
      display.display();
      delay(25000);
    }

   // Check if water pump is turned off
    if (digitalRead(RELAY_PIN) == LOW) {

    // Read and display temperature
    tempCelsius = readTemperature();
    tempFahrenheit = convertToFahrenheit(tempCelsius);
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("Temperature:");
    display.setTextSize(2);
    display.setCursor(0, 10);
    display.print(tempCelsius);
    display.print(" C");
    display.setCursor(0, 30);
    display.print(tempFahrenheit);
    display.print(" F");
    display.display();
    delay(5000);

    // Read and display water level
    float waterLevel = readWaterLevel();
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("Water Level:");
    display.setTextSize(2);
    display.setCursor(0, 10);
    display.print(waterLevel);
    display.display();
    delay(5000);
    
    Blynk.run();

    //Gauge Widget configuration 
    Blynk.virtualWrite(V0, readWaterLevel());    // update Water Level gauge widget
    Blynk.virtualWrite(V1, readTemperature());   // update Temperature gauge widget
    Blynk.virtualWrite(V2, ec);                  // update Nutrients EC Level gauge widget
    Blynk.virtualWrite(V6, cf);                 // update Nutrients CF Level gauge widget
    Blynk.virtualWrite(V7, ppm);               // update Nutrients in PPM gauge widget
    Blynk.run();
    delay(5000);

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("LDR.....");
    display.display();

    for(int i=0 ;i<4;i++)
    {
    // Check for darkness and turn on LED strip
    int ldrValue = analogRead(LDR_PIN);

    if (ldrValue < 100) {  // Adjust this threshold based on your LDR reading in the dark
    digitalWrite(LED_STRIP_PIN, HIGH);  // Turn on the LED strip (assuming LED_STRIP_PIN is correct)
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("LIGHTS ON...");
    display.display();
    } else {
     digitalWrite(LED_STRIP_PIN, LOW);  // Turn off the LED strip (assuming LED_STRIP_PIN is correct)
    }
    delay(10000);
    }

    Blynk.run();

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("PUMP OFF...");
    display.display();

    // Wait for 15 minutes
    delay(1*60*1000);

    Blynk.run();

   }
}

void turnOnWaterPump() {
  digitalWrite(RELAY_PIN, HIGH);
}

void turnOffWaterPump() {
  digitalWrite(RELAY_PIN, LOW);
}

void turnOnLEDStrip() {
  digitalWrite(LED_STRIP_PIN, HIGH);
}

void turnOffLEDStrip() {
  digitalWrite(LED_STRIP_PIN, LOW);
}   

float readTemperature() {
  tempSensor.requestTemperatures();
  return tempSensor.getTempCByIndex(0);
}

float convertToFahrenheit(float celsius) {
  return celsius * 1.8 + 32;
}

float readTDS() {
  tempSensor.requestTemperatures();
  sensor::waterTemp = tempSensor.getTempCByIndex(0);
  float rawEc = analogRead(TDS_SENSOR_PIN) * device::aref / 1024.0;
  float temperatureCoefficient = 1.0 + 0.02 * (sensor::waterTemp - 25.0);
  sensor::ec = (rawEc / temperatureCoefficient) * sensor::ecCalibration;
  sensor::tds = (133.42 * pow(sensor::ec, 3) - 255.86 * sensor::ec * sensor::ec + 857.39 * sensor::ec) * 0.5;
  return sensor::tds;
}

float readWaterLevel() {
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  float distance = duration * 0.034 / 2.0;
  float waterLevel = 35.0 - distance;
  if (waterLevel > 30.0) {
    waterLevel = 30.0;
  }
  if (waterLevel < 5.0) {
    waterLevel = 5.0;
  }
  return waterLevel;
}

void addNutrients() {
  digitalWrite(MICROPUMP_PIN, HIGH);  // Open solenoid valve to add nutrients
  delay(5000);  // Wait for nutrients to mix
  digitalWrite(MICROPUMP_PIN, LOW);  // Close solenoid valve
  // Read and display TDS, EC, and PPM
  float ppm = readTDS();
  float ec = ppm / 1000.0;
  float cf = ppm / 10.0;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("DONE");
  display.setTextSize(2);
  display.setCursor(0, 10);
  display.print(ppm);
  display.print("ppm");
  display.setCursor(0, 30);
  display.print(cf);
  display.print("cF");
  display.setCursor(0, 50);
  display.print(ec);
  display.print("S/cm");
  display.display();
  delay(25000);
}
