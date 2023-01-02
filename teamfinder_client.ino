
/***

TECHIN514 Team Selection Exercise 2022

Adjust the Wifi and target address parameters as instructed during the in-class exercise.

Using the Arduino toolchain, load this program onto an ESP32 board and execute it.

Make sure to have the serial port output running at 115200 baud.  

You should see your ESP32 chip ID and your team number in the serial output.

If you don't get a team number, please try to run it a few more times, but otherwise
you can email the instructional team the chip ID from your output, along with a copy 
of the "configuration section" of this program, and we will find your team number for you.

Please submit the "configuration section" and the serial output into canvas as your 
team selection assignment assignment. 

p.s. not the cleanest or most efficient implementation, 
just something quick that should work for class.  prototyping style!

pull requests welcome!

Provided under MIT License. 

***/


#include <Arduino.h>

#include <WiFi.h>
#include <WiFiMulti.h>  

#include <HTTPClient.h>

// "configuration section"
const char* STUDENT_NAME =  "your name here";
const char* WIFI_SSID =     "ssid";
const char* WIFI_PASSWORD = "password";
const char* TARGET_SERVER = "x.y.z.a";

uint32_t chipID = 0;
WiFiMulti wifiMulti;

void setup() {

    Serial.begin(115200);
    Serial.printf("TECHIN514 Team Selection Exercise\n\n");


    // from the ESP32 GetChipID example.  32bit ID should be enough for this
    for(int i=0; i<17; i=i+8) {
	  chipID |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
	  }

    Serial.printf("Chip ID       : 0x%x\n", chipID);
    Serial.printf("Student name  : %s\n", STUDENT_NAME);
    Serial.printf("wifiSSID      : %s\n", WIFI_SSID);
    Serial.printf("target server : %s\n", TARGET_SERVER);
    Serial.println();

    Serial.printf("Waiting for Wifi connection...");
    wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

}


void loop() {

    if ((wifiMulti.run() == WL_CONNECTED)) {

        Serial.printf("CONNECTED!\n");

        HTTPClient http;

        // TODO : chipID as hex?

        char idString[10];
        sprintf(idString, "0x%x", chipID);
        String targetURL = "http://";
        targetURL += TARGET_SERVER;
        targetURL += "/?id=";
        targetURL += idString;
        targetURL += "&name=";
        targetURL += STUDENT_NAME;

        targetURL.replace(" ", "-");        
        
        Serial.printf("[HTTP] attempting connection to :%s\n ", targetURL.c_str());
        http.begin(targetURL);
        int httpCode = http.GET();

        if (httpCode == HTTP_CODE_OK) {
            Serial.print("Welcome to team ");
            Serial.println(http.getString());
        }
        else {
            Serial.printf("[HTTP] error on GET : %s\n", http.errorToString(httpCode).c_str());
        }

        http.end();

        Serial.printf("\nSTOPPING HERE!\n");
        while (true) {};

    }

    // loop until wifi is connected
    delay(2000);
    Serial.printf(".");

}