/*******************************
 * Binary watch version 0.1
 *
 * How to read it (BCD, binary coded digits)
 * Each digit is separated. The two first digits are for the hours, the two last for minutes
 * the Bottom digit adds 1, the second 2, the third 4, and the top one 8
 * For instance if # are on and _ are off
 *      _ _ _ _           ex 2)   _ _ _ _
 *      _ _ # #                   _ _ _ #
 *      _ # _ #                   # _ # _
 *      # # _ #                   _ # # #
 *      1 3:4 7                   2 1:3 5
 *
 * Buttons
 * Top button, Unlock and show time
 * Left button : set time and then date
 * Right button : change mode (time, date, stopwatch, remaining battery)
 * See readme for more details
 *
 *******************************/

#include <RTCZero.h>
#include <time.h>
#include "Adafruit_FreeTouch.h"

#define BUTTON_RIGHT 42//PA03
#define BUTTON_LEFT A5//PB02
#define BUTTON_TOP A1//PB08
#define BATTERY_PROBE 25 //PB03;

#define QTOUCH_THRESHOLD 600

#define ON_DURATION 10000 //sec during which the watch is showing time
#define SLEEP_DURATION 3 //sec during which the device is sleeping. Correspond to the max amount of time between 2 readings of the capacitive button

Adafruit_FreeTouch qt_top = Adafruit_FreeTouch(BUTTON_TOP, OVERSAMPLE_4, RESISTOR_50K, FREQ_MODE_NONE);
Adafruit_FreeTouch qt_left = Adafruit_FreeTouch(BUTTON_LEFT, OVERSAMPLE_4, RESISTOR_50K, FREQ_MODE_NONE);
Adafruit_FreeTouch qt_right = Adafruit_FreeTouch(BUTTON_RIGHT, OVERSAMPLE_4, RESISTOR_50K, FREQ_MODE_NONE);

RTCZero rtc;

//Pins for Arduino Zero (Programming port) board. Do not forget to burn the bootloader
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

unsigned long pm = 0;
uint8_t menuIndex = 0; //(dispaly time, date, stopwatch, remaining battery)
bool wasSleeping = false;

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

  ledsTest(3); // set a delay to ease programming, sleep mode can interfere with prog mode
  //not actually needed if we use a programmer

  // Setup the RTC
  rtc.begin(false); //don't reset time

  //disable usb 
  //USBDevice.detach();
  USB->DEVICE.CTRLA.reg &= ~USB_CTRLA_ENABLE;

  if (! qt_top.begin() || ! qt_left.begin() || ! qt_right.begin() )  //eror with qtouch makes col 2 blink
  {
    digitalWrite(21,!digitalRead(21));
    delay(500);
  }
  saveTime(12, 11); //the watch is not supposed to shudown.
}

void loop() {

  if(qt_top.measure() > QTOUCH_THRESHOLD)  //unlock and display the time
  {
    wasSleeping = false;
    timeToLeds();
    delay(3000);
    pm = millis();
    menuIndex = 0; //return to first screen (time) if you are lost
  }

  if(qt_right.measure() > QTOUCH_THRESHOLD /*&& wasSleeping = false*/)
  {
    menuIndex = (menuIndex+1) % 4;
    //show index for 1s
    digitalWrite(ledPin[menuIndex][0], HIGH);
    delay(500);
    digitalWrite(ledPin[menuIndex][0], LOW);

    if(menuIndex == 0)
      timeToLeds();
    if(menuIndex == 1)
      dateToLeds();
    if(menuIndex == 2){
      stopwatch();
      menuIndex++; //don't forget to inc menu because stopwatch is a new screen
    }
    if(menuIndex == 3){
      remainingBattery(3000);
    }
    pm = millis();
  }

  if(qt_left.measure() > QTOUCH_THRESHOLD /*&& wasSleeping = false*/)
  {
    ledsTest(0.75);
    setTime();
    ledsTest(0.5); //faster animation
    setDate();
    ledsTest(0.5);

    menuIndex = 0; //return to time display
    pm = millis();
  }

  //sleeps after timeout or if we woke up and the user didn't touch any button
  if(millis() - pm > ON_DURATION || wasSleeping == true){
    // Sleep until the next interrupt
    ledsOff();
    wasSleeping = true;
    //Set sleep mask to standby
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
    rtc.enableAlarm(rtc.MATCH_HHMMSS);
    //rtc.setAlarmSeconds(rtc.getSeconds()+SLEEP_DURATION); // no !!! doesn't works if we change the time with setTime. The alarm is not only based on seconds
    rtc.setAlarmEpoch(rtc.getEpoch()+SLEEP_DURATION);
    delay(10);
    rtc.standbyMode();
  }
  delay(100);
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

void remainingBattery(int time)
{
  ledsOff();
  float voltage = analogRead(BATTERY_PROBE);
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
  }
  ledsShow();
  delay(time);  //stay a bit on this screen
  digitToLedColumn(0b0000,4); //remove on "buffer"
  ledsOff(); //shutdown leds
}

void timeToLeds()
{
  ledsOff();
  // Get time as an epoch value and convert to tm struct
  time_t epoch = rtc.getEpoch(); //we can use directly getHours and getMinutes but I'm coming from RtcCounter lib
  struct tm* t = gmtime(&epoch);

  int timeArr[4] = { ( (t->tm_hour/10) % 10 ),  //1st digit of hours
                     ( (t->tm_hour) % 10 ),  //2nd of hours
                     ( (t->tm_min/10) % 10 ),  //1st digit of min
                     ( (t->tm_min) % 10 )   //2nd of min
                    };

  for(int i=0; i<4; i++)  //read and copy the bits from digit to led digit
  {
    digitToLedColumn(timeArr[i],i);
  }
  ledsShow();
}

