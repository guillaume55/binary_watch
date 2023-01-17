/*******************************
 * 
 * TODO :
 * ENREGISTRER DANS LA RTC !!!!!!
 * Attention à l'overflow de millis() ???
 * 
 * 
 * Binary watch version 0.1
 * 
 * How to read it 
 * Each digit is separated. The two first digits are for the hours, the two last for minutes
 * the Bottom digit adds 1, the second 2, the third 4, and the top one 8
 * For instance if # are on and _ are off
 *      _ _ _ _
 *      _ _ # #
 *      _ # _ #
 *      # # _ #
 *      1 3:4 7
 *      
 * Buttons
 * "Main menu"
 * Top button, Unlock and show time
 * Left button : set time 
 * Right button : change mode (time, date, stopwatch, remaining battery)
 * 
 * "Set time and date" mode : 
 * The leds are blinking, first set time (hours, minutes), then date(day, month)
 * Tap top button to validate, right to ++, left to --
 * 
 * 
 *******************************/

#include <RTCCounter.h>
#include <time.h>
#include "Adafruit_FreeTouch.h"

//use precompiled atmel qtouch

#define BUTTON_RIGHT 42//PA03
#define BUTTON_LEFT A5//PB02
#define BUTTON_TOP A1//PB08

#define QTOUCH_THRESHOLD 600

#define ON_DURATION 10000 //sec during which the watch is showing time

Adafruit_FreeTouch qt_top = Adafruit_FreeTouch(BUTTON_TOP, OVERSAMPLE_4, RESISTOR_50K, FREQ_MODE_NONE);
Adafruit_FreeTouch qt_left = Adafruit_FreeTouch(BUTTON_LEFT, OVERSAMPLE_4, RESISTOR_50K, FREQ_MODE_NONE);
Adafruit_FreeTouch qt_right = Adafruit_FreeTouch(BUTTON_RIGHT, OVERSAMPLE_4, RESISTOR_50K, FREQ_MODE_NONE);

#define BLINK_ALL -1

unsigned long pmBlink = 0;
bool blink = false; 
bool blinkTog = true;

//as arduno zero on prog port
const int ledPin [4][4] = { {0,0,17,16}, //1st digit of hours, The two first leds are not connected to the uC
                          {9,8,18,14}, //2nd digit of hours
                          {24,23,7,6}, //1st digit of min 
                          {22,38,20,21}  //2nd digit of min 
                        };

int ledState [4][4] = { {0,0,0,0}, //1st digit of hours
                        {0,0,0,0}, //2nd digit of hours
                        {0,0,0,0}, //1st digit of min
                        {0,0,0,1}  //2nd digit of min, shows one led to say that everything is working
                      };                     

const int batteryProbe = 25; //PB03;

uint8_t menuIndex = 0; //(dispaly time, date, stopwatch, remaining battery)

void setup() {
  //set leds as ouputs
  for(int i=0; i<4; i++)
  {
    for(int j=0; j<4; j++)
    {
      pinMode(ledPin[i][j], OUTPUT);
      digitalWrite(ledPin[i][j], ledState[i][j]);
    }
  }
  //while(1)
    ledsTest(1);

  //Do not remove that line 
  delay(5000); // set a delay to ease programming, sleep mode can interfere with prog mode
  
  // Setup the RTCCounter
  rtcCounter.begin();

  // Set the alarm to generate an interrupt every s
  rtcCounter.setPeriodicAlarm(1);

  // Set the sleep mode
  //SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

  //disable usb
  USB->DEVICE.CTRLA.reg &= ~USB_CTRLA_ENABLE;

  if (! qt_top.begin() || ! qt_left.begin() || ! qt_right.begin() )  //eror with qtouch makes col 2 blink
    blinkLeds(2);
  
  ledsTest(0.5);
}


unsigned long pm = 0;

