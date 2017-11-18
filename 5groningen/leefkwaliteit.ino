// 5Groningen Leefkwaliteit sensor
// All rights reserved
// Written by Rudi Niemeijer - Testconsultancy Groep
// Version 1.0 - 23-01-2017 - Creation
//
// This is the firmware for the Leefkwaliteit Sensor, a prototype for a future 5G application. This
// particular device has consists of the following hardware:
// Arduino Leonardo
// PIR-sensor                               .
// SC6Dlite serial 6-digital led display    .
// MH-Z14A CO2 sensor                       .
// RN2483 LoRaWAN module                    .
// 5V USB Power Bank
// All is mounted in a dedicated box with 5Groningen artwork on top.
//
// The working is as follows:
// The Leonardo will take a CO2 measurement from the MH-Z14A. The measured value is added to the running average
// and the value is divided by two. The resuling value is sent via LoRaWAN to the backend.
// The Leonardo will count 30 seconds down. 

#include <SoftwareSerial.h>

#define SensorRX 10           // Yellow
#define SensorTX 11           // White
#define LoraRX     8          // Yellow
#define LoraTX     9          // White
#define displayRX      5
#define displayTX      6
#define defaultWakeTime 30
#define pirPin 13
#define displayID 255
#define warmUpSeconds 0       // CO2 sensor seconds before first measurement can be taken, should be 180
#define sendEvery 60

bool motionDetected = false;        // Once motion is detected, wakeTime is set to defaultWakeTime
bool motionLastTimeChecked = false; // Was there motion last time we checked?
int co2AveragePpm = 0;              // Keep track of average running CO2 ppm
bool lastCO2measurementOK = false;  // Last CO2 measurement was OK
int wakeTime = 0;                   // Seconds before display is switched off
bool displayIsOn = false;           // Status of the display: on or off 
bool lifeDot = false;               // Dot led 6
byte crlf[2] = {0x0D,0x0A};         // Used to terminate RN2486 commands
bool networkJoined = false;         // Keep track if we're connected
bool recentSendSuccess = false;           // Some indication for succesful send data
int cycles = 0;
bool bootState = true;              // If we're booting, sensor is not trustable
long startMillis = millis();        // Milliseconds since program start
bool demoMode = false;              // Demo mode is entered when invalid CO2 reading
String r;                           // Latest response from the LoRa module

// Define the serial ports
SoftwareSerial co2sensor (SensorRX, SensorTX);
SoftwareSerial lora (LoraRX, LoraTX);
SoftwareSerial leddisplay (displayRX, displayTX);

int runningSeconds() {  // Only valid in the frist 57 days after startup
  return (millis() - startMillis) / 1000;
}

// Assume the lora to be open
void sendCommand(String cmd) {
  lora.print(cmd);
  lora.write(crlf, 2);
  delay(1000);
}

// Assume the lora port to be open
String getResponse() {
  String response = "";
  while (!lora.available()) { // Linger here while no response
  } // Might be better to create a timeout after a few seconds
  
  while (lora.available()) {
    response = response + char(lora.read());
  } 
  response = response + "";
  response.trim();
  return response;
}

// Initialise the RN2486 LoRa device
bool initLora() {
  String r;
  lora.begin(57600);
  delay(100);
  
  sendCommand("sys reset");
  r= getResponse();
    
  sendCommand("mac join otaa"); 
  r = getResponse();
  
  if (r == "ok") {
    r = getResponse();
    if (r == "accepted") {
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
  lora.end();
  return networkJoined;
}

void sendMeasurement(int payload) {
  String n = "";
  byte b1, b2;
  if (payload < 256) {
    n = String(payload, HEX);
  } else {
    b1 = payload / 256;
    b2 = payload - (b1 * 256);
    n = String(b1, HEX) + String(b2, HEX);
  }
  transmitData(n);
}

int transmitData(String payload) {
  String r;
  recentSendSuccess = false;
  if (networkJoined) {
    lora.begin(57600);

    sendCommand("mac tx cnf 1 " + payload);
    r = getResponse();
    if (r == "ok") {
      r = getResponse();
      if (r != "mac_tx_ok") {
        // We got data back! Not sure what to do with it though.
        recentSendSuccess = true;
      } else {
        // No data back, just a succesful send
        recentSendSuccess = true;
        // mac_tx_ok
      }
    } else {
      recentSendSuccess = false;
      // Invalid parameters?
    }
    lora.end();
  }
  return recentSendSuccess;
}

// display a message
// 
void displayMessage(String message) {
  leddisplay.begin(9600);
  delay(100);

  if (bootState) {
    if (runningSeconds() > 30) {
      bootState = false;
    } else {
      message = "boot " + String(3 - (runningSeconds() / 10));
    }
  }

  // display the message here
  while (message.length() < 6) {
    message = " " + message;
  }
  leddisplay.write(displayID);
  leddisplay.write('b');
  if (wakeTime > 0) {
    leddisplay.println(message);
  } else {
    leddisplay.println("");
  }
  leddisplay.end();
}

void checkMotionPresent() {
  int pinHigh;
  motionLastTimeChecked = digitalRead(pirPin);
  if (motionLastTimeChecked) {
    motionDetected = true;
    if (motionDetected) {
      wakeTime = defaultWakeTime;  // Reset wakeTime counter to defaultWakeTime seconds
    }
  } else {
    if (wakeTime == 0) {
      motionDetected = false;
    }
  }
}

void dotStatus() {
  byte dots = 0;

  if (motionDetected) {
    bitSet(dots, 0);
    Serial.println("Status: Motion with delay");
  }

  if (motionLastTimeChecked) {
    bitSet(dots, 1);
    Serial.println("Status: Actual motion");
  }

  if (networkJoined) {
    bitSet(dots, 2);
    Serial.println("Status: LoRa");
  }

  if (recentSendSuccess) {
    bitSet(dots, 3);
    Serial.println("Status: Send success");
  }

  if (lastCO2measurementOK) {
    bitSet(dots, 4);
    Serial.println("Status: CO2");
  }

  if (demoMode) {
    dots = 0;
    bitSet(dots, 5);
  }
  
  leddisplay.begin(9600);
  delay(100);
  leddisplay.write(displayID);
  leddisplay.write('c');
  leddisplay.write(dots);
  leddisplay.end();
}

int takeCO2Reading() {
  byte cmd[9] = {
    0xFF,0x01,0x86,0x00,0x00,0x00,0x00,0x00,0x79}; 
  byte response[9]; 
  byte calculatedChecksum, givenChecksum;
  int ppm;

  co2sensor.begin(9600);

  // while(co2sensor.available()) {
  //   co2sensor.read();
  // }
      
  co2sensor.write(cmd, 9);
  delay(100);
  co2sensor.readBytes(response, 9);

  givenChecksum = (byte) response[8];  
  calculatedChecksum = 0;
  for (int i = 1; i<= 7; i++) {
    calculatedChecksum = calculatedChecksum + response[i];  // calculatedChecksum will overflow
  }
  calculatedChecksum = (0xFF - calculatedChecksum) + 1;
  if (calculatedChecksum == givenChecksum) {
    lastCO2measurementOK = true;
    demoMode = false;
    byte responseHigh = response[2];
    byte responseLow = response[3];
    ppm = (256 * responseHigh) + responseLow;
  } else {
    lastCO2measurementOK = false;
    demoMode = true;
    byte responseHigh = response[2];
    byte responseLow = response[3];
    ppm = 400 + random(50);
  }

  co2sensor.end();
  return ppm;
}

void setup() {
  pinMode(pirPin, INPUT);
  wakeTime = defaultWakeTime;
  
  Serial.begin(57600);
  displayMessage("");
  co2AveragePpm = takeCO2Reading();
  initLora();
}

// Main loop that is run every second
// Contains all logic, measurements and display calls
void loop() {
  cycles += 1;
  
  delay(500);                // Delay 500 milliseconds in order to have this loop run every second

  // Count down, if no motion the display is switched off
  checkMotionPresent();
  if (wakeTime > 0) {
     wakeTime = wakeTime - 1;
  }

  // Measure CO2 value and display
  co2AveragePpm = (4 * co2AveragePpm + takeCO2Reading()) / 5; // Dampen new value a bit
  Serial.println("Current average ppm " + String(co2AveragePpm));
  displayMessage(String(co2AveragePpm)); 

  delay(1500);
  
  // Send CO2 value through LoRa, but not every loop
  if (cycles > sendEvery) {
    sendMeasurement(co2AveragePpm);
    cycles = 0;
  }
  if (!networkJoined) {
    initLora();
  }
  dotStatus();
}
