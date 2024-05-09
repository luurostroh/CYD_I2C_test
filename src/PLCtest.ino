/*
  The TFT_eSPI library incorporates an Adafruit_GFX compatible
  button handling class, this sketch is based on the Arduin-o-phone
  example.

  This example displays a keypad where numbers can be entered and
  sent to the Serial Monitor window.

  The sketch has been tested on the ESP8266 (which supports SPIFFS)

  The minimum screen size is 320 x 240 as that is the keypad size.
*/

// The SPIFFS (FLASH filing system) is used to hold touch screen
// calibration data

#include "FS.h"

#include <SPI.h>
#include <TFT_eSPI.h>  // Hardware-specific library
#include "Wire.h"
#include "ESP32TimerInterrupt.h"
//touch panel
#include <XPT2046_Touchscreen.h>

/////////////////////////////////////////////////////////
#define IN1 0x1
#define IN2 0x2
#define IN3 0x4
#define IN4 0x8
#define IN5 0x10
#define IN6 0x20
#define IN7 0x40
#define IN8 0x80

#define OUT1 0x1
#define OUT2 0x2
#define OUT3 0x4
#define OUT4 0x8
#define OUT5 0x10
#define OUT6 0x20
#define OUT7 0x40
#define OUT8 0x80

typedef union 
{
  uint64_t data;
  uint8_t data_arr[8];
}I2C_data_t;
I2C_data_t i2c_inputs;
I2C_data_t i2c_outputs;

#define I2C_OUT_ADDR 56
#define I2C_IN_ADDR 48

#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33
SPIClass TP_Spi = SPIClass(HSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);
// calibration values
float xCalM = 0.07, yCalM = 0.09; // gradients
float xCalC = -17.77, yCalC = -11.73; // y axis crossing points
//end touch

TFT_eSPI tft = TFT_eSPI();  // Invoke custom library

// This is the file name used to store the calibration data
// You can change this to create new calibration files.
// The SPIFFS file name must start with "/".
#define CALIBRATION_FILE "/TouchCalData1"

// Set REPEAT_CAL to true instead of false to run calibration
// again, otherwise it will only be done once.
// Repeat calibration if you change the screen rotation.
#define REPEAT_CAL false

// Keypad start position, key sizes and spacing
#define KEY_X 40  // Centre of key
#define KEY_Y 96
#define KEY_W 20  // Width and height
#define KEY_H 20
#define KEY_SPACING_X 5  // X and Y gap
#define KEY_SPACING_Y 5
#define KEY_TEXTSIZE 1  // Font size multiplier

// Using two fonts since numbers are nice when bold
#define LABEL1_FONT &FreeMonoBold9pt7b//FreeSans9pt7b//FreeSansOblique12pt7b  // Key label font 1
#define LABEL2_FONT &FreeSansBold12pt7b     // Key label font 2

// Numeric display box size and location
#define DISP_X 1
#define DISP_Y 10
#define DISP_W 238
#define DISP_H 50
#define DISP_TSIZE 3
#define DISP_TCOLOR TFT_CYAN

// Number length, buffer for storing it and character index
#define NUM_LEN 12
char numberBuffer[NUM_LEN + 1] = "";
uint8_t numberIndex = 0;

// We have a status line for messages
#define STATUS_X 120  // Centred on this
#define STATUS_Y 65

// Create 15 keys for the keypad
char keyLabel[16][5] = { "1", "2", "3", "4", "5", "6", "7", "8", "1", "2", "3", "4", "5", "6", "7" , "8"};
uint16_t keyColor[16] = { TFT_RED, TFT_DARKGREY, TFT_DARKGREEN,
                          TFT_BLUE, TFT_BLUE, TFT_BLUE,
                          TFT_BLUE, TFT_BLUE, TFT_BLUE,
                          TFT_BLUE, TFT_BLUE, TFT_BLUE,
                          TFT_BLUE, TFT_BLUE, TFT_BLUE };

// Invoke the TFT_eSPI button class and create all the button objects
TFT_eSPI_Button inps[8];
TFT_eSPI_Button outs[8];

//------------------------------------------------------------------------------------------

bool IRAM_ATTR TimerHandler0(void * timerNo)
{
   R_W_i2c();
//Serial.println("tick");
	return true;
}

ESP32Timer ITimer0(0);

void setup() 
{
  
//  Wire.setPins(27,22);
  Wire.begin(27,22,400000);
  // Use serial port
  Serial.begin(9600);
  // Start the SPI for the touch screen and init the TS library
  TP_Spi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin(TP_Spi);
  ts.setRotation(1);
  // Initialise the TFT screen
  tft.init();

  // Set the rotation before we calibrate
  tft.setRotation(1);

  // Calibrate the touch screen and retrieve the scaling factors
  //touch_calibrate();

  // Clear the screen
  tft.fillScreen(TFT_BLACK);

  // Draw keypad background
 // tft.fillRect(0, 0, 240, 320, TFT_DARKGREY);

  // Draw number display area and frame
  // tft.fillRect(DISP_X, DISP_Y, DISP_W, DISP_H, TFT_BLACK);
  // tft.drawRect(DISP_X, DISP_Y, DISP_W, DISP_H, TFT_WHITE);

 // touch_calibrate();
  // Draw keypad
  drawI_O();
  ITimer0.attachInterruptInterval(100000, TimerHandler0);
}

//------------------------------------------------------------------------------------------

