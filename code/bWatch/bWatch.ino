/*******************************
 * 
 * TODO :
 * Attention à l'overflow de millis() ???
 * 
 * 
 * Binary watch version 0.1
 * 
 * How to read it (BCD, binary coded digits)
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
 * Top button, Unlock and show time 
 * Left button : set time and then date 
 * Right button : change mode (time, date, stopwatch, remaining battery)
 * See readme for more details
 * 
 *******************************/

#include <RTCCounter.h>
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
volatile bool wasSleeping = false;

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
  
  // Setup the RTCCounter
  rtcCounter.begin();

  // Set the alarm to generate an interrupt every s
  rtcCounter.setPeriodicAlarm(SLEEP_DURATION);

  //disable usb
  USBDevice.detach();


  if (! qt_top.begin() || ! qt_left.begin() || ! qt_right.begin() )  //eror with qtouch makes col 2 blink
  {
    digitalWrite(21,!digitalRead(21));
    delay(500);
  }
  
  saveTime(12,30); //for debug
}

void loop() {
  // If the alarm interrupt has been triggered
  if (rtcCounter.getFlag()) {

    // Clear the interrupt flag
    rtcCounter.clearFlag();

    // Blink the LED
    /*digitalWrite(17, HIGH);
    delay(250);
    digitalWrite(17, LOW);*/
  }

  if(qt_top.measure() > QTOUCH_THRESHOLD)  //unlock and display the time
  {  
    wasSleeping = false;
    timeToLeds();
    delay(3000);
    pm = millis(); 
    menuIndex = 0; //return to first screen (time) if you are lost 
  }

  if(qt_right.measure() > QTOUCH_THRESHOLD) 
  {
    menuIndex = (menuIndex+1) % 4;
    //show index for 1s
    digitalWrite(ledPin[menuIndex][0], HIGH);
    delay(500);
    digitalWrite(ledPin[menuIndex][0], LOW);

    pm = millis();
    
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
  }
  
  if(qt_left.measure() > QTOUCH_THRESHOLD) 
  {
    ledsTest(0.75);
    pm = millis(); 
    setTime();
    ledsTest(0.5); //faster animation
    setDate();
    ledsTest(0.5);
  }

  //sleeps after timeout
  if(millis() - pm > ON_DURATION || wasSleeping == true){
    // Sleep until the next interrupt
    ledsOff();
    systemSleep();
    //LowPower.sleep(1000);
    //pm = millis();
    //METTRE D'ABORD -> pm = millis();
  }
  delay(100);
}
/*
void onWakeUp() {
  wasSleeping = true;  
  // This function will be called once on device wakeup
  // You can do some little operations here (like changing variables which will be used in the loop)
  // Remember to avoid calling delay() and long running functions since this functions executes in interrupt context
}*/

void systemSleep()
{
  // If the alarm interrupt has not yet triggered
  
  if (!rtcCounter.getFlag()) {
    /*__DSB();
    // Wait For Interrupt
    __WFI();*/
    wasSleeping = true;  
    //from ArduinoLowPower
    // Disable systick interrupt:  See https://www.avrfreaks.net/forum/samd21-samd21e16b-sporadically-locks-and-does-not-wake-standby-sleep-mode
    SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;	
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
    __DSB();
    __WFI();
    // Enable systick interrupt
    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
  }

}

/*
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
}*/

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
  time_t epoch = rtcCounter.getEpoch();
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
  time_t epoch = rtcCounter.getEpoch();
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
  time_t epoch = rtcCounter.getEpoch();
  struct tm* t = gmtime(&epoch);

  int index = 0;
  uint8_t hour = t->tm_hour;
  uint8_t minute = 11; //the two bottom leds will be on

  //display current value
  hour = hour-1; //if init will do +1, counter it 
  bool doInit = 1;

  
  while(qt_top.measure() < QTOUCH_THRESHOLD+100 && index < 4) 
  {
    if(qt_right.measure() > QTOUCH_THRESHOLD){
      if(index == 0){
        index += 2; //hours in one shot
        //digitToLedColumn(0b1111, index+1);
      }
      else 
        index++; //set minutes in two steps

      //digitToLedColumn(0b1111, index%4); //on all leds of the column prevent accessing col 4 that do not exists
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
  //time_t epoch = rtcCounter.getEpoch();
  //struct tm* t = gmtime(&epoch);

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
  time_t epoch = rtcCounter.getEpoch();
  struct tm* t = gmtime(&epoch);
  
  struct tm tm;

  tm.tm_isdst = t->tm_isdst;
  tm.tm_yday = t->tm_yday;
  tm.tm_wday = t->tm_wday;
  tm.tm_year = t->tm_year;
  tm.tm_mon = month;
  tm.tm_mday = day;
  tm.tm_hour = t->tm_hour;
  tm.tm_min = t->tm_min;
  tm.tm_sec = t->tm_sec;
  
  rtcCounter.setEpoch(mktime(&tm));
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

    //right button will pause 
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

      //remove time when it was paused
      //if(paused)
      //  pauseTime += 1000;
      if(paused) //pause correspond to delaying startTime
        sTime += 1000;
      
      //display time from start 
      uint8_t minutes = (millis() - sTime/* - pauseTime*/) / 60000;
      uint8_t sec = ( ((millis() - sTime) / 1000) % 60000);
      
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

