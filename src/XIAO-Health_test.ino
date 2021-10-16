/*
  Seeeduino XIAO expansion board - Health Check Test

  This example implements a prety basic clock, exercising most of the Seeeduino XIAO expansion board functions.
  The managed interfaces are:
    - External LiPo Battery;
    - OLED display;
    - Real Time Clock;
    - User push button;
    - SD card.

   At stat-up via the ON/OFF switch when powered-on from battery, the system reads the SD card to get the last date/time it has been switched-on.
   This is displayed on the OLED as long as the user button is NOT press.
   When done, current start-up time is stored onto the SD, the buzzer rings and OLED display is switched-off.
   From that moment, current date and time is displayed each time the button is pressed.
   The buzzer rings at every O-clock.

   Created the 10th of April 2021 at DléLabs - xiao@djelabs.com
*/

//#define DEBUG  // /!\ Comment out when running not connected to the serial interface.
#ifdef DEBUG
#define DEBUG_PRINT(...)    Serial.print(__VA_ARGS__)
#define DEBUG_PRINTLN(...)  Serial.println(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#define DEBUG_PRINTLN(...)
#endif

#include <Arduino.h>
#include <U8x8lib.h>
#include <PCF8563.h>  //  Bill2462's library
#include <SPI.h>
#include <SD.h>

// RTC
PCF8563 rtc;
Time nowTime;

// Button
const int buttonPin = 1;     // the number of the pushbutton pin
int buttonState = 0;    // variable for reading the pushbutton status

// Display
bool DisplayIsOFF = false;
U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);

// Buzzer
int speakerPin = 3;

// SD card
const int chipSelect = 2;
String msg = "";
File myFile;
bool SDCardFound = false;


void setup() {
  Serial.begin(9600);
#ifdef DEBUG
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo AND Seeduino XIAO
  }
#endif
  DEBUG_PRINTLN("Start-Up");
  // initialize the LED pin as an output:
  pinMode(LED_BUILTIN, OUTPUT);
  // initialize the pushbutton pin as an input:
  pinMode(buttonPin, INPUT_PULLUP);
  // SD card initialiation
  if (!SD.begin(chipSelect)) {
    DEBUG_PRINTLN("SD initialization failed!");
    SDCardFound = false;
  }
  else
  {
    SDCardFound = true;
  }
  // OLED Initialisation
  u8x8.begin();
  u8x8.setFlipMode(0);
  // choose a suitable font amongst the 4 choices below
  //u8x8.setFont(u8x8_font_chroma48medium8_r);   
  //u8x8.setFont(u8x8_font_chroma48medium8_n);
  //u8x8.setFont(u8x8_font_chroma48medium8_u);
  u8x8.setFont(u8x8_font_torussansbold8_r);

  Wire.begin();
  rtc.init();//initialize the clock with default I2C @ equal to 0x51

  //  Comment out after setting the time for the first run
