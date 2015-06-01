/* Temperature and moisture logger
 *  
 * A datalogger for soil moisture and temperature.
 * Date and time functions are working using a DS1307 RTC connected via I2C.
 * The data are logged on a FAT16/32 formatted SD card in a readable format.
 *
 * Sensor Pinmap
 * -------------
 *
 * Moisture sensor 1        -       ADC0
 * Moisture sensor 2        -       ADC1
 * Moisture sensor 3        -       ADC2
 * Temperature sensor 1     -       ADC3
 *
 * Sensor Powerpins (Output!)
 * ----------------
 * 
 * The temperaturesensors share a common powerpin while the moisture sensors need
 * to be powered seperatly.
 * 
 * Moisture sensor 1        -       PD7
 * Moisture sensor 2        -       PD6
 * Temperature sensors     -       PD4
 *
 * Developer: aaron@duckpond.ch
 *
 * TODO: Convert the sleepmode and wdt stuff from plain c to arduino language.
 * TODO: Optimize powersaving (partly done)
 * TODO: Adjust the lookuptable to Ub=4.3V
 */
#include<SPI.h>
#include<SD.h>
#include<RTClib.h>
#include<LowPower.h>
#include<Wire.h>
#include<avr/pgmspace.h>
#include<avr/sleep.h>
#include<avr/wdt.h>
#include<avr/power.h>

/* Defines */
#define P_MOIST1        7
#define P_MOIST2        6
#define P_TEMP          4

#define M1              A0
#define M2              A1
#define T1              A2
#define T2              A3

#define CHIPSELECT      10
#define WDT_FREQ        6
#define CSV_SEP         ", "
#define CSV_NL          "; "
#define LINUX           
#define DEBUG           

#define TEMP_UPPER_BOUNDARY 917
#define TEMP_LOWER_BOUNDARY 183

/* Makros */
#ifdef WINDOWS
    #define NEWLINE "\r\n"
#endif

#ifdef LINUX
    #define NEWLINE "\n"
#endif

/* Global variables */

// Lookuptable, where index [i] = °C, adc values from -10°C to 100°C
//TODO: Make sure this stuff goes to PROGMEM
const int adc_values[] = { 183,190,198,206,214,222,230,238,247,256,264,273,282,291,301,310,319,329,339,348,358,368,378,388,397,407,417,427,437,447,457,467,476,486,496,506,515,525,534,543,553,562,571,580,589,598,606,615,623,632,640,648,656,663,671,679,686,693,700,707,714,721,728,734,740,747,753,759,764,770,776,781,786,792,797,802,807,811,816,820,825,829,833,838,842,845,849,853,857,860,864,867,870,874,877,880,883,886,888,891,894,897,899,902,904,906,909,911,913,915,917 };
 
RTC_DS1307 RTC;

String dateStamp;
String s = "";

File logFile;

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
    s += read_moisture();
    s += CSV_SEP;
    s += read_temperature();

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
    String str = "";
    int temp[2];
    
    digitalWrite(P_TEMP, HIGH);         // Enable senor power

    temp[0] = analogRead(T1);           // Read temperature sensors
    temp[1] = analogRead(T2);
    
    digitalWrite(P_TEMP, LOW);          // Disable senor power
    
    //str += String(convert_advalue_to_temperature(temp[0]));
    //str += ", ";
    //str += String(convert_advalue_to_temperature(temp[1]));
    str += String(temp[0]);
    str += ", ";
    str += String(temp[1]);

    return str;
}

/***************************************************
 *  Name:        convert_advlaue_to_temperature()
 *
 *  Returns:     String with the moisture value
 *
 *  Parameters:  Nothing
 *
 *  Description: Read the analog value of the moisture sensor
 *
 ***************************************************/
String convert_advalue_to_temperature(int value)
{
    int i = 0;
    int out = 0;

    if(value < TEMP_LOWER_BOUNDARY || value > TEMP_UPPER_BOUNDARY){ // Boundary check
        return ("Error:" + String(value));
    }else{
        for(i = 0; value >= adc_values[i]; i++);                  // Find the corresponding temperature
        out = (i - 10);
    }

    return String(out);
}

/***************************************************
 *  Name:        read_moisture()
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
    int i = 0;

    digitalWrite(P_MOIST1, HIGH);   // Enable senor power
    digitalWrite(P_MOIST2, HIGH);   

    delay(200);                     // Wait for the Sensor to boot up
    
    moist += analogRead(M1);        // Read the moisture sensors
    moist += ", ";
    moist += analogRead(M2);
    
    digitalWrite(P_MOIST1, LOW);    // Disable senor power
    digitalWrite(P_MOIST2, LOW);   
    
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
    pinMode(CHIPSELECT, OUTPUT); // Configure the following pins as output
    pinMode(P_MOIST1,   OUTPUT);
    pinMode(P_MOIST2,   OUTPUT);
    pinMode(P_TEMP,     OUTPUT);

    Wire.begin();
    RTC.begin();        // init real time clock

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
    
    //clock_prescale_set(clock_div_256); // reduce sysclock to the minimum (due to power saving)
}

/***************************************************
 *  Name:        sleep
 *
 *  Returns:     nothing
 *
 *  Parameters:  uint16_t minutes
 *
 *  Description: Let the system sleep in pwr down mode
 *               for n minutes.
 *
 ***************************************************/
void sleep(uint16_t minutes)
{
    uint16_t n = (8*minutes);       // 1min =~ 64s

    for(int i = 0; i < n; i++) { 
        LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF); 
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
    DateTime now = RTC.now();                   // Read Time
    dateStamp = gen_date_stamped_dataline(now); // Generate date stamp
#ifdef DEBUG
    Serial.println(dateStamp);                  // Debug on serial port
#endif
    logFile.println(dateStamp);                 // Write data to the logfile
    logFile.flush();                            // Save changes on the sdcard
    sleep(30);                                  // Power down for 30min
}
