#include <badger.h>
/* 5 * 60 * 1000UL defines interval when temperature is sent 
 * 30 * 1000UL defines offset from beginning
 *  PRINT_ENABLE - prints to serial port " NEXT TX IN ..."
 *  PRINT_DISABLE - disables " NEXT TX IN ..." message
 */

badger_scheduler temp_sched(5 * 60 * 1000UL, 30 * 1000UL, PRINT_ENABLE); // Once every 5 minutes send, wake up every 30 seconds
// badger_scheduler status_sched(1440 * 60 * 1000UL, 150 * 1000UL, PRINT_ENABLE); // Once every day
bool LoRa_add_sensitivity(uint8_t sensitivity);

uint32_t s_badger_sleep_period_ms = 32 * 1000UL;
uint32_t current_battery_voltage;


// enter your own key
const uint8_t devEUI[8] = {0x70, 0xB3, 0xD5, 0xB0, 0x20, 0x03, 0x5C, 0xE9};
const uint8_t appKey[16] = {0x1F, 0x8D, 0x75, 0xF8, 0x57, 0x6E, 0x9E, 0x80, 0x76, 0xDA, 0x04, 0xD4, 0xD6, 0x7E, 0xC7, 0xBA};
const uint8_t appEUI[8] = {0x70, 0xB3, 0xD5, 0x7E, 0xF0, 0x00, 0x36, 0xB5};

void setup() 
{
    Serial.begin(9600);
    badger_init();
    Serial.println("Badgerboard firmware started.");
    badger_print_EUI(devEUI);
    LoRa_init(devEUI, appEUI, appKey);  
//    LoRa_add_sensitivity(6); // 1 = short range, 6 = very long range
    current_battery_voltage = badger_read_vcc_mv();
    Serial.print("VCC = ");
    Serial.print(current_battery_voltage);
    Serial.println("V.");

}

void loop()
{
    if (temp_sched.run_now())
    {
        // this function is called when the time defined at the "temp_sched" variable is reached
        Serial.print("Sending temperature");
        
        badger_pulse_led(1); // Pulse Bager board with period time in ms
        //badger_blink_error(badger_temp_sensor_send()); // JSON formatted temperatur and humidty (11 bytes)
        badger_blink_error(badger_temp_send()); // 4 bytes float
        
    } else {
      Serial.print("Nothing to do. ");
    }
    if (Lora_requires_reset())
    {
        Serial.println("Restarting due to LoRa module needing reset");
        badger_restart();        
    }
    current_battery_voltage = badger_read_vcc_mv();
    Serial.print("VCC = ");
    Serial.print(current_battery_voltage);
    Serial.println("V. Sending Badgerboard to sleep.");
    badger_sleep_now(s_badger_sleep_period_ms);
    Serial.print("Woke up. ");
}