/*
  rtc.stopClock();//stop the clock
  rtc.setYear(21);//set year
  rtc.setMonth(04);//set month
  rtc.setDay(15);//set dat
  rtc.setHour(13);//set hour
  rtc.setMinut(8);//set minute
  rtc.setSecond(00);//set second
  rtc.startClock();//start the clock
*/
  if (SDCardFound == true) {
    // Acquire start-up time
    nowTime = rtc.getTime();//get current time
    // Check if no file is stored on the SD : case of the first time system is started-up.
    if ( SD.exists("log.txt") )
    {
      //
      // file exists : just need to read the previous start date&time and write the current one.
      DEBUG_PRINTLN("File exists");
      myFile = SD.open("log.txt", FILE_READ);
      //
      // if the file opened okay, read to it:
      if (myFile) {
        DEBUG_PRINTLN("File opened");
        msg = "";
        int cursorline = 0;
        u8x8.setCursor(0, cursorline);
        u8x8.print("Last start:");
        cursorline += 10;
        char filereadout;
        //
        // read-out the last start
        // println test ends with /r -ascii 13 AND /n ascii 10)
        while (myFile.available()) {
          filereadout = (myFile.read());
          if (filereadout == 13)
          {
            // cariage return detected: print the retrieved line}
            u8x8.setCursor(0, cursorline);
            u8x8.print(msg);
            DEBUG_PRINT("Msg read = ");
            DEBUG_PRINTLN(msg);
            cursorline += 10;
          } else
          {
            if (filereadout == 10)
            { 
              // new character line ==> clears the stored message
              msg = "";
            }
            else
            {
              msg += filereadout;
            }
          }
        }
        //
        // Display as long as user does not confirm by a button click
        WaitUserClick();
        u8x8.clearDisplay();
        //
        // Reading is done.
        // delete the file and write down the date and time at start-up.
        myFile.close();
        myFile.flush();
        bool status;
        status = SD.remove("log.txt");
        if ( status == true)
        {
          DEBUG_PRINTLN("File removal successful");
        }
        else
        { DEBUG_PRINT("File removal failled with error code= "); DEBUG_PRINTLN(status);
        }
        myFile = SD.open("log.txt", O_CREAT | O_WRITE );
        buildDate();
        myFile.println(msg);
        buildTime();
        myFile.println(msg);
        myFile.close();
        myFile.flush();
        DEBUG_PRINTLN("log.txt updated");
        myFile.close();
      } else
      {
        // if the file didn't open, print an error:
        DEBUG_PRINTLN("error opening test.txt");
      }
    }
    else
    {
      u8x8.setCursor(0, 0);
      u8x8.print("Last start:");
      u8x8.setCursor(0, 10);
      u8x8.print(" None");
      DEBUG_PRINTLN("File does NOT exist");
      myFile = SD.open("log.txt", O_CREAT | O_WRITE );
      if (myFile) {
        buildDate();
        myFile.println(msg);
        buildTime();
        myFile.println(msg);
        myFile.close();
        myFile.flush();
        DEBUG_PRINTLN("log.txt created");
      }
      else {
        DEBUG_PRINTLN("error creating log.txt");
      }
    }
  } // endif (SDCardFound == true)

  else
  {
    // cariage return detected: print the retrieved line}
    u8x8.setCursor(0, 0);
    u8x8.print("No SD card found");
    u8x8.setCursor(0, 10);
    u8x8.print("at start-up");
    WaitUserClick();
  }
}

void loop() {
  nowTime = rtc.getTime();//get current time
  if (nowTime.minute == 0 && nowTime.second == 0)
  {
    playTone(200, 500);
    delay(500);
    playTone(200, 500);
  }
  // read the state of the pushbutton value:
  buttonState = digitalRead(buttonPin);
  if (buttonState == LOW)
  {
    DEBUG_PRINTLN("Button pressed");
    DisplayIsOFF = false;
    u8x8.setCursor(0, 0);
    twoDigitDisplay(nowTime.day);
    u8x8.print("/");
    twoDigitDisplay(nowTime.month);
    u8x8.print("/");
    u8x8.print("20");
    twoDigitDisplay(nowTime.year);
    // Time for the time
    u8x8.setCursor(0, 10);
    twoDigitDisplay(nowTime.hour);
    u8x8.print(":");
    twoDigitDisplay(nowTime.minute);
    u8x8.print(":");
    twoDigitDisplay(nowTime.second);
    delay(1000);
  }
  else
  {
    if (DisplayIsOFF == false)
    {
      u8x8.clearDisplay();
      DisplayIsOFF = true;
    }
  }
}

// miscelanious functions
//
void playTone(int tone, int duration) {
  for (long i = 0; i < duration * 1000L; i += tone * 2) {
    digitalWrite(speakerPin, HIGH);
    delayMicroseconds(tone);
    digitalWrite(speakerPin, LOW);
    delayMicroseconds(tone);
  }
}

void buildDate() {
  msg = "";
  msg = nowTime.day;
  msg += "/";
  msg += nowTime.month;
  msg += "/20";
  msg += nowTime.year;
  DEBUG_PRINT("Msg built =");
  DEBUG_PRINTLN(msg);
}

void buildTime() {
  msg = "";
  msg += nowTime.hour;
  msg += ":";
  msg += nowTime.minute;
  msg += ":";
  msg += nowTime.second;
  DEBUG_PRINT("Msg built =");
  DEBUG_PRINTLN(msg);
}

void twoDigitDisplay(int value) {
  if (value < 10)
  {
    u8x8.print("0");
  }
  u8x8.print(value);
}

void WaitUserClick() {
  buttonState = digitalRead(buttonPin);
  while (buttonState == HIGH)
  {
    buttonState = digitalRead(buttonPin);
  }
  playTone(200, 500);
  delay(500);
}