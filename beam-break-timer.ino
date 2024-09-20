/* This sketch uses a photo sensor (beam break) to time events (pendulum swings).
After the chosen number of beam breaks, it outputs results to the serial monitor and logs
pendulum timing, pressure, temperature and humidity data to micro SD card on the data logging shield

***********************************************************************************
NOTE: Although this sketch uses the real time clock to output the actual time of the data sample, IT DOES NOT USE the RTC data when timing 
the pendulum.. It uses the onboard micros() function WHICH IS KNOWN TO BE INACCURATE (on my Arduino Uno R3, it is about 90 seconds a day slow).
***********************************************************************************

HARDWARE:
  ADAfruit data logging shield
  PCF8523 RTC clock on the data logging shield
  MS8607 pressure, humidity & temperature sensor
  TT Electronics OPB901W55Z optical sensor from Digikey

LIBRARIES:
  RTClib by Adafruit v 2.1.4
  Adafruit BusIO v 1.16.1
  Adafruit MS8607 v 1.04
  Adafruit Unified Sensor v 1.1.14 
*/
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"

#include <Adafruit_MS8607.h>
#include <Adafruit_Sensor.h>

Adafruit_MS8607 ms8607;
RTC_PCF8523 RTC;  // define the Real Time Clock object
DateTime now;     //PCF8523 code
File logfile;  // the logging file

// ******** SET THESE CONSTANTS TO THE VALUES NEEDED FOR THE NEXT TEST RUN ********
const byte photoSensorPin = 2;          // beam break photo sensor
const float nominalDblSwingTime_s = 2;  // time for a DOUBLE swing; 1 second pendulum, period = double swing = 2 seconds
const int logInterval = 150;            // number of DOUBLE swings between logging results
  // ******** EVERYTHING ELSE CAN REMAIN UNTOUCHED ********
const byte chipSelect = 10;          // for the data logging shield, we use digital pin 10 for the SD cs line
unsigned long breakTime_u;           // time that the sensor interrupt is triggered (micros)
unsigned long breakCount;            // number of beam breaks (after ignoring the first one)
unsigned long swingStartTime_u = 0;  // start time (micros) of a double swing
float swingTime_s;                   // actual time for a double swing (seconds)
volatile bool beamBroken = false;
float error_s = 0;  // error of this swing

void setup() {
  Wire.begin();
  Serial.begin(9600);
  Serial.println(F("Pendulum timing and environmental data logging"));

  pinMode(photoSensorPin, INPUT_PULLUP);  // HIGH when NOT blocked

  //set up the interrupt on the photo sensor pin
  attachInterrupt(digitalPinToInterrupt(photoSensorPin), beamBreak, FALLING);  //sensor output is HIGH if the light is on (not blocked)

  // Initialize PCF8523 RTC         //this is the RTC on the data logging shield
  initialize_PCF8523_RTC();

  //Initialize MS8607 PHT sensor
  initialize_MS8607_PHT();

  // initialize SD card on logging shield
  initializeSDCard();
}

void loop() {
  if (beamBroken == true) {  // a beam interrupt has just occurred
    breakTime_u = micros();  // hold the time of the break
    beamBroken = false;      // reset the beam break flaglogfile.print("test");

    if (swingStartTime_u == 0) {       //ignore the first swing
      swingStartTime_u = breakTime_u;  // use the break time as the start time for the next swing
      breakCount = 0;

    } else {
      breakCount += 1;
      if ((breakCount % (2 * logInterval)) == 0) {  // ignore the beam break on the return swing
        readWrite_PCF8523_RTC_Time();               // get the RTC time
        calcTimingParameters();                     // calculate the results - num swings, swing time, error, mean error, st dev of the error
        writeResults();                             // write the results to the serial monitor
        readWrite_MS8607_PHTData();
        breakCount = 0;
        swingStartTime_u = breakTime_u;  // use the break time as the start time for the next swing
        Serial.println();
        logfile.flush();
      }  // end of if ((breakCount % 2) == 0)
    }    // end of if (swingStartTime_u == 0)
  }      // end of if (beamBroken == true)
}  // end of loop

void initialize_PCF8523_RTC() {
  // initialize PCF8523 RTC on the logging shield
  if (!RTC.begin()) {
    Serial.println(F("RTC failed"));
  } else {
    Serial.println(F("RTC initialized"));
  }
}