void loop() {
  // If the alarm interrupt has been triggered
  if (rtcCounter.getFlag()) {

    // Clear the interrupt flag
    rtcCounter.clearFlag();

    // Blink the LED
    /*
    digitalWrite(17, HIGH);
    delay(250);
    digitalWrite(17, LOW);*/
  }

  if(qt_top.measure() > QTOUCH_THRESHOLD)  //unlock and display the time
  {  
    timeToLeds();
    pm = millis(); 
    menuIndex = 0; //reset it if you are lost 

  }

  if(qt_left.measure() > QTOUCH_THRESHOLD) 
  {
    menuIndex = (menuIndex+1) % 4;
    //show index for 1s
    digitalWrite(ledPin[menuIndex][0], HIGH);
    ledsShow();
    delay(1000);
    ledsOff();

    pm = millis();
    
    if(menuIndex == 0)
      timeToLeds();
    if(menuIndex == 1)
      dateToLeds();
    if(menuIndex == 2)
      stopwatch();
    if(menuIndex == 3)
      remainingBattery();
  }
  
  if(qt_right.measure() > QTOUCH_THRESHOLD) 
  {
    ledsTest(1);
    pm = millis(); 
    setTime();
    ledsTest(0.5); //faster animation
    setDate();
    ledsTest(0.5);
    blink = false;
  }
  
  //blinkLeds(BLINK_ALL);

  //sleeps after timeout
  if(millis() - pm > ON_DURATION){
    // Sleep until the next interrupt
    ledsOff();
    systemSleep();
    //pm = millis();
    //METTRE D'ABORD -> pm = millis();
  }
  delay(100);
}

void systemSleep()
{
  // If the alarm interrupt has not yet triggered
  
  if (!rtcCounter.getFlag()) {
    
    // Wait For Interrupt
    __WFI();
  }
}

void blinkLeds(int col)
{
  if(blink && pmBlink-millis() > 500)
  {
    if(!blinkTog)
      ledsShow();
    else 
    {
      if(col == BLINK_ALL)
        ledsOff();
      else {
        int leds [4][4] = {};
        
        //save current state
        for(int i=0; i<4; i++){
          for(int j=0; j<4; j++){
            leds[i][j] = ledState[i][j];
          }
        }

        //shut off one the column that should blink
        for(int i=0; i<4; i++){
          ledState[col][i] = 0;
        }
        
        ledsShow();
        
        //restore state to power those leds on next time
        for(int i=0; i<4; i++){
          for(int j=0; j<4; j++){
            ledState[i][j] = leds[i][j];
          }
        }
      }
    }
      
    blinkTog = !blinkTog; //toggle state
    //reset timer
    pmBlink = millis();
  }
}

//show the leds that are supposed to be on
void ledsShow()
{
  ledsOff();
  for(int i=0; i<4; i++)
  {
    for(int j=0; j<4; j++)
    {
      digitalWrite(ledPin[i][j], ledState[i][j]);
    }
  }
}

//shut off all leds
void ledsOff()
{
  for(int i=0; i<4; i++)
  {
    for(int j=0; j<4; j++)
    {
      digitalWrite(ledPin[i][j], 0);
    }
  }
}

void ledsColOff(int col)
{
    for(int j=0; j<4; j++)
    {
      digitalWrite(ledPin[col][j], 0);
    }
}

void ledsTest(float speed)
{
  for(int i=0; i<4; i++)
  {
    for(int j=0; j<4; j++)
    {
      digitalWrite(ledPin[i][j], 1);
      delay(100*speed);
      digitalWrite(ledPin[i][j], 0);
      delay(50*speed);
    }
  }
}

void remainingBattery() 
{
  ledsOff();
  float voltage = analogRead(batteryProbe);
  //voltage dividor cuts volts of the battery in half
  //Supply voltage is 2.8V
  
  voltage = ( 2.8 * 1023 ) / voltage;
  if(voltage > 4)
    digitToLedColumn(0b1111, 3);
  else if(voltage > 3.6)
    digitToLedColumn(0b0111, 3);
  else if(voltage > 3.2)
    digitToLedColumn(0b0011, 3);
  else {
    digitToLedColumn(0b0001, 3);
    blink = true;
  }
  ledsShow();
}

void timeToLeds()
{
  // Get time as an epoch value and convert to tm struct
  time_t epoch = rtcCounter.getEpoch();
  struct tm* t = gmtime(&epoch);
  
  uint8_t timeArr[4] = {(uint8_t) ( (t->tm_hour/10) % 10 ),  //1st digit of hours
                     (uint8_t) ( (t->tm_hour/10) % 10 ),  //2nd of hours
                     (uint8_t) ( (t->tm_min/10) % 10 ),  //1st digit of min
                     (uint8_t) ( (t->tm_min/10) % 10 )   //2nd of min
                    };
  //uint8_t timeArr[4] = {1,8,3,5};         
                    
  for(int i=0; i<4; i++)  //read and copy the bits from digit to led digit
  {
    digitToLedColumn(timeArr[i],i);
  }
  ledsShow();
}

