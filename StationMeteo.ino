/* 
 *  Affiche l'heure, la date, la température et l'humidité sur un écran LCD
 *  Utilise un écran LCD en liaison série I2C, une horloge RTC SparkFunDS3234RTC, en liaison série SPI,
 *  et qui garde l'heure et la date même si on débranche l'arduino (pile 1225).
 *  Utilise un capteur numérique DHT22 pour la température et l'humidité.
 *  
 *  On ne peut pas utiliser une interruption pour afficher l'heure et la température/humidité indépendamment
 *  parce que la librairie "LiquidCrystal_I2C" ne supporte pas le multi-threading (i.e. plusieurs threads en parallèle)
 *  
 *  ELB - laurent.benaroya@gmail.com - 20/10/2018
 */
 
/******************************************************************************
DS3234_RTC_Demo.ino
Jim Lindblom @ SparkFun Electronics
original creation date: October 2, 2016
https://github.com/sparkfun/SparkFun_DS3234_RTC_Arduino_Library

Configures, sets, and reads from the DS3234 real-time clock (RTC).

Resources:
SPI.h - Arduino SPI Library

Development environment specifics:
Arduino 1.6.8
SparkFun RedBoard
SparkFun Real Time Clock Module (v14)
*****************************************************************************/
/*-----( Import needed libraries )-----*/
#include <SPI.h>
#include <SparkFunDS3234RTC.h>
#include "DHT.h"
#include <Wire.h>  // Comes with Arduino IDE
/* Get the LCD I2C Library here: 
 https://bitbucket.org/fmalpartida/new-liquidcrystal/downloads
 Move any other LCD libraries to another folder or delete them
 See Library "Docs" folder for possible commands etc.
 */
#include <LiquidCrystal_I2C.h>

/*-----( Declare Constants )-----*/
/*-----( Declare objects )-----*/
/* set the LCD address to 0x27 for a 16 chars 2 line display
 A FEW use address 0x3F
 Set the pins on the I2C chip used for LCD connections:
                    addr, en,rw,rs,d4,d5,d6,d7,bl,blpol
*/
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address
char stringT[16] = {'\0'};
char stringH[16];

/* 
 *  RTC
 * Comment out the line below if you want date printed before month.
 E.g. October 31, 2016: 10/31/16 vs. 31/10/16
 */
//#define PRINT_USA_DATE

float h,t;
int lastSecond = -1;

#define DHTPIN 3     // what digital pin we're connected to (DHT22)

// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

/* Connect pin 1 (on the left) of the sensor to +5V
 NOTE: If using a board with 3.3V logic like an Arduino Due connect pin 1
 to 3.3V instead of 5V!
 Connect pin 2 of the sensor to whatever your DHTPIN is
 Connect pin 4 (on the right) of the sensor to GROUND
 Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor
*/
/* Initialize DHT sensor.
 Note that older versions of this library took an optional third parameter to
 tweak the timings for faster processors.  This parameter is no longer needed
 as the current DHT reading algorithm adjusts itself to work on faster procs.
 */
DHT dht(DHTPIN, DHTTYPE);

//////////////////////////////////
// Configurable Pin Definitions //
//////////////////////////////////
#define DS13074_CS_PIN 10 // DeadOn RTC Chip-select pin
//#define INTERRUPT_PIN 2 // DeadOn RTC SQW/interrupt pin (optional)
static const char *dayIntToStr3[7] {
  "Dim",
  "Lun",
  "Mar",
  "Mer",
  "Jeu",
  "Ven",
  "Sam"  
};

void setup() 
{
  // Use the serial monitor to view time/date output
  Serial.begin(9600);

  /////////
  // DHT //
  /////////   
    
  dht.begin();

  // Call rtc.begin([cs]) to initialize the library
  // The chip-select pin should be sent as the only parameter
  rtc.begin(DS13074_CS_PIN);
  //rtc.set12Hour(); // Use rtc.set12Hour to set to 12-hour mode

  // Now set the time...
  // You can use the autoTime() function to set the RTC's clock and
  // date to the compiler's predefined time. (It'll be a few seconds
  // behind, but close!)

  /* This hardware works as described. However, the Arduino IDE sets 
   *  the time to the time that the Arduino IDE was launched, and not to the compile time
   *  when using the rtc.autoTime() library function.
   *  As long as you realize this and employ an adequate work-around
   *  (either set the time by other methods or quit/re-launch the IDE before compiling and uploading),
   *  then you should not experience any issues. The device itself works great.
   */    
  //rtc.autoTime();
  /* Or you can use the rtc.setTime(s, m, h, day, date, month, year)
  function to explicitly set the time:
  e.g. 7:32:16 | Monday October 31, 2016:
  */

  //////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////
  rtc.setTime(0, 58, 12, 5, 1, 11, 18);    // manually set time
  //////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////


  // Update time/date values, so we can set alarms
  rtc.update();


  /////////
  // LCD //
  /////////
  lcd.begin(16,2);   // initialize the lcd for 16 chars 2 lines, turn on backlight

// ------- Quick 3 blinks of backlight  -------------
  for(int i = 0; i< 3; i++)
  {
    lcd.backlight();
    delay(250);
    lcd.noBacklight();
    delay(250);
  }
  lcd.backlight(); // finish with backlight on  

//-------- Write characters on the display ------------------
// NOTE: Cursor Position: (CHAR, LINE) start at 0  
  lcd.setCursor(0,0); //Start at character 0 on line 0
  lcd.print("Temperature");
  delay(1000);
  lcd.setCursor(0,1);
  lcd.print("Begin");
  delay(1000);  
  lcd.clear();
  
  lcd.setCursor(0,1);
  lcd.print("T:");
  lcd.setCursor(8,1);
  lcd.print("H:");
  
  printTimeLCD(true); // Print the time
}


void loop() 
{
  static int8_t lastSecond = -1;
  
  // Call rtc.update() to update all rtc.seconds(), rtc.minutes(),
  // etc. return functions.
  rtc.update();

  if (rtc.second() != ((lastSecond+20) % 60)) // 20 seconds
  {
    ///// display on LCD
    
    ///// DHT
    // Read temperature as Celsius (the default)
    t = dht.readTemperature();
    dtostrf(t, 2+1+1, 1, stringT);
    
    // read humitdity (DTH 22)
    h = dht.readHumidity();
    dtostrf(h, 2+1+1, 1, stringH);


    // print temperature and humidity
    // display temperature
    lcd.setCursor(0+3,1);    
    lcd.print(stringT);
  
    // display humidity
    lcd.setCursor(8+3,1);    
    lcd.print(stringH);
    

    ////// print time
    printTimeLCD(false); // Print the new time        

    // Update lastSecond 
    rtc.update();
    lastSecond = rtc.second();
  } 
  
}

/*
void printTime()
{
  Serial.print(String(rtc.hour()) + ":"); // Print hour
  if (rtc.minute() < 10)
    Serial.print('0'); // Print leading '0' for minute
  Serial.print(String(rtc.minute()) + ":"); // Print minute
  if (rtc.second() < 10)
    Serial.print('0'); // Print leading '0' for second
  Serial.print(String(rtc.second())); // Print second

  if (rtc.is12Hour()) // If we're in 12-hour mode
  {
    // Use rtc.pm() to read the AM/PM state of the hour
    if (rtc.pm()) Serial.print(" PM"); // Returns true if PM
    else Serial.print(" AM");
  }
  
  Serial.print(" | ");

  // Few options for printing the day, pick one:
  Serial.print(rtc.dayStr()); // Print day string
  //Serial.print(rtc.dayC()); // Print day character
  //Serial.print(rtc.day()); // Print day integer (1-7, Sun-Sat)
  Serial.print(" - ");
#ifdef PRINT_USA_DATE
  Serial.print(String(rtc.month()) + "/" +   // Print month
                 String(rtc.date()) + "/");  // Print date
#else
  Serial.print(String(rtc.date()) + "/" +    // (or) print date
                 String(rtc.month()) + "/"); // Print month
#endif
  Serial.println(String(rtc.year()));        // Print year
}

*/


void printTimeLCD(bool bdate)
{
  // print time on LCD screen
  rtc.update();
  lcd.setCursor(0,0);  // Print hour
  uint8_t hh = rtc.hour();
  if (hh < 10)
    lcd.print("0");  
  lcd.print(String(hh));
  lcd.print(":");
  
  // Print minute
  uint8_t mm = rtc.minute();
  if ( mm < 10)
    lcd.print("0");   
  lcd.print(String(mm));
  //lcd.print(":");

/*
  // Print second
  if (rtc.second() < 10)
    lcd.print("0");
  lcd.print(String(rtc.second()));
*/
  if ((hh == 0) || bdate) // update date
  {
    lcd.setCursor(7,0);
    uint8_t dd = rtc.getDate();
    if (dd < 10) 
      lcd.print("0");
    lcd.print(String(dd));
    lcd.print("/");
    lcd.setCursor(10,0);
    uint8_t mo = rtc.getMonth();
    if (mo < 10)
      lcd.print("0");
    lcd.print(String(mo));

    lcd.setCursor(13,0);
    lcd.print(dayIntToStr3[rtc.getDay()-1]); // day substr
  }

}