void initializeSDCard() {
  // initialize the SD card
  Serial.print(F("Initializing SD card..."));
  // make sure that the default chip select pin is set to output, even if you don't use it:
  pinMode(10, OUTPUT);

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    error("Card failed, or not present");
  }
  Serial.println(F("card initialized."));

  // create a new file
  char filename[] = "LOGGR000.CSV";
  for (uint8_t i = 0; i < 100; i++) {
    filename[5] = i / 100 + '0';
    filename[6] = i / 10 + '0';
    filename[7] = i % 10 + '0';
    if (!SD.exists(filename)) {
      // only open a new file if it doesn't exist
      logfile = SD.open(filename, FILE_WRITE);
      break;  // leave the loop!
    }
  }

  if (!logfile) {
    error("couldnt create file");
  }

  Serial.print(F("Logging to: "));
  Serial.println(filename);
  logfile.println("Year,Month,Day,,Hour,Minute,Second,,Swing time,Error,,Temp,Pressure,Humidity");
  logfile.flush();
}  // end of initialize SD card

void error(char *str) {
  Serial.print(F("error: "));
  Serial.println(str);
  while (1);
}

void beamBreak() {
  beamBroken = true;
}

void calcTimingParameters() {
  swingTime_s = float(breakTime_u - swingStartTime_u) / 1000000;
  error_s = (swingTime_s - nominalDblSwingTime_s * logInterval);
}

void writeResults() {
  Serial.print(F("  swing= "));
  Serial.print(swingTime_s, 6);
  Serial.print(F("  err= "));
  Serial.print(error_s / (nominalDblSwingTime_s * logInterval) * (24.0 * 60.0 * 60.0), 1);
  logfile.print(swingTime_s, 6);
  logfile.print(",");
  logfile.print(error_s / (nominalDblSwingTime_s * logInterval) * (24.0 * 60.0 * 60.0), 1);
  logfile.print(",,");
}

void readWrite_PCF8523_RTC_Time() {
  // fetch the time
  now = RTC.now();

  logfile.print(now.year(), DEC);
  logfile.print(",");
  logfile.print(now.month(), DEC);
  logfile.print(",");
  logfile.print(now.day(), DEC);
  logfile.print(",,");
  logfile.print(now.hour(), DEC);
  logfile.print(",");
  logfile.print(now.minute(), DEC);
  logfile.print(",");
  logfile.print(now.second(), DEC);
  logfile.print(",,");
  logfile.flush();

  Serial.print(now.year(), DEC);
  Serial.print("/");
  Serial.print(now.month(), DEC);
  Serial.print("/");
  Serial.print(now.day(), DEC);
  Serial.print(" ");
  Serial.print(now.hour(), DEC);
  Serial.print(":");
  Serial.print(now.minute(), DEC);
  Serial.print(":");
  Serial.print(now.second(), DEC);
}

void initialize_MS8607_PHT() {
  // Try to initialize!
  if (!ms8607.begin()) {
    Serial.println(F("Failed to find MS8607 chip"));
    while (1) { delay(10); }
  }
  Serial.println(F("Initializing the MS8607 PHT module"));

  ms8607.setHumidityResolution(MS8607_HUMIDITY_RESOLUTION_OSR_8b);
}

void readWrite_MS8607_PHTData() {
  sensors_event_t temp, pressure, humidity;
  ms8607.getEvent(&pressure, &temp, &humidity);
  //#if ECHO_TO_SERIAL
  Serial.print(F("  T= "));
  Serial.print(temp.temperature * 9 / 5 + 32, 1);
  Serial.print(F("F"));
  Serial.print(F("  P= "));
  Serial.print(pressure.pressure, 1);
  Serial.print(F("hPa"));
  Serial.print(F("  H= "));
  Serial.print(humidity.relative_humidity, 1);
  Serial.print(F("%rH"));

  logfile.print(temp.temperature * 9 / 5 + 32, 1);
  logfile.print(",");
  logfile.print(pressure.pressure, 1);
  logfile.print(",");
  logfile.print(humidity.relative_humidity, 1);
  logfile.println();
  logfile.flush();
}
