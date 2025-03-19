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
I2C_data_t i2c_inputs={0};
I2C_data_t i2c_inputs_tmp={0};
I2C_data_t i2c_outputs={0};
I2C_data_t i2c_outputs_tmp={0};

#define I2C_OUT_ADDR 56
#define I2C_IN_ADDR 48
#define I2C_ADDR 2


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

// Global var: Touch threshold
static uint16_t touchThreshold = 250;
// Initialise with example calibration values so processor does not crash if xpt2046_xpt2046_setTouch() not called in xpt2046_setup()
static uint16_t touchCalibration_x0 = 195, touchCalibration_x1 = 3500, touchCalibration_y0 = 229, touchCalibration_y1 = 3741;
static uint8_t  touchCalibration_rotate = 0, touchCalibration_invert_x = 0, touchCalibration_invert_y = 0;
static uint16_t _width = 320, _height = 240;
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


SemaphoreHandle_t I2Csemaphore;


void xpt2046_convert_raw_xy(uint16_t *x, uint16_t *y);

void task_RW_I2C(void *parameter)
{
  while(1)
  {
     // xSemaphoreTake(I2Csemaphore, portMAX_DELAY);
      vTaskDelay(1);
      R_W_i2c();
  }

}
//------------------------------------------------------------------------------------------

int cnt=0;

bool IRAM_ATTR TimerHandler0(void * timerNo)
{
 static portBASE_TYPE xHigherPriorityTaskWoken;
 xHigherPriorityTaskWoken = pdFALSE;
// xSemaphoreGiveFromISR(I2Csemaphore,(BaseType_t*)&xHigherPriorityTaskWoken);
//  if(++cnt>500){cnt=0;Serial.println("tick");}

	return true;
}

ESP32Timer ITimer0(0);

void setup() 
{
  i2c_inputs.data = 0xFFFFFFFFFFFFFFFF;
  i2c_inputs_tmp.data = 0xFFFFFFFFFFFFFFFF;
  i2c_outputs.data = 0;
  i2c_outputs_tmp.data = 0;

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
  // touch_calibrate();

  // Clear the screen
  tft.fillScreen(TFT_BLACK);

  // Draw keypad background
 // tft.fillRect(0, 0, 240, 320, TFT_DARKGREY);

  // Draw number display area and frame
  // tft.fillRect(DISP_X, DISP_Y, DISP_W, DISP_H, TFT_BLACK);
  // tft.drawRect(DISP_X, DISP_Y, DISP_W, DISP_H, TFT_WHITE);

  // touch_calibrate();
  // tft.fillScreen(TFT_BLACK);
  // Draw keypad
   drawI_O();

  //test
  tft.drawFastHLine(0,20,320,TFT_RED);  
  tft.drawFastHLine(0,60,320,TFT_RED);  
  tft.drawFastHLine(0,120,320,TFT_RED);
  tft.drawFastHLine(0,180,320,TFT_RED);
  tft.drawFastHLine(0,220,320,TFT_RED);
  

  tft.drawFastVLine(20,0,240,TFT_RED); 
  tft.drawFastVLine(40,0,240,TFT_RED); 
  tft.drawFastVLine(60,0,240,TFT_RED); 
  tft.drawFastVLine(80,0,240,TFT_RED);
  tft.drawFastVLine(100,0,240,TFT_RED); 
  tft.drawFastVLine(120,0,240,TFT_RED); 
  tft.drawFastVLine(140,0,240,TFT_RED); 
  tft.drawFastVLine(160,0,240,TFT_RED);
  tft.drawFastVLine(240,0,240,TFT_RED);
  tft.drawFastVLine(300,0,240,TFT_RED);
   //test


  xTaskCreate(task_RW_I2C,"taskRW",10000,NULL,1,NULL);
  vSemaphoreCreateBinary(I2Csemaphore);
  ITimer0.attachInterruptInterval(10000, TimerHandler0);
 vTaskStartScheduler();
}

//------------------------------------------------------------------------------------------

