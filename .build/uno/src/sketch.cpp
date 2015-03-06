#include <Arduino.h>
#include<Wire.h>
#include<SPI.h>
#include<SD.h>
#include"RTClib.h"
#include <avr/pgmspace.h>
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
//#include <avr/pgmspace.h>
 
// Defines
#define TEMPERATURE     A0
#define MOISTURE        A1
#define CHIPSELECT      10

// Lookuptable
const int adc_values[] = {289,299,309,319,329,340,351,362,373,385,397,409,421,433,446,459,472,485,499,512,526,539,553,567,581,594,608,622,636,649,663,676,690,703,716,728,741,753,765,777,789,800,811,821,832,842,851,861,870,878,887,895,902,910,917,923,930,936,942,947,952,957,962,966,970,974,978,981,984,987,990,993,995,997,1000,1002,1003,1005,1007,1008,1010,1011,1012,1013,1014,1015,1016,1017,1017,1018,1018,1019,1019,1020,1020,1021,1021,1021,1022,1022,1022,1022,1022,1023,1023,1023,1023,1023,1023,1023,1023,1024};

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
    int temp = 0, a0 = 0, i = 0, j = 0;
    
    a0 = analogRead(TEMPERATURE);
    
    for(i = 0; adc_values[i] <= a0; i++){
    }
    
    temp = i;
        
    return String(temp);
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
    //logFile.println(dateStamp);         // Write data to the logfile
    //logFile.flush();                    // Save changes on the sdcard

    delay(1000);
}
