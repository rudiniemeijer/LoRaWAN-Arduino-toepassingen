
// 5Groningen Sensor Stick
// All rights reserved
// Written by Rudi Niemeijer - Testconsultancy Groep
// Version 1.0 - 2-02-2017 - Creation
//
// This is the firmware for Sensor Stick, a prototype for a future 5G application. This
// particular device has consists of the following hardware:
// Sodaq One v2
// 3 x DS18B20 OneWire temperature sensors with 12 bits resolution                             .
// All is mounted in a PVC housing with the '5G' branding on it

#include <OneWire.h>
#include <DallasTemperature.h>

// Various definitions
#define MAX_ONE_WIRE_SENSORS 3        // Expected number of DS18B20 temperature sensors
#define ONE_WIRE_BUS 2                // D2 is used as OneWire port
#define TEMPERATURE_PRECISION 12      // Highest resolution possible
#define LOOP_DELAY_SECONDS 180        // Delay after each measurement
#define DISABLE_LORA false
#define MAX_MOIST_SENSORS 3           // Expected number of soil moisure sensors
#define ENABLE_PIN 11                 // Must be high to enable pins 2, 3, 6, 7, 8 and 9 to function
#define RN2483 Serial1

// Various variable declarations
int actualNumberOfTemperatureSensors;     // Contains the actual number of temperature sensors found
float lastMeasuredTemperature[MAX_ONE_WIRE_SENSORS];
int moistSensorPin[MAX_MOIST_SENSORS] = {A6, A7, A8};  // Must be connected continuously, i.e. no gaps
int actualNumberOfMoistSensors;       // Contains the actual number of sensors found
float lastMeasuredMoist[MAX_MOIST_SENSORS];
byte crlf[2] = {0x0D,0x0A};         // Used to terminate RN2486 commands
bool networkJoined = false;         // Keep track if we're connected
bool recentSendSuccess = false;     // Some indication for succesful send data
bool noUSBSerial = false;           // Let's assume the USB cable is connected

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature dsSensors(&oneWire);  // Pass our oneWire reference to Dallas Temperature
DeviceAddress temperatureSensor[MAX_ONE_WIRE_SENSORS];  // Array that contains sensoraddresses

void setLedColor(float hue) {
  float r, g, b = 0;
  r = hue * 6 - 4;
  g = 0;
  b = (1 - hue) * 6;

  if (hue < 0.666) {
    r = 0;
    g = 4 - hue * 6;
    b = hue * 6 - 2;
  }
 
  if (hue < 0.333) {
    r = 2 - hue * 6;
    g = hue * 6;
    b = 0;
  } 
    
  if (r > 1) {
    r = 1;
  }
  if (g > 1) {
    g = 1;
  }
  if (b > 1) {
    b = 1;
  }
  if (r < 0) {
    r = 0;
  }
  if (g < 0) {
    g = 0;
  }
  if (b < 0) {
    b = 0;
  }
  r = r * 255.0;
  g = g * 255.0;
  b = b * 255.0;

  // debugPrint("HUE="+String(hue,DEC)+", R="+String(r,0)+", G="+String(g,0)+", B="+String(b,0));
  if (hue > 0) {
    analogWrite(LED_RED, 255-r);
    analogWrite(LED_GREEN, 255-g);
    analogWrite(LED_BLUE, 255-b);
  } else {
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_BLUE, HIGH);
  }
}  

// Enable devices that are powered by the Sodaq One voltage regulator
void externalPower(bool state) {
  if (state == true) {
    digitalWrite(ENABLE_PIN_IO, HIGH);
    digitalWrite(ENABLE_PIN, HIGH);
    debugPrint("Devices and ports are enabled.");
  } else {
    digitalWrite(ENABLE_PIN_IO, LOW);
    digitalWrite(ENABLE_PIN, LOW);
    debugPrint("Devices and ports are disabled.");
  }
  delay(500); // Give our devices some time to wake up
}

void initSensors() {
  int i;
  dsSensors.begin();
  for (i = 0; i < actualNumberOfTemperatureSensors; i++) {
    dsSensors.getAddress(temperatureSensor[i], i);
    dsSensors.setResolution(temperatureSensor[i], TEMPERATURE_PRECISION);
  } 
}

// Initialise the RN2486 LoRa device
bool initLora() {
  String response;
  RN2483.begin(57600);
  while (!RN2483) {
    delay(100);
  }
  
  sendCommand("sys reset");
  delay(1000);
  response = getResponse();
    
  sendCommand("mac join otaa"); 
  response = getResponse();
  
  if (response == "ok") {
    delay(6000);
    response = getResponse();
    if (response == "accepted") {
      // There. A valid LoRa connection
      networkJoined = true;
    } else {
      // Denied. Either no free channels or something else
      networkJoined = false;
    }
  } else {  // not ok
    // Not a wanted response. Something with the hardware
    // We might want to throw a panic here
    networkJoined = false;
  }  
  RN2483.end();

  return networkJoined;
}

void sendCommand(String cmd) {
  RN2483.print(cmd);
  RN2483.write(crlf, 2);
  debugPrint("> "+cmd);
  delay(1000);
}

