#define BLYNK_TEMPLATE_ID "TMPL66CpvZNde"
#define BLYNK_TEMPLATE_NAME "WATERFLOW monitoring"
#define BLYNK_AUTH_TOKEN
"Z3wj1NXCirvntue1gc0oeXxzoBOM9Wx8"
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DS1302.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#define FLOW_SENSOR_PIN D7
#define CALIBRATION_FACTOR 7.5
#define LED_PIN D4
#define EEPROM_SIZE 512
volatile int pulseCount = 0;
float flowRate = 0.0;
float totalLiters = 0.0;
float lastHistory = 0.0; 
unsigned long lastUpdate = 0;
unsigned long lastSaveTime = 0;
const unsigned long flowInterval = 200;
const unsigned long saveInterval = 10000; 
bool ledState = false;
unsigned long blinkTime = 0;
// Buffer untuk rata-rata flow rate
#define BUFFER_SIZE 10
float flowRateBuffer[BUFFER_SIZE] = {0};
int bufferIndex = 0;
// WiFi Credentials
char ssid[] = "MARSH";
char pass[] = "minako01";
// RTC DS1302 Pins
#define DS1302_DATA D5
#define DS1302_CLK D6
#define DS1302_RST D8
DS1302 rtc(DS1302_DATA, DS1302_CLK, DS1302_RST);
BlynkTimer timer;
// NTP client setup for WIB (UTC +7)
WiFiUDP udp;
NTPClient timeClient(udp, "pool.ntp.org", 7 * 3600, 60000);
// LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2);
// Fungsi membaca nilai totalLiters dari EEPROM
void readFromEEPROM() {EEPROM.begin(EEPROM_SIZE);
EEPROM.get(0, totalLiters);
EEPROM.end();
if (totalLiters < 0 || totalLiters > 100) {
totalLiters = 0.0;
}
}
// Fungsi menyimpan nilai totalLiters ke EEPROM
void saveToEEPROM() {
EEPROM.begin(EEPROM_SIZE);
EEPROM.put(0, totalLiters);
EEPROM.commit();
EEPROM.end();
}
// Interrupt untuk menghitung pulsa
void IRAM_ATTR pulseCounter() {
pulseCount++;
}
// Perhitungan flow rate dan update total liter air
void updateFlow() {
unsigned long currentTime = millis();
if (currentTime - lastUpdate >= flowInterval) {
float elapsedTime = (currentTime - lastUpdate) / 1000.0; //
flowRate = (pulseCount / (CALIBRATION_FACTOR * 60)) /
elapsedTime;
lastUpdate = currentTime;
pulseCount = 0;// Tambahkan rata-rata flow rate ke buffer
flowRateBuffer[bufferIndex] = flowRate;
bufferIndex = (bufferIndex + 1) % BUFFER_SIZE;
// Hitung rata-rata flow rate
float averageFlowRate = 0.0;
for (int i = 0; i < BUFFER_SIZE; i++) {
averageFlowRate += flowRateBuffer[i];
}
averageFlowRate /= BUFFER_SIZE;
// Update total liter air
totalLiters += averageFlowRate * elapsedTime + 100;
// Debugging
Serial.print("Flow Rate: ");
Serial.print(flowRate, 2);
Serial.print(" L/s, Total Liters: ");
Serial.println(totalLiters, 2);
// Update LCD dan Blynk
lcd.setCursor(0, 1);
lcd.print("Total Air: ");
lcd.print(totalLiters, 2);
lcd.print(" L");
Blynk.virtualWrite(V0, averageFlowRate);
Blynk.virtualWrite(V1, totalLiters);
// Cek jika total liter melebihi 7if (totalLiters > 7.0) {
lcd.setCursor(0, 0);
lcd.print("Pemakaian Maks");
digitalWrite(LED_PIN, HIGH);
isMaxUsage = true; 
} else {
digitalWrite(LED_PIN, LOW); 
isMaxUsage = false; 
lcd.setCursor(0, 0); 
liter < 7
lcd.print(" "); 
}
if (currentTime - lastSaveTime >= saveInterval) {
saveToEEPROM();
lastSaveTime = currentTime;
}
}
}
void handleLED() {
if (isMaxUsage) {
unsigned long currentMillis = millis();
if (currentMillis - blinkTime >= 500) {
ledState = !ledState;
digitalWrite(LED_PIN, ledState);
blinkTime = currentMillis;
}} else {
digitalWrite(LED_PIN, LOW);
}
}
void resetTotalLitersAtTime() {
timeClient.update();
int currentHour = timeClient.getHours();
int currentMinute = timeClient.getMinutes();
static bool historySaved = false;
if (currentHour == 3 && currentMinute == 34 && !historySaved) {
lastHistory = totalLiters; 
Blynk.virtualWrite(V2, lastHistory); 
totalLiters = 0.0;
Serial.println("Total liters has been reset to 0.");
Blynk.virtualWrite(V1, totalLiters);
saveToEEPROM();
historySaved = true;
}
if (currentHour != 3 || currentMinute != 34) {
historySaved = false;
}
Blynk.virtualWrite(V2, lastHistory);
}void setup() {
Serial.begin(9600);
readFromEEPROM();
Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
pinMode(LED_PIN, OUTPUT);
attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN),
pulseCounter, FALLING);
WiFi.begin(ssid, pass);
while (WiFi.status() != WL_CONNECTED) {
delay(1000);
Serial.println("Connecting to WiFi...");
}
Serial.println("Connected to WiFi");
timeClient.begin();
timeClient.update();
lcd.init();
lcd.backlight();
Blynk.virtualWrite(V0, 0);
Blynk.virtualWrite(V1, 0);
Blynk.virtualWrite(V2, lastHistory); 
}
void loop() {
Blynk.run();handleLED();
resetTotalLitersAtTime();
updateFlow();
}