void loop(void) 
{
  TS_Point p;
  uint16_t x_,y_;
    if (ts.tirqTouched() && ts.touched()){
        p = ts.getPoint();
        x_ = p.x;
        y_ = p.y;
        xpt2046_convert_raw_xy(&x_,&y_);

        Serial.print("raw x");Serial.print(p.x);Serial.print(" y");Serial.println(p.y);
        Serial.print("press x");Serial.print(x_);Serial.print(" y");Serial.println(y_);Serial.println();
        
        // Adjust press state of each key appropriately
        for (uint8_t b = 0; b < 8; b++) {
            if (inps[b].contains(p.x,p.y)) {
              Serial.print("stisknut button ");Serial.println(b);
                inps[b].press(true); // tell the button it is pressed
            } else {
                inps[b].press(false); // tell the button it is NOT pressed
            }
        }
    }

  // uint16_t t_x = 0, t_y = 0;  // To store the touch coordinates
  // TS_Point p_raw,p_coord;
  // // Pressed will be set true is there is a valid touch on the screen
  // bool pressed = ts.tirqTouched() && ts.touched();
//   Serial.print(pressed);
//  i2c_outputs.data_arr[0] |= OUT1;
//  delay(100);
//  i2c_outputs.data_arr[0] &= ~OUT1; 
//  delay(200);  
//   i2c_outputs.data_arr[0] |= OUT2;
//  delay(100);
//  i2c_outputs.data_arr[0] &= ~OUT2; 
//  delay(200); 
//   i2c_outputs.data_arr[0] |= OUT3;
//  delay(100);
//  i2c_outputs.data_arr[0] &= ~OUT3; 
//  delay(200); 
//   i2c_outputs.data_arr[0] |= OUT4;
//  delay(100);
//  i2c_outputs.data_arr[0] &= ~OUT4; 
//  delay(200); 
//   i2c_outputs.data_arr[0] |= OUT5;
//  delay(100);
//  i2c_outputs.data_arr[0] &= ~OUT5; 
//  delay(200); 
//   i2c_outputs.data_arr[0] |= OUT6;
//  delay(100);
//  i2c_outputs.data_arr[0] &= ~OUT6; 
//  delay(200); 
//  i2c_outputs.data_arr[0] |= OUT7;
//  delay(100);
//  i2c_outputs.data_arr[0] &= ~OUT7; 
//  delay(200); 
//   i2c_outputs.data_arr[0] |= OUT8;
//  delay(100);
//  i2c_outputs.data_arr[0] &= ~OUT8; 
for (size_t i = 0; i < 8; i++)
{
  if(inps[i].justPressed())
  {
    Serial.print("press ");Serial.println(i);
    i2c_inputs.data |= 1<<i;
  }
  else i2c_inputs.data &= ~(1<<i);

}

 delay(200); 
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
    outs[b].initButton(&tft,xpos,20,30,30,4,TFT_DARK_YELLOW,TFT_BLACK,lbls[b],1);
    outs[b].drawButton();
    xpos += 40;
  }
  xpos = 20;
  for (uint8_t c = 0; c < 8; c++) 
  {
    inps[c].initButton(&tft,xpos,220,30,30,3,TFT_DARKGREEN,TFT_YELLOW,lbls[c],1);
    inps[c].drawButton();
    xpos += 40;
  }
}





void R_W_i2c()
{
  uint64_t i_o_mask = 0;
   //Read 8 bytes from the slave
 uint8_t bytesReceived;// = Wire.requestFrom(I2C_IN_ADDR, 8);
//  Serial.printf("requestFrom: %u\n", bytesReceived);
  // if ((bool)bytesReceived) 
  // {  //If received more than zero bytes
  //   Wire.readBytes(i2c_inputs.data_arr, bytesReceived);
  // }
 i2c_outputs.data = ~i2c_inputs.data;
  // Write message to the slave
  // Wire.beginTransmission(I2C_OUT_ADDR);
  // Wire.write(i2c_outputs.data_arr,8);
  // uint8_t error = Wire.endTransmission(true);

  // uint16_t t_x = 0, t_y = 0;  // To store the touch coordinates
  // TS_Point p_raw,p_coord;
  // Pressed will be set true is there is a valid touch on the screen
  // if (ts.tirqTouched() && ts.touched()) {
  //   TS_Point p = ts.getPoint();
  //   Serial.print("Pressure = ");
  //   Serial.print(p.z);
  //   Serial.print(", x = ");
  //   Serial.print(p.x);
  //   Serial.print(", y = ");
  //   Serial.print(p.y);
  //   Serial.println();

  // }

 if(i2c_inputs.data != i2c_inputs_tmp.data)
  {
     i_o_mask = i2c_inputs_tmp.data ^ i2c_inputs.data;
     i2c_inputs_tmp.data = i2c_inputs.data;
    for(int i=0;i<8;i++)
    {
      if(i2c_inputs.data_arr[0] & 1<<i)
      {
       inps[i].press(true);
       Serial.print("press");
       inps[i].setBackground(TFT_DARKGREEN,TFT_YELLOW);
      }
      else 
      {
        inps[i].press(false);
        inps[i].setBackground(TFT_GREEN,TFT_YELLOW);
      }  

      if(inps[i].justPressed())inps[i].setBackground(TFT_GREEN,TFT_YELLOW);  
      else if(inps[i].justReleased())inps[i].setBackground(TFT_DARKGREEN,TFT_YELLOW);
    }
  }

 if(i2c_outputs.data != i2c_outputs_tmp.data)
  {
    i_o_mask = i2c_outputs.data; 
    i2c_outputs_tmp.data = i2c_outputs.data;    
    for(int ii=0;ii<8;ii++)
    {
      if(i2c_outputs.data_arr[0] & 1<<ii)
      {
        outs[ii].press(true);//setBackground(TFT_YELLOW,TFT_BLACK);
      }
      else 
      {
        outs[ii].press(false);//setBackground(TFT_DARK_YELLOW,TFT_BLACK);
      }
    }
  }
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
void xpt2046_convert_raw_xy(uint16_t *x, uint16_t *y)
{
  uint16_t x_tmp = *x, y_tmp = *y, xx, yy;

  if(!touchCalibration_rotate){
    xx=(x_tmp-touchCalibration_x0)*_width/touchCalibration_x1;
    yy=(y_tmp-touchCalibration_y0)*_height/touchCalibration_y1;
    if(touchCalibration_invert_x)
      xx = _width - xx;
    if(touchCalibration_invert_y)
      yy = _height - yy;
  } else {
    xx=(y_tmp-touchCalibration_x0)*_width/touchCalibration_x1;
    yy=(x_tmp-touchCalibration_y0)*_height/touchCalibration_y1;
    if(touchCalibration_invert_x)
      xx = _width - xx;
    if(touchCalibration_invert_y)
      yy = _height - yy;
  }
  *x = xx;
  *y = yy;
}