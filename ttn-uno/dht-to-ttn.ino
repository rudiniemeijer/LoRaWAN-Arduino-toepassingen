// Use The Things Network library and a DHT sensor library
#include <TheThingsNetwork.h>
#include <DHT.h>

// AppEUI and AppKey are stored in the LoRa module, so OTAA join
// Use European frequency plan
#define freqPlan TTN_FP_EU868

// Define the DHT sensor to use, either DHT11 or DHT22
#define DHTPIN 2
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);
TheThingsNetwork ttn(loraSerial, debugSerial, freqPlan);

void setup() {
  ttn.join(); // Join OTAA with stored AppEUI and AppKey
  dht.begin(); // Initialise the DHT22 sensor
}

void loop() {
  // Using a DHT22 temperature/humidity sensor. Read every 30 seconds and
  // transmit the data to The Things Network
  // Read sensor values and multiply by 100 to effectively have 2 decimals
  uint16_t humidity = dht.readHumidity(false) * 100;
  uint16_t temperature = dht.readTemperature(false) * 100;

  // Compress data in 4 bytes of 8 bits
  byte payload[4];
  payload[0] = highByte(temperature);
  payload[1] = lowByte(temperature);
  payload[2] = highByte(humidity);
  payload[3] = lowByte(humidity);
  ttn.sendBytes(payload, sizeof(payload));
  
  delay(30000); // Wait 30 seconds and repeat all over again
}
