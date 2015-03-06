#include <Arduino.h>
#include<Wire.h>
#include<SPI.h>
#include<SD.h>
#include"RTClib.h"
String gen_date_stamp(DateTime now);
String read_temperature(void);
String read_moisture(void);
void setup(void);
void loop ();
#line 1 "src/sketch.ino"
/* Temperature and humidity Logger
 *  
 * Date and time functions using a DS1307 RTC connected via I2C and Wire lib
 *
 * Moisture sensor on ADC0
 * Temperature sensor on ADC1
 */

//#include<Wire.h>
//#include<SPI.h>
//#include<SD.h>
//#include"RTClib.h"
 
// Defines
#define TEMPERATURE     A0
#define MOISTURE        A1
#define CHIPSELECT      10

// Globals
RTC_DS1307 RTC;
String dateStamp = "";
File logFile;

String gen_date_stamp(DateTime now)
{
    String s = "[";

    s += now.year();
    s += "-";
    s += now.month();
    s += "-";
    s += now.day();
    s += "/";
    s += now.hour();
    s += ":";
    s += now.minute();
    s += ":";
    s += now.second();

    s += "],";

    return s;
}

String read_temperature(void)
{

    String temp = "";
    temp += analogRead(TEMPERATURE);
    return temp;
}

String read_moisture(void)
{
    String moist = ",";
    moist += analogRead(MOISTURE);
    return moist;
}

void setup(void) 
{
    // Init software
    Serial.begin(9600);
    Wire.begin();
    RTC.begin();

    // Init hardware
    pinMode(SS, OUTPUT);

    // Uncoment this to automatically set the compile time!
    //RTC.adjust(DateTime(__DATE__, __TIME__)); 
    
    // Check if SD card is present and readable
    if(!SD.begin(CHIPSELECT)){
        Serial.println("[ERROR]: SDcard failed or not present!");
        while(1);   // Do nothing
    }

    // Check if rtc is running
    if (!RTC.isrunning()){
        Serial.println("[ERROR]: RTC is NOT running!");
    }

    // Open the logfile on the SDcard
    logFile = SD.open("logfile.txt", FILE_WRITE);

    // Check if logfile is readable
    if(!logFile){
        Serial.println("[ERROR]: Logfile corrupted!");
        while(1);   // Do nothing
    }
     
}
 
void loop () 
{
    DateTime now = RTC.now();           // Read Time
    dateStamp = gen_date_stamp(now);    // Generate date stamp
    dateStamp += read_temperature();
    dateStamp += read_moisture();

    Serial.println(dateStamp);          // Debug on serial port
    logFile.println(dateStamp);         // Write data to the logfile
    logFile.flush();                    // Save changes on the sdcard

    delay(1000);
}
