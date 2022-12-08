#include <dummy.h>

#include <EEPROM.h>

#include <M5Station.h>
#include <I2C_AXP192.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <SensirionI2CScd4x.h>

SensirionI2CScd4x Co2Sensor;

#define uS_to_s_Factor 1000000
#define TIME_TO_SLEEP 1800



// Network details
const char* ssid = "ssid";
const char* wifi_password = "password";

// MQTT broker 
const char* mqtt_server = "MQTT Broker";
const char* mqtt_topic_temp = "M5/Temperature";
const char* mqtt_topic_co2 = "M5/co2";
const char* mqtt_topic_humidity = "M5/Humidity";
const char* mqtt_username = "Username";
const char* mqtt_password = "Password";
const char* mqtt_client_id = "KRL";


WiFiClient wifi_client;
PubSubClient client(mqtt_server, 1883, wifi_client);

void setup()
{
    M5.begin(115200);
    delay(1000);
    M5.Lcd.setTextFont(4);
    M5.Lcd.drawString("Unit CO2", 110, 0);
    uint16_t error;
    char errorMessage[256];
    Co2Sensor.begin(Wire);
    // stop potentially previously started measurement
    error = Co2Sensor.stopPeriodicMeasurement();
    if (error) {
        Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    }
    
    // Start Measurement
    error = Co2Sensor.startPeriodicMeasurement();
    if (error) {
        Serial.print("Error trying to execute startPeriodicMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    }
    Serial.println("Waiting for first measurement... (5 sec)");

  connect_to_wifi();  
  connect_to_mqtt();
  delay(3000);
  get_data_and_publish(); 
}
void loop()
{  
  

}
void sleepy(){
   
  Serial.println("Going to sleep...");
  delay(1000);
  esp_sleep_enable_timer_wakeup(uS_to_s_Factor * TIME_TO_SLEEP);
  delay(2500);
  esp_deep_sleep_start();
}
void disconnect_wifi(){
  
      WiFi.disconnect();
  
}
void get_data_and_publish()
{
    uint16_t error;
    char errorMessage[256];
    delay(100);
    // Read Measurement
    uint16_t co2_measured      = 0;
    float temperature_measured = 0.0f;
    float humidity_measured    = 0.0f;
    bool isDataReady  = false;
    Serial.println("Start measuring...."); 
    error = Co2Sensor.getDataReadyFlag(isDataReady);
    if (error) {
        M5.Lcd.print("Error trying to execute readMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
        return;
    }
    if (!isDataReady) {
        Serial.println("Data isnt ready!");
        get_data_and_publish();
        return;
    }
    error = Co2Sensor.readMeasurement(co2_measured, temperature_measured, humidity_measured);
    if (error) {
        M5.Lcd.print("Error trying to execute readMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    } else if (co2_measured == 0) {
        Serial.println("Invalid sample detected, skipping.");
    } else { 
        Serial.println("Show Measurement");
        M5.Lcd.setCursor(0, 35);
        M5.Lcd.printf("Co2:%d\n", co2_measured);
        M5.Lcd.printf("Temp.:%f\n", temperature_measured);
        M5.Lcd.printf("Humidity:%f\n", humidity_measured);
        String valuesToSend_temp = String(temperature_measured);
        String valuesToSend_co2 = String(co2_measured);
        String valuesToSend_humidity = String(humidity_measured);
        client.publish(mqtt_topic_temp, valuesToSend_temp.c_str());
        client.publish(mqtt_topic_co2, valuesToSend_co2.c_str());
        client.publish(mqtt_topic_humidity, valuesToSend_humidity.c_str());
        Serial.println("Messages published...");
        client.loop();
        delay(2000);
        sleepy();
    }   
    Serial.println("Excited without measuring..");

}

void connect_to_wifi()
{
    WiFi.persistent(false);
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    // Connect to the network
    WiFi.begin(ssid, wifi_password);
    Serial.println("Connecting to WiFi");
    int retry_count = 0;
    while (WiFi.status() != WL_CONNECTED && retry_count < 15)
    {
        delay(1000);
        retry_count++;
        Serial.print(".");
    }
    if (WiFi.status() != WL_CONNECTED)
    {
        ESP.restart();
    }
    Serial.println("");
    Serial.println(WiFi.localIP());
    
}
void connect_to_mqtt()
{
    int retry_count = 0;
    while (!client.connected() && retry_count < 15)
    {
        // Connect to MQTT broker
        if (client.connect(mqtt_client_id, mqtt_username, mqtt_password))
        {
            Serial.println("Connected to MQTT Broker!");
            
        }
        else
        {
            Serial.println("Connection to MQTT Broker failed...");
            Serial.println(client.state());
            retry_count++;
            delay(1000);
        }
    }
    if (!client.connected())
    {
        ESP.restart();
    }
}

