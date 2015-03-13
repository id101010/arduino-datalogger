/* Temperature and moisture logger
 *  
 * A datalogger for soil moisture and temperature.
 * Date and time functions are working using a DS1307 RTC connected via I2C.
 * The data are logged on a FAT16/32 formatted SD card in a readable format.
 *
 * Moisture sensor        -      ADC0
 * Temperature sensor     -      ADC1
 *
 * Developer: aaron@duckpond.ch
 *
 * TODO: Convert the sleepmode and wdt stuff from plain c to arduino language.
 */

#include<Wire.h>
#include<SPI.h>
#include<SD.h>
#include<RTClib.h>
#include<avr/pgmspace.h>
#include<avr/sleep.h>
#include<avr/wdt.h>

// Defines
#define TEMPERATURE     A0
#define MOISTURE        A1
#define CHIPSELECT      10
#define SLEEPTIME       9
#define CSV_SEP         ","
#define CSV_NL          ";"
#define LINUX

// Makros
#ifndef cbi
    #define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif

#ifndef sbi
    #define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

#ifdef WINDOWS
    #define NEWLINE "\r\n"
#endif

#ifdef LINUX
    #define NEWLINE "\n"
#endif

// Lookuptable, where index [i] = °C, adc values from -10°C to 100°C
//TODO: Make sure this stuff goes to PROGMEM
const int adc_values[] = {192,199,207,215,224,232,241,250,258,268,277,286,296,305,315,325,335,344,355,365,375,385,395,406,416,426,437,447,458,468,478,489,499,509,519,529,539,549,559,569,579,588,598,607,617,626,635,644,653,661,670,678,686,695,703,711,718,726,733,741,748,755,762,769,775,782,788,794,800,806,812,818,823,829,834,839,845,849,854,859,864,868,873,877,881,885,889,893,897,901,904,908,911,915,918,921,924,927,930,933,936,939,941,944,947,949,951,954,956,958,961};

// Global variables
RTC_DS1307 RTC;
String dateStamp = "";
File logFile;
volatile boolean flag_wdt = true;

/***************************************************
 *  Name:        ISR(WDT_vect)
 *
 *  Returns:     Nothing
 *
 *  Parameters:  Nothing
 *
 *  Description: Interrupt service routine for the 
 *               Watchdog timer.
 *
 ***************************************************/
ISR(WDT_vect) {
  flag_wdt = true;  // set global flag
}

/***************************************************
 *  Name:        setup_watchdog()
 *
 *  Returns:     Nothing
 *
 *  Parameters:  int timeout
 *
 *  Description: Sets up the watchtog timer to time out 
 *               in one of the following intervals:
 *               0=16ms, 1=32ms,2=64ms,3=128ms,4=250ms,
 *               5=500ms, 6=1s, 7=2s, 8=4s, 9=8s
 ***************************************************/
void setup_watchdog(int timeout) 
{
    byte bb;
    int ww;

    // Set max value
    if (timeout > 9 ){
        timeout = 9;
    }

    bb = timeout & 7;

    if (timeout > 7){
         bb |= (1<<5);
    }
    
    bb |= (1<<WDCE);
    ww = bb;

    MCUSR &= ~(1<<WDRF);

    // start timed sequence
    WDTCSR |= (1<<WDCE) | (1<<WDE);

    // set new watchdog timeout value
    WDTCSR = bb;
    WDTCSR |= _BV(WDIE);
}

/***************************************************
 *  Name:        system_sleep()
 *
 *  Returns:     nothing
 *
 *  Parameters:  nothing
 *
 *  Description: Set the system to sleep state until the 
 *               watchdog timer times out.
 *
 ***************************************************/
void system_sleep(void) 
{
  cbi(ADCSRA,ADEN);                    // Turn adc OFF
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // Set the sleep mode
  sleep_enable();                      // enable sleep mode
  sleep_mode();                        // System sleeps here

  sleep_disable();                     // System continues execution only when the wdt timed out 
  sbi(ADCSRA,ADEN);                    // switch adc back ON
}


/***************************************************
 *  Name:        gen_date_stamped_dataline()
 *
 *  Returns:     String with time & date & temp & moist
 *
 *  Parameters:  DateTime structure
 *
 *  Description: Generate a string containing the time and date
 *               aswell as the temperature and moisture set up
 *               in a csv line.
 *
 ***************************************************/
