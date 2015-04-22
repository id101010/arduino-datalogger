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
 * TODO: Optimize powersaving (partly done)
 * TODO: Adjust the lookuptable to Ub=4.3V
 */

#include<Wire.h>
#include<SPI.h>
#include<SD.h>
#include<RTClib.h>
#include<avr/pgmspace.h>
#include<avr/sleep.h>
#include<avr/wdt.h>

// Defines
#define SENSOR_POWERPIN 7
#define TEMPERATURE     A0
#define MOISTURE        A1
#define CHIPSELECT      10
#define WDT_FREQ        6
#define CSV_SEP         ","
#define CSV_NL          ";"
#define LINUX           
//#define DEBUG           

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
const int adc_values[] = { 183,190,198,206,214,222,230,238,247,256,264,273,282,291,301,310,319,329,339,348,358,368,378,388,397,407,417,427,437,447,457,467,476,486,496,506,515,525,534,543,553,562,571,580,589,598,606,615,623,632,640,648,656,663,671,679,686,693,700,707,714,721,728,734,740,747,753,759,764,770,776,781,786,792,797,802,807,811,816,820,825,829,833,838,842,845,849,853,857,860,864,867,870,874,877,880,883,886,888,891,894,897,899,902,904,906,909,911,913,915,917 };
 
// Global variables
RTC_DS1307 RTC;

String dateStamp;
String s = "";

File logFile;
volatile uint16_t flag_wdt;


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
    flag_wdt++;    // increment global flag
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
void system_sleep() 
{
    cbi(ADCSRA,ADEN);                    // Turn adc OFF
    digitalWrite(SENSOR_POWERPIN, LOW);  // Disable sensor power
    
    set_sleep_mode(SLEEP_MODE_PWR_DOWN); // Set the sleep mode
    sleep_enable();                      // enable sleep mode
    sleep_mode();                        // System sleeps here

    sleep_disable();                     // System continues execution
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
    s = "";
    digitalWrite(SENSOR_POWERPIN, HIGH); // Enable senor power

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
    if(atemp < 183 || atemp > 917){
#ifdef DEBUG
        Serial.println("[ERROR]: Abnormal temperature readings.");
#endif
        return String(atemp);
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
#ifdef DEBUG
    Serial.begin(9600); // init serial interface
#endif
    Wire.begin();       // init i2c bus 
    RTC.begin();        // init real time clock

    pinMode(CHIPSELECT, OUTPUT);        // Make sure the Chipselect is configured as output
    pinMode(SENSOR_POWERPIN, OUTPUT);   // Enable the sensor powersource

    // Uncoment this to automatically set the compile time! Make sure to comment it out again!
    //RTC.adjust(DateTime(__DATE__, __TIME__)); 
   
    // Check if SD card is present and readable
    if(!SD.begin(CHIPSELECT)){
#ifdef DEBUG
        Serial.println("[ERROR]: SDcard failed or not present!");
#endif
        while(1);   // Error case, do nothing
    }

    // Check if rtc is running
    if (!RTC.isrunning()){
#ifdef DEBUG
        Serial.println("[ERROR]: RTC is NOT running!");
#endif
    }

    // Open the logfile on the SDcard
    logFile = SD.open("logfile.csv", FILE_WRITE);
    
    // Check if logfile is readable
    if(!logFile){
#ifdef DEBUG
        Serial.println("[ERROR]: Logfile corrupted!");
#endif
        while(1);   // Error case, do nothing
    }

    // Set up the sleep mode
    cbi(SMCR,SE);                   // sleep enable, power down mode
    cbi(SMCR,SM0);                  // power down mode
    sbi(SMCR,SM1);                  // power down mode
    cbi(SMCR,SM2);                  // power down mode
    setup_watchdog(WDT_FREQ);       // WDT counts with this frequency
     
}

/***************************************************
 *  Name:        calc_sleeptim
 *
 *  Returns:     timeout
 *
 *  Parameters:  seconds, wdt_frequency
 *
 *  Description: Calculates sleeptime for the wdt_freq
 *
 ***************************************************/
uint16_t calc_sleeptime(uint16_t seconds, uint8_t wdt_frequency)
{
    switch(wdt_frequency){
        case 9:
            return (seconds / 8);
            break;
        case 8:
            return (seconds / 4);
            break;
        case 7:
            return (seconds / 2);
            break;
        case 6:
            return (seconds / 1);
            break;
        case 5:
            return (2 * seconds);
            break;
        case 4:
            return (4 * seconds);
            break;
        case 3:
            return (8 * seconds);
            break;
        case 2:
            return (16 * seconds);
            break;
        case 1:
            return (32 * seconds);
            break;
        case 0:
            return (64 * seconds);
            break;
        default:
            return seconds;
            break;
    }
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
    if(flag_wdt >= calc_sleeptime(1800, WDT_FREQ)){    // If the sleeptime is reached
        flag_wdt = 0;                               // Set counter to 0
        DateTime now = RTC.now();                   // Read Time
        dateStamp = gen_date_stamped_dataline(now); // Generate date stamp
#ifdef DEBUG
        Serial.println(dateStamp);                  // Debug on serial port
#endif
        logFile.println(dateStamp);                 // Write data to the logfile
        logFile.flush();                            // Save changes on the sdcard
    }else{
        system_sleep();                             // Good night
        //delay(1000);
    }
}
