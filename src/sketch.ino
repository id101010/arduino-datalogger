/* Temperature and humidity Logger
 *  
 * Date and time functions using a DS1307 RTC connected via I2C and Wire lib
 *
 * Moisture sensor on ADC0
 * Temperature sensor on ADC1
 */

#include <Wire.h>
#include "RTClib.h"
 
// Defines
#define TEMPERATURE     A0
#define MOISTURE        A1

// Globals
RTC_DS1307 RTC;
String dateStamp = "";

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
    // Init stuff
    Serial.begin(9600);
    Wire.begin();
    RTC.begin();
    
    // Check if rtc is running
    if (!RTC.isrunning()){
        Serial.println("RTC is NOT running!");
    }
     
    // Uncoment this to automatically set the compile time!
    //RTC.adjust(DateTime(__DATE__, __TIME__)); 
}
 
void loop () 
{
    DateTime now = RTC.now();           // Read Time
    dateStamp = gen_date_stamp(now);    // Generate date stamp
    dateStamp += read_temperature();
    dateStamp += read_moisture();

    Serial.println(dateStamp);

    // TODO Save to SDcard
    
    delay(3000);
}