String gen_date_stamped_dataline(DateTime now)
{
    String s = "";

    // Append time to a string
    s += now.year();
    s += "-";
    s += now.month();
    s += "-";
    s += now.day();
    s += CSV_SEP;        
    s += now.hour();
    s += ":";
    s += now.minute();
    s += ":";
    s += now.second();
    s += CSV_SEP;

    // Now append the measured data
    s += read_temperature();
    s += CSV_SEP;
    s += read_moisture();

    // New csv line
    s += CSV_NL;

    return s;
}

/***************************************************
 *  Name:        read_temperaure()
 *
 *  Returns:     String with the temperature value
 *
 *  Parameters:  Nothing
 *
 *  Description: Read the analog value of the temperature 
 *               sensor and convert it using the adc lookuptable.
 *
 ***************************************************/
String read_temperature(void)
{
    int temp = 0, atemp = 0, i = 0;
    
    atemp = analogRead(TEMPERATURE);
    
    // Boundary check for the lookuptable
    if(atemp < 192 || atemp > 961){
        Serial.println("[ERROR]: Abnormal temperature readings.");
        return "TempError";
    } else {
        // Find the corresponding temperature to the adc value
        for(i = 0; atemp >= adc_values[i]; i++){
        }
    }
    
    temp = i - 10; // Add 10 since the lookuptable starts with -10°C

    return String(temp);
}

/***************************************************
 *  Name:        read_tmoisture()
 *
 *  Returns:     String with the moisture value
 *
 *  Parameters:  Nothing
 *
 *  Description: Read the analog value of the moisture sensor
 *   
 *  TODO: Test the sensor to find the boundarys.
 *
 ***************************************************/
String read_moisture(void)
{
    String moist = "";
    moist += analogRead(MOISTURE);
    return moist;
}

/***************************************************
 *  Name:        setup()
 *
 *  Returns:     Nothing
 *
 *  Parameters:  Nothing
 *
 *  Description: Initialize the device on start
 *
 ***************************************************/
void setup(void) 
{
    Serial.begin(9600); // init serial interface
    Wire.begin();       // init i2c bus 
    RTC.begin();        // init real time clock

    pinMode(CHIPSELECT, OUTPUT);    // Make sure the Chipselect is configured as output

    // Uncoment this to automatically set the compile time! Make sure to comment it out again!
    //RTC.adjust(DateTime(__DATE__, __TIME__)); 
   
    // Check if SD card is present and readable
    if(!SD.begin(CHIPSELECT)){
        Serial.println("[ERROR]: SDcard failed or not present!");
        while(1);   // Error case, do nothing
    }

    // Check if rtc is running
    if (!RTC.isrunning()){
        Serial.println("[ERROR]: RTC is NOT running!");
    }

    // Open the logfile on the SDcard
    logFile = SD.open("logfile.csv", FILE_WRITE);
    
    // Check if logfile is readable
    if(!logFile){
        Serial.println("[ERROR]: Logfile corrupted!");
        while(1);   // Error case, do nothing
    }

    // Set up the sleep mode
    cbi(SMCR,SE);               // sleep enable, power down mode
    cbi(SMCR,SM0);              // power down mode
    sbi(SMCR,SM1);              // power down mode
    cbi(SMCR,SM2);              // power down mode
    setup_watchdog(SLEEPTIME);
     
}

/***************************************************
 *  Name:        loop()
 *
 *  Returns:     Nothing
 *
 *  Parameters:  Nothing
 *
 *  Description: Main loop
 *
 ***************************************************/
void loop(void) 
{
    // Only do something when the wdt timeout occurs
    if(flag_wdt){
        flag_wdt = false;                   // Reset the global flag
        DateTime now = RTC.now();           // Read Time
        
        dateStamp = gen_date_stamped_dataline(now);    // Generate date stamp

        //Serial.println(dateStamp);          // Debug on serial port
        
        logFile.println(dateStamp);         // Write data to the logfile
        logFile.flush();                    // Save changes on the sdcard
    }else{
        system_sleep();
    }
}