void dateToLeds() //close to TimeToLeds
{
  ledsOff();
  // Get time as an epoch value and convert to tm struct
  //time_t epoch = rtcCounter.getEpoch();
  time_t epoch = rtc.getEpoch();
  struct tm* t = gmtime(&epoch);

  //do +1 because RTC starts at 0 and human month and days at 1
  int timeArr[4] = {(t->tm_mday+1) / 10,  //1st digit of month day
                    (t->tm_mday+1) % 10,  //2nd of month day
                    (t->tm_mon + 1) / 10,  //1st digit of month
                    (t->tm_mon + 1) % 10  //2nd of month
                  };

  for(int i=0; i<4; i++)  //read and copy the bits from digit to led digit
  {
    digitToLedColumn(timeArr[i], i);
  }
  ledsShow();
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
  int index = 0;
  uint8_t hour = rtc.getHours();
  uint8_t minute = 11; //the two bottom leds will be on

  //display current value
  hour = hour-1; //if init will do +1, counter it
  bool doInit = 1; //force display

  //top button to quit if not finished, better to click right
  while(qt_top.measure() < QTOUCH_THRESHOLD+100 && index < 4)
  {
    //hours modified, edit sec
    if(qt_right.measure() > QTOUCH_THRESHOLD){
      if(index == 0){
        index += 2; //hours in one shot
      }
      else
        index++; //set minutes in two steps

      ledsShow();
      delay(300);
      ledsOff();
    }

    if( (qt_left.measure() > QTOUCH_THRESHOLD || doInit == 1) && index < 4){
      doInit = 0; //do it once only

      if(index == 0) //1st of hour
        hour = (hour + 1) % 24;
      if(index == 2) //1st of min
        minute = (minute + 10) % 60;
      if(index == 3){
        if(minute % 10 == 9)
          minute -= 10; //don't modify the tens digit of the minute, too strange and unexpected when we set the time
        minute = (minute + 1) % 60;

      }

      //refresh leds
      uint8_t timeArr[4] = {hour / 10,  //1st digit of min
                         hour % 10,  //2nd of hours
                         minute / 10,  //1st digit of min
                         minute % 10   //2nd of min
                    };

      for(int i=0; i<4; i++)  //read and copy the bits from digit to led digit
      {
        digitToLedColumn(timeArr[i], i);
      }
      ledsShow();
      delay(300);

    }

    //blink active column(s)
    ledsColOff(index); //okay since it does not modify leds array
    if(index == 0)  //if hours, blink 2 col
      ledsColOff(index+1);
    delay(50);
    ledsShow();
    delay(200);

  }
  saveTime(hour, minute);
}

void saveTime(uint8_t hour, uint8_t minute)
{
  rtc.setHours(hour);
  rtc.setMinutes(minute);
}

void setDate()
{
  int index = 0;
  uint8_t month = 0;
  uint8_t mday = 0; // month day (not week day)

  //display current value
  bool doInit = 1;

  while(qt_top.measure() < QTOUCH_THRESHOLD+100 && index < 4)
  {
    if(qt_right.measure() > QTOUCH_THRESHOLD){

      index += 2;

      ledsShow();
      delay(300);
      ledsOff();
    }

    if( (qt_left.measure() > QTOUCH_THRESHOLD || doInit == 1) && index < 4){
      doInit = 0; //do it once only

      if(index == 0) //first we modify mday
        mday = (mday + 1) % 31;
      if(index == 2) //then month
        month = (month + 1) % 12;


      //refresh leds
      uint8_t timeArr[4] = {(mday+1) / 10, //do +1 because January 1st is month 0 and mday 0 for ex, starts at 0 not 1
                         (mday+1) % 10,
                         (month+1) / 10,
                         (month+1) % 10
                    };

      for(int i=0; i<4; i++)  //read and copy the bits from digit to led digit
      {
        digitToLedColumn(timeArr[i], i);
      }
      ledsShow();
      delay(300);

    }

    //blink active column(s)
    ledsColOff(index); //okay since it does not modify leds array
    ledsColOff(index+1);//blink two colums
    delay(50);
    ledsShow();
    delay(200);

  }
  saveDate(mday, month);
}

void saveDate(uint8_t day, uint8_t month)
{
  rtc.setDay(day);
  rtc.setMonth(month);
}

void stopwatch()
{
  //first column is now min and last is last digit of seconds
  unsigned long sTime = millis();
  unsigned long oneHertz = millis();

  bool paused = false; //don't start in pause mode as it's difficult to understand what's happening if it do nothing

  ledsOff();

  while(qt_right.measure() < QTOUCH_THRESHOLD)
  {

    //left button will pause
    if(qt_left.measure() > QTOUCH_THRESHOLD)
    {
      paused = !paused;
      ledsOff();
      delay(500);
      ledsShow();
      delay(500);
    }

    if(millis()-oneHertz >= 1000)
    {
      oneHertz = millis();

      if(paused) //pause correspond to delaying startTime
        sTime += 1000;

      //display time from start
      uint8_t minutes = ( (millis() - sTime) / 1000) / 60;
      uint8_t sec = ( ((millis() - sTime) / 1000) % 60);

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