void loop(void) 
{
  uint16_t t_x = 0, t_y = 0;  // To store the touch coordinates
  TS_Point p_raw,p_coord;

  // Pressed will be set true is there is a valid touch on the screen
  bool pressed = ts.tirqTouched() && ts.touched();
 i2c_outputs.data_arr[0] |= OUT1;
 delay(50);
 i2c_outputs.data_arr[0] &= ~OUT1; 
 delay(1000);  
  i2c_outputs.data_arr[0] |= OUT2;
 delay(50);
 i2c_outputs.data_arr[0] &= ~OUT2; 
 delay(1000); 
  i2c_outputs.data_arr[0] |= OUT3;
 delay(50);
 i2c_outputs.data_arr[0] &= ~OUT3; 
 delay(1000); 
  i2c_outputs.data_arr[0] |= OUT4;
 delay(50);
 i2c_outputs.data_arr[0] &= ~OUT4; 
 delay(1000); 
  i2c_outputs.data_arr[0] |= OUT5;
 delay(50);
 i2c_outputs.data_arr[0] &= ~OUT5; 
 delay(1000); 
  i2c_outputs.data_arr[0] |= OUT6;
 delay(50);
 i2c_outputs.data_arr[0] &= ~OUT6; 
 delay(1000); 
 i2c_outputs.data_arr[0] |= OUT7;
 delay(50);
 i2c_outputs.data_arr[0] &= ~OUT7; 
 delay(1000); 
  i2c_outputs.data_arr[0] |= OUT8;
 delay(50);
 i2c_outputs.data_arr[0] &= ~OUT8; 
 delay(1000); 
  }



//------------------------------------------------------------------------------------------

void drawI_O() 
{
  // Draw the keys
  char lbls[8][2]={"1","2","3","4","5","6","7","8"};
  tft.setFreeFont(LABEL1_FONT);
  uint16_t xpos = 20;
  for (uint8_t b = 0; b < 8; b++) 
  {
    outs[0].initButton(&tft,xpos,20,30,30,3,TFT_YELLOW,TFT_BLACK,lbls[b],1);
    outs[0].drawButton();
    xpos += 40;
  }
  xpos = 20;
  for (uint8_t c = 0; c < 8; c++) 
  {
    inps[0].initButton(&tft,xpos,220,30,30,3,TFT_DARKGREEN,TFT_YELLOW,lbls[c],1);
    inps[0].drawButton();
    xpos += 40;
  }
}





void R_W_i2c()
{

   //Read 8 bytes from the slave
  uint8_t bytesReceived = Wire.requestFrom(I2C_IN_ADDR, 8);
  Serial.printf("requestFrom: %u\n", bytesReceived);
  if ((bool)bytesReceived) 
  {  //If received more than zero bytes
    Wire.readBytes(i2c_inputs.data_arr, bytesReceived);
  }

 // i2c_outputs.data = ~i2c_inputs.data;
  //Write message to the slave
  Wire.beginTransmission(I2C_OUT_ADDR);
  Wire.write(i2c_outputs.data_arr,8);
  uint8_t error = Wire.endTransmission(true);
 // Serial.printf("endTransmission: %u\n", error);

 


}

//------------------------------------------------------------------------------------------

void touch_calibrate() 
{
    TS_Point p;
    int16_t x1,y1,x2,y2;

    // data not valid so recalibrate
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(20, 0);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    tft.println("Touch corners as indicated");

    tft.setTextFont(1);
    tft.println();
    while(ts.touched());
    tft.drawFastHLine(10,20,20,TFT_RED);
    tft.drawFastVLine(20,10,20,TFT_RED);
    while(!ts.touched());
    delay(50);
    p = ts.getPoint();
    x1 = p.x;
    y1 = p.y;
    tft.drawFastHLine(10,20,20,TFT_BLACK);
    tft.drawFastVLine(20,10,20,TFT_BLACK);
    delay(500);
    while(ts.touched());
    tft.drawFastHLine(tft.width() - 30,tft.height() - 20,20,TFT_RED);
    tft.drawFastVLine(tft.width() - 20,tft.height() - 30,20,TFT_RED);
    while(!ts.touched());
    delay(50);
    p = ts.getPoint();
    x2 = p.x;
    y2 = p.y;
    tft.drawFastHLine(tft.width() - 30,tft.height() - 20,20,TFT_BLACK);
    tft.drawFastVLine(tft.width() - 20,tft.height() - 30,20,TFT_BLACK);

    int16_t xDist = tft.width() - 40;
    int16_t yDist = tft.height() - 40;

    // translate in form pos = m x val + c
    // x
    xCalM = (float)xDist / (float)(x2 - x1);
    xCalC = 20.0 - ((float)x1 * xCalM);
    // y
    yCalM = (float)yDist / (float)(y2 - y1);
    yCalC = 20.0 - ((float)y1 * yCalM);

    Serial.print("x1 = ");Serial.print(x1);
    Serial.print(", y1 = ");Serial.print(y1);
    Serial.print("x2 = ");Serial.print(x2);
    Serial.print(", y2 = ");Serial.println(y2);
    Serial.print("xCalM = ");Serial.print(xCalM);
    Serial.print(", xCalC = ");Serial.print(xCalC);
    Serial.print("yCalM = ");Serial.print(yCalM);
    Serial.print(", yCalC = ");Serial.println(yCalC);

}

//------------------------------------------------------------------------------------------

// Print something in the mini status bar
void status(const char *msg) {
  tft.setTextPadding(240);
  //tft.setCursor(STATUS_X, STATUS_Y);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft.setTextFont(0);
  tft.setTextDatum(TC_DATUM);
  tft.setTextSize(1);
  tft.drawString(msg, STATUS_X, STATUS_Y);
}

//------------------------------------------------------------------------------------------