void dateToLeds() //close to TimeToLeds
{
  // Get time as an epoch value and convert to tm struct
  time_t epoch = rtcCounter.getEpoch();
  struct tm* t = gmtime(&epoch);
  
  uint8_t timeArr[4] = {(uint8_t) ( ((t->tm_mon + 1)/10) ),  //1st digit of month
                       (uint8_t) ( ((t->tm_mon + 1) %10) ),  //2nd of month
                       (uint8_t) ( (t->tm_mday/10) / 10 ),  //1st digit of month day
                       (uint8_t) ( (t->tm_mday/10) % 10 )   //2nd of month day
                      };
                    
  for(int i=0; i<4; i++)  //read and copy the bits from digit to led digit
  {
    digitToLedColumn(timeArr[i], i);
  }
  
}

void digitToLedColumn(uint8_t digit, int column)
{

  for(int i=0; i<4; i++)  //read and copy the bits from digit to led digit
  {
    ledState[column][3-i] = bitRead(digit, i); //starting from the rightmost bit and copying it to the bottom led
  }
}

void setTime()
{
  //blink = true;
  time_t epoch = rtcCounter.getEpoch();
  struct tm* t = gmtime(&epoch);

  int index = 0;
  uint8_t hour = t->tm_hour;
  uint8_t minute = t->tm_min;
  
  while(qt_top.measure() < QTOUCH_THRESHOLD+100 && index < 4)
  {
    if(qt_left.measure() > QTOUCH_THRESHOLD){
      if(index == 0){
        index += 2; //hours in one shot
        digitToLedColumn(0b1111, index+1);
      }
        
      else 
        index++; //min in two steps

      digitToLedColumn(0b1111, index);
      ledsShow();
      delay(300);
      ledsOff();

    }
    
    if(qt_right.measure() > QTOUCH_THRESHOLD){
      
      if(index == 0) //1st of hour 
        hour = (hour + 1) % 24;
      if(index == 2) //1st of min 
        minute = (minute + 10) % 60;
      if(index == 3)
        minute = (minute + 1) % 60;

      //refresh leds 
      uint8_t timeArr[4] = {hour / 10 ,  //1st digit of min
                         hour % 10,  //2nd of hours
                         minute / 10,  //1st digit of min
                         minute % 10   //2nd of min
                    };
                        
      for(int i=0; i<4; i++)  //read and copy the bits from digit to led digit
      {
        digitToLedColumn(timeArr[i], i);
      }
      ledsShow();
      //rtcCounter.setEpoch(mktime(&t)); //pas fou de faire ça à chaque fois, on verra pour faire propre + tard
      delay(300);
      
    }

    //blink active column(s)
    ledsColOff(index); //okay since it does not modify leds array
    if(index == 0)  //if hours, blink 2 col
      ledsColOff(index+1);
    delay(100);
    ledsShow();    

    //blinkLeds(index); //do not works
    saveTime(hour, minute);
  }
  //blink = false;
}

void saveTime(uint8_t hour, uint8_t minute)
{
  //get date first ????? 
  time_t epoch = rtcCounter.getEpoch();
  struct tm* t = gmtime(&epoch);
  
  struct tm tm;

  tm.tm_isdst = t->tm_isdst;
  tm.tm_yday = t->tm_yday;
  tm.tm_wday = t->tm_wday;
  tm.tm_year = t->tm_year;
  tm.tm_mon = t->tm_mon;
  tm.tm_mday = t->tm_mday;
  tm.tm_hour = hour;
  tm.tm_min = minute;
  tm.tm_sec = 0;
  
  rtcCounter.setEpoch(mktime(&tm));
}

void setDate()
{
  //same as time, please verify the above function first
}

void stopwatch()
{
  //first column is now min and last is last digit of seconds 
  unsigned long sTime = millis();
  unsigned long oneHertz = millis();
  unsigned long pauseTime = 0;

  bool paused = true; //start in paused mode for convenience
  
  ledsOff();
  
  while(qt_top.measure() < QTOUCH_THRESHOLD)
  {

    //left button will pause 
    if(qt_right.measure() > QTOUCH_THRESHOLD)
    {
      paused = !paused;
      delay(1000);
    }
    
    if(millis()-oneHertz > 1000)
    {
      oneHertz = millis();

      //remove time when it was paused
      if(paused)
        pauseTime += 1000;
      
      //display time from start 
      uint8_t minutes = (millis() - sTime - pauseTime) / 60000;
      uint8_t sec = ( (millis() / 1000) % 60000);
      
      uint8_t timeArr[4] = {minutes / 10 ,  //1st digit of min
                         minutes % 10,  //2nd of hours
                         sec / 10,  //1st digit of min
                         sec % 10   //2nd of min
                    };
                        
      for(int i=0; i<4; i++)  //read and copy the bits from digit to led digit
      {
        digitToLedColumn(timeArr[i], i);
      }
      ledsShow();
    }
  }
}