String getResponse() {
  String response = "";

  while (!RN2483.available()) { // Linger here while no response
  } // Might be better to create a timeout after a few seconds

  while (RN2483.available()) {
    response = response + char(RN2483.read());
  } 
  response = response + "";
  response.trim();
  debugPrint("< "+response);
  return response;
}

String getAddress(DeviceAddress device) {
  String s = "";
  for (byte i = 0; i < 8; i++) {
    s = s + hexPair(device[i]);
  }
  return s;
}

float getTemperature(DeviceAddress device) {
  return dsSensors.getTempC(device);
}

void getAllTemperatures() {
  int i;
  dsSensors.requestTemperatures();
  for (i = 0; i < actualNumberOfTemperatureSensors; i++) {
    lastMeasuredTemperature[i] = getTemperature(temperatureSensor[i]);
  }
}

void getAllMoistMeasurements() {
  int i;
  for (i = 0; i < actualNumberOfMoistSensors; i++) {
    lastMeasuredMoist[i] = (analogRead(moistSensorPin[i]) / 1023.0) * 100;
  }
}

String hexPair(byte number) {
  // Return a hex string in 2 character positions
  String s;
  s = String(number, HEX);
  if (s.length() < 2) {
    s = "0" + s;
  }
  return s;
}

void sendRecentMeasurements() {
  // Assume sensors have been read recently
  int i, j;
  byte b1, b2;
  String payload, response = "";
  if (networkJoined) {
    for (i = 0; i < actualNumberOfTemperatureSensors; i++) {
      j = lastMeasuredTemperature[i] * 1000;
      if (j < 256) {
        payload = payload + "00" + hexPair(j); 
      } else {
        b1 = j / 256;
        b2 = j - (b1 * 256);
        payload = payload + hexPair(b1) + hexPair(b2);
      }
    }
    for (i = 0; i < actualNumberOfMoistSensors; i++) {
      j = lastMeasuredMoist[i] * 10;
      if (j < 256) {
        payload = payload + "00" + hexPair(j); 
      } else {
        b1 = j / 256;
        b2 = j - (b1 * 256);
        payload = payload + hexPair(b1) + hexPair(b2);
      }
    }
    debugPrint("We have this data to send: " + payload);
    recentSendSuccess = false;
    RN2483.begin(57600);
    while (!RN2483) {
      delay(100);
    }
    sendCommand("mac tx cnf 1 " + payload);
    delay(2000);
    response = getResponse();
    if (response == "ok") {
      recentSendSuccess = true;
      debugPrint("Data has been sent succesfully.");
      delay(2000);
      response = getResponse();
    }
    RN2483.end();  
  }
}

void setup() {
  int i;
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(ENABLE_PIN_IO, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);
  externalPower(true);

  debugBegin(57600);
  debugPrint("5GRONINGEN SENSOR STICK V1.0");
  initSensors();
  actualNumberOfTemperatureSensors = dsSensors.getDeviceCount();
  debugPrint("We have "+String(actualNumberOfTemperatureSensors, DEC)+" DS18B20 temperature sensors.");
  for (i = 0; i < actualNumberOfTemperatureSensors; i++) {
    dsSensors.getAddress(temperatureSensor[i], i);
    dsSensors.setResolution(temperatureSensor[i], TEMPERATURE_PRECISION);
    debugPrint("Meeting temperature sensor " + String(i, DEC) + ". Hi "+getAddress(temperatureSensor[i]));
  }
  actualNumberOfMoistSensors = MAX_MOIST_SENSORS;
  debugPrint("Assume we have " + String(actualNumberOfMoistSensors, DEC) + " soil moisture sensors (they're not very communicative).");
  debugPrint("Setup done.");
  debugPrint("\nEntering the main loop.\n");
}

void debugBegin(int b) {
  SerialUSB.begin(b);
  int i = 0;
  while (!SerialUSB) {
    delay(100);   
    i = i + 1;
    if (i > 50) {
      noUSBSerial = true;
      break;
    }
  } 
}
void debugPrint(String s) {
  if (!noUSBSerial) {
    SerialUSB.println(s);
  }
}

void loop() {
  int i;
  setLedColor(0.1);
  setLedColor(0.2);
  initSensors();
  setLedColor(0.3);
  if (!DISABLE_LORA) {
    initLora();
  }
  setLedColor(0.4);
  getAllTemperatures();
  setLedColor(0.5);
  getAllMoistMeasurements();
  setLedColor(0.6);
  for (i = 0; i < actualNumberOfTemperatureSensors; i++) {
    debugPrint("Temperature reading of sensor "+String(i, DEC)+" is "+String(lastMeasuredTemperature[i], 4)+" degrees Centigrade.");
  }
  for (i = 0; i < actualNumberOfMoistSensors; i++) {
    debugPrint("Soil moist percentage of sensor "+String(i, DEC)+" is "+String(lastMeasuredMoist[i], 1)+" percent."); 
  }
  setLedColor(0.7);
  if (!DISABLE_LORA) {
    sendRecentMeasurements();
  }
  setLedColor(0.8);
  externalPower(false);
  debugPrint("Sleeping for " + String(LOOP_DELAY_SECONDS, DEC) + " seconds.");
  setLedColor(-1);
  externalPower(false);
  delay(LOOP_DELAY_SECONDS * 1000); 
}
