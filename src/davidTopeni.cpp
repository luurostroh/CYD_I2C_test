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
#include <Arduino.h>
// #include "FS.h"
#include <SPI.h>
#include <TFT_eSPI.h>  // Hardware-specific library
#include "Wire.h"
#include "ESP32TimerInterrupt.h"
// #include <SPIFFS.h>
//touch panel
#include <XPT2046_Touchscreen.h>
//
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "webpage.h"
#include <string.h>
#include "Keypad.h"

#include <ElegantOTA.h>
#include <EEPROM.h>
/////////////////////////////////////////////////////////

// #define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
// #include "esp_log.h"
#define DEBUG

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

#define CERPADLO OUT1

//I2C
typedef union 
{
  uint64_t data;
  uint8_t data_arr[8];
}I2C_data_t;
I2C_data_t i2c_inputs={0};
I2C_data_t i2c_inputs_tmp={0};
I2C_data_t i2c_outputs={0};
I2C_data_t i2c_outputs_tmp={0};
uint8_t I2C_rec_data[26] = {0};
#define I2C_OUT_ADDR 60
#define I2C_IN_ADDR 56
#define I2C_ADDR 8
////

typedef struct
{
  int x_pos;
  int y_pos;
}tft_position_t;

tft_position_t temperature_labels_position[7]=
{
  {185,8},
  {185,41},
  {185,74},
  {185,107},
  {185,140},
  {185,173},
  {185,206}
};

tft_position_t cerpadlo_status_position = {185,254};

#define  eePasswPtr  0
#define   eeSsidPtr 25
#define  eeSwOnPtr  51
#define eeSwOffPtr  55
#define eeAlarmPtr  59

void EEwriteFloat(float val,uint16_t addr);
float EEreadFloat(uint16_t addr);

bool after_reset = true;
bool cerpadlo_jede = false;
bool is_alarm = false;
bool show_temp_enable = false;
float temperature_values[7]={0};
float tmp_temperature_values[7]={999.0,999.0,999.0,999.0,999.0,999.0,999.0};//nesmi byt 0 kvuli inicializaci
float switch_on_temp = 0.0F;
float switch_off_temp = 0.0F;
float alarm_temp = 0.0F;
bool client_connected;
void ChangedTempToPage(uint8_t temp_index);

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
#define LABEL3_FONT &FreeMonoBold18pt7b 
#define LABEL4_FONT &FreeMonoOblique24pt7b  
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
static uint16_t touchCalibration_x0 = 195, touchCalibration_x1 = 3600, touchCalibration_y0 = 229, touchCalibration_y1 = 3541;
static uint8_t  touchCalibration_rotate = 1, touchCalibration_invert_x = 1, touchCalibration_invert_y = 0;
static uint16_t _width = 240, _height = 320;

uint16_t keyColor[16] = { TFT_RED, TFT_DARKGREY, TFT_DARKGREEN,
                          TFT_BLUE, TFT_BLUE, TFT_BLUE,
                          TFT_BLUE, TFT_BLUE, TFT_BLUE,
                          TFT_BLUE, TFT_BLUE, TFT_BLUE,
                          TFT_BLUE, TFT_BLUE, TFT_BLUE };


SemaphoreHandle_t I2Csemaphore;

bool toggle[8] = {false};
bool external_receive = false;
void xpt2046_convert_raw_xy(uint16_t *x, uint16_t *y);
void drawI_O();
void R_W_i2c();
void mainPage();
void settingsPage();
void AlarmPage();
void thermostatPage();
void cerpadloPage();
void wifiPage();
void basePage(String page_name);
void touchHandler();
void returnBack();
void show_temperatures();
void touch_calibrate(); 
void thermostat();
void AlarmToPage(String s);
void CerpadloStatusToPage(String s);
void ThermostatToPage();
uint8_t crc8(const uint8_t *data, size_t len);
bool checkI2CrecDataCRC();
bool checkI2CrecDataChecksum();
void AlarmActivate();
void AlarmDeactivate();

 void FindSeparators(char *array,char *indexes,int len);
// void keypadAlphaH();
// void keypadAlphaL();
// void keypadNum();

Keypad keypad = Keypad();
uint16_t current_page;
uint16_t previous_page;
uint16_t current_button;
// void task_RW_I2C(void *parameter)
// {
//   int cnt;
//   while(1)
//   {
//      // xSemaphoreTake(I2Csemaphore, portMAX_DELAY);
//       vTaskDelay(1);
//       if(++cnt>10){cnt=0;Serial.println("i2c");}
      
//       R_W_i2c();
//   }

// }
//------------------------------------------------------------------------------------------

char labels[8][10] = {"vymenik", "stoupacka", "zpatecka", "u krbu", "mistnost","komin1", "komin2","cerpadlo"};

int backlight_timer=0;
bool backlight_off = false;
bool IRAM_ATTR TimerHandler0(void * timerNo)
{
 static portBASE_TYPE xHigherPriorityTaskWoken;
 xHigherPriorityTaskWoken = pdFALSE;
// xSemaphoreGiveFromISR(I2Csemaphore,(BaseType_t*)&xHigherPriorityTaskWoken);
  if(++backlight_timer>50000){backlight_timer=0;backlight_off = true;}
	return true;
}

ESP32Timer ITimer0(0);


AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
/////////////////
// File indexF;
// void handleRoot() {
//   indexF = SPIFFS.open("/index.html", "r");
//   if (!indexF) {
//     // server.send(404, "text/plain", "File Not Found");
//     printf("File Not Found\n");
//     return;
//   }
//   Serial.println("file index found");
//   String s = String(indexF);
//   Serial.println(s);
//   // server.streamFile(file, "text/html");
//   indexF.close();
// }




String ssid = "ETC12345";
String password = "heslotajne";

void setupWebServer()
{

    // Serve the root page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send_P(200, "text/html", idx);
    });
    ElegantOTA.begin(&server); // ElegantOTA update page
    // Start the server
    server.begin();



  // Route to load style.css file
  // server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
  //   request->send(SPIFFS, "/style.css", "text/css");
  // });

  // Endpoint to toggle outputs
  // server.on("/toggle", HTTP_GET, [](AsyncWebServerRequest *request) {
  //   if (request->hasParam("out")) {
  //     String outParam = request->getParam("out")->value();
  //     int out = outParam.toInt();
  //     if (out >= 0 && out < 8) {
  //       // toggle[pin] ^= 1;
  //       xSemaphoreTake(I2Csemaphore, portMAX_DELAY);
  //       i2c_outputs.data_arr[0] ^= 1 << out;
  //       xSemaphoreGive(I2Csemaphore);
  //       request->send(200, "text/plain", "Toggled out " + String(out));
  //     } else {
  //       request->send(400, "text/plain", "Invalid out number");
  //     }
  //   } else {
  //     request->send(400, "text/plain", "Missing pin parameter");
  //   }
  // });

  // Endpoint to read inputs
  // server.on("/inputs", HTTP_GET, [](AsyncWebServerRequest *request) {
  //   xSemaphoreTake(I2Csemaphore, portMAX_DELAY);
  //   String inputStates = String(i2c_inputs.data_arr[0], BIN);
  //   xSemaphoreGive(I2Csemaphore);
  //   request->send(200, "text/plain", "Input states: " + inputStates);
  // });

  // Start the server
  // server.begin();
}

// Initialize the web server in the placeholder

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
 {
  String debug_data = "";
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  String payload = "";
  char msg[5];
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) 
  {
    data[len] = 0;
    String rec_data = "";
    payload = (char *)data;
   
    debug_data += "ok\n";
    debug_data += payload;
    Serial.println(debug_data);

    if (payload[0] == 'r') 
    {
      if(payload[1]=='s')
      {
#ifdef DEBUG        
        Serial.println("wifi alrm deact");
#endif
        AlarmDeactivate();
      }

    }
    else if (payload[0] == 's') 
    {
          char msgArray[30]={0};
          char separator_indexes[10];
          char j;
          String tmpString ="";
          payload.toCharArray(msgArray,payload.length()+1);
          // FindSeparators(msgArray,separator_indexes,30);
            char c;
            j = 0;
            // Serial.print("FS>>");
            for (char i = 0; i < 30; i++) {
                c = msgArray[i];
                // Serial.print("-");
                // Serial.print(c);
                if (c == 0x3E /*'>'*/) {
                    separator_indexes[j] = i;
                    // Serial.print("&");
                    // Serial.print(j,10);
                    // Serial.print("%");
                    // Serial.print(i,10);
                    // Serial.print("%");
                    j++;
                } // = i; indexes++;
            }
            tmpString = payload.substring(separator_indexes[0] + 1, separator_indexes[1]); // payload.substring(4,8);
            // Serial.println(tmpString);
            switch_on_temp = tmpString.toFloat();
            // Serial.print("**");
            // Serial.print(switch_on_temp);
            tmpString = payload.substring(separator_indexes[1] + 1, separator_indexes[2]); // payload.substring(9,13);
            Serial.println(tmpString);
            switch_off_temp = tmpString.toFloat();
            // Serial.print("**");
            // Serial.print(switch_off_temp);
            tmpString = payload.substring(separator_indexes[2] + 1, separator_indexes[3]); // payload.substring(14,18);
            Serial.println(tmpString);
            alarm_temp = tmpString.toFloat();
            // Serial.print("**");
            // Serial.println(alarm_temp);

            EEwriteFloat(switch_on_temp, eeSwOnPtr); 
            EEwriteFloat(switch_off_temp, eeSwOffPtr); 
            EEwriteFloat(alarm_temp, eeAlarmPtr);
            EEPROM.commit();
    }
  }

 }

 void FindSeparators(char *array,char *indexes, int len)
 {
  char c;
  #ifdef DEBUG 
  Serial.print("FS>>");Serial.print(len);Serial.print("<<");
  #endif
  for(char i=0;i<len;i++)
  {
    c=*array++ ;
#ifdef DEBUG 
    Serial.print("-");Serial.print(c);Serial.print("-");
#endif
    if( c == 0x3E/*'>'*/){ 
      *indexes++ = i;
#ifdef DEBUG      
      Serial.print("%");Serial.print(i);Serial.print("%");
#endif
    }// = i; indexes++;}
  }
  Serial.println();
 }

void AlarmActivate()
{
    if (is_alarm == false) 
    {
        if(client_connected)AlarmToPage("on");
        is_alarm = true;
        i2c_outputs.data_arr[0] |= 2; // OUT2 on
        if (current_page == 1)AlarmPage();          
    }
}

void AlarmDeactivate()
{
    if (is_alarm == true) {
        if (temperature_values[0] < alarm_temp ) {
            is_alarm = false;
            i2c_outputs.data_arr[0] &= ~(1 << 1);
            if (client_connected)
                AlarmToPage("off");
            mainPage();
        }
    }
}

void DataToPage()
{
    char msg[150];
    int komin1 = (int) temperature_values[5];
    int komin2 = (int) temperature_values[6];
    sprintf(msg,"vymenik>%.1f>stoupacka>%.1f>zpatecka>%.1f>krb>%.1f>mistnost>%.1f>komin1>%u>komin2>%u>switchON>%.1f>switchOFF>%.1f>alarmTemp>%.1f>",                
            temperature_values[0],temperature_values[1],temperature_values[2],
            temperature_values[3],temperature_values[4],
            komin1,komin2,
            switch_on_temp, switch_off_temp, alarm_temp
          );
            
   ws.textAll(msg);

   if(is_alarm)AlarmToPage("on");
   if(cerpadlo_jede)CerpadloStatusToPage("on");

}
void ChangedTempToPage(uint8_t temp_index)
{
  char msg[25];
  if(temp_index < 5)
  {
    sprintf(msg,"%s>%.1f>",labels[temp_index],temperature_values[temp_index]);
  }
  else
  {
    int x = (int)temperature_values[temp_index];
    sprintf(msg,"%s>%u>",labels[temp_index],x);
  }
  
  ws.textAll(msg);
}

void ThermostatToPage()
{
   char msg[55];
   sprintf(msg, "switchON>%.1f>switchOFF>%.1f>alarmTemp>%.1f>", switch_on_temp, switch_off_temp, alarm_temp);
   ws.textAll(msg);
}

void AlarmToPage(String s)
{
  char msg[15];
  sprintf(msg,"alarm>%s>",s);
  ws.textAll(msg);
}

void CerpadloStatusToPage(String s)
{
  char msg[20];
  sprintf(msg,"cerpadlo>%s>",s);
  ws.textAll(msg);
}

 
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) 
{
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      client_connected = true;
      DataToPage();
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      client_connected = false;
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket()
{
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}


void InitWIFI()
{
   WiFi.mode(WIFI_STA);
   WiFi.setMinSecurity(WIFI_AUTH_WPA_PSK);
#ifdef DEBUG    
   Serial.println("start wifi");
#endif
   tft.setTextColor(TFT_YELLOW, TFT_BLACK);
   tft.setFreeFont(LABEL1_FONT);
   tft.drawString("cekam na pripojeni",20,180);
   char _ssid[15];
   char _password[15];
   ssid.toCharArray(_ssid,15);
   password.toCharArray(_password,15);
   WiFi.begin(_ssid, _password);

   if (WiFi.waitForConnectResult(5000) != WL_CONNECTED) 
   {
#ifdef DEBUG 
       Serial.printf("WiFi Failed!\n");
#endif     
      //  tft.drawRect(0,0,320,240,TFT_RED);
   } 
   else 
   {
#ifdef DEBUG   
       Serial.print("IP Address: ");
       Serial.println(WiFi.localIP());
#endif     
       setupWebServer();
       initWebSocket();
       ws.cleanupClients();
      //  tft.drawRect(0,0,320,240,TFT_GREEN);
   }
}
//////////

void setup() 
{

  // SPIFFS.begin();
  EEPROM.begin(100);
  Serial.begin(9600);
  char tmp[25]={' '};


  // Serial.print("pass>>");
  for(uint8_t i = 0;i<25;i++)
  {
    tmp[i] = EEPROM.read(i);
    // Serial.print(c,16);
    // Serial.print(" ");
  }
#ifdef DEBUG 
  Serial.print("pass>>");
  Serial.println(tmp);
#endif
  password = String(tmp);
  for(uint8_t i = 0;i<25;i++)
  {
    tmp[i] = EEPROM.read(i+25);
  }
#ifdef DEBUG 
  Serial.print("ssid>>");
  Serial.println(tmp);
#endif
  ssid = String(tmp);
  // uint16_t eeaddr = ;
  switch_on_temp = EEreadFloat(eeSwOnPtr);
  // eeaddr = ;
#ifdef DEBUG 
  Serial.print("T1>>");
  Serial.println(switch_on_temp);
#endif
  switch_off_temp = EEreadFloat(eeSwOffPtr);
#ifdef DEBUG 
  Serial.print("T2>>");
  Serial.println(switch_off_temp);
#endif
  // eeaddr = ;
  alarm_temp = EEreadFloat(eeAlarmPtr);
#ifdef DEBUG   
  Serial.print("T3>>");
  Serial.println(alarm_temp);
#endif  
  esp_log_level_set("*", ESP_LOG_ERROR);        // set all components to ERROR level
  i2c_inputs.data = 0xFFFFFFFFFFFFFFFF;
  i2c_inputs_tmp.data = 0xFFFFFFFFFFFFFFFF;
  i2c_outputs.data = 0;
  i2c_outputs_tmp.data = 0;
 // Use serial port
#ifdef DEBUG  
  Serial.println("\n--- Internal RAM (SRAM) ---");
  Serial.print("Total Heap: ");
  Serial.print(ESP.getHeapSize() / 1024);
  Serial.println(" KB");
  Serial.print("Free Heap: ");
  Serial.print(ESP.getFreeHeap() / 1024);
  Serial.println(" KB");
  Serial.print("Min. Free Heap: ");
  Serial.print(ESP.getMinFreeHeap() / 1024);
  Serial.println(" KB");
  Serial.print("Sketch size: ");
  Serial.print(ESP.getSketchSize() / 1024); // v KB
  Serial.println(" KB");
  Serial.print("Free Sketch Space: ");
  Serial.print(ESP.getFreeSketchSpace() / 1024); // v KB
  Serial.println(" KB");
  Serial.println("\n--- External PSRAM ---");
  if (psramFound()) { // Zkontroluje, zda je PSRAM přítomna
    Serial.print("Total PSRAM: ");
    Serial.print(ESP.getPsramSize() / (1024 * 1024)); // v MB
    Serial.println(" MB");
    Serial.print("Free PSRAM: ");
    Serial.print(ESP.getFreePsram() / (1024)); // v KB
    Serial.println(" KB");
  } else {
    Serial.println("PSRAM Not Found.");
  }
#endif  
//  Wire.setPins(27,22);

// handleRoot();
  // Start the SPI for the touch screen and init the TS library
  TP_Spi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin(TP_Spi);
  ts.setRotation(1);
  // Initialise the TFT screen
  tft.init();

  // Set the rotation before we calibrate
  tft.setRotation(0);
  InitWIFI();
  // Calibrate the touch screen and retrieve the scaling factors
  // touch_calibrate();
// touch_calibrate(); 
//   // Clear the screen
//  while(1);
  mainPage();
 

   

   Wire.begin(27, 22, 400000);
   // xTaskCreate(task_RW_I2C,"taskRW",10000,NULL,1,NULL);


   vSemaphoreCreateBinary(I2Csemaphore);
   ITimer0.attachInterruptInterval(10000, TimerHandler0);
   vTaskStartScheduler();

   //  mainPage();
}

//------------------------------------------------------------------------------------------

void loop(void) 
{
  
  uint8_t readI2Ccounter;
  int16_t t_x = 0, t_y = 0,t_x1 = 0, t_y1 = 0;  // To store the touch coordinates

  if (after_reset) 
  {
      after_reset = false;
      show_temp_enable = true;
      show_temperatures();
  }
   if(backlight_off)
   {
    show_temp_enable = false;
    digitalWrite(TFT_BL,0);
    tft.fillScreen(TFT_BLACK);
   } 
  //kontrola vystupu
   if(external_receive)
   {
    external_receive = false;
   }

    if (ts.tirqTouched() && ts.touched() || is_alarm)
    {
      if(backlight_off)
      {
        if(is_alarm)AlarmPage();
        else 
        {
          mainPage();
          show_temp_enable = true;
          show_temperatures();
        }
        backlight_off = false;
      }
      backlight_timer = 0;
      digitalWrite(TFT_BL,1);
      // Serial.println("OKok");
        touchHandler();
    }

 

    delay(100);
    // if(++readI2Ccounter > 5)
    // {
    //   Serial.println("I2C");
    //   readI2Ccounter = 0;
      R_W_i2c();
    // }
    
    thermostat();
  }



//------------------------------------------------------------------------------------------


TFT_eSPI_Button menuButton;
void mainPage()
{
  current_page = 1;
  previous_page = 1;
  
  tft.fillScreen(TFT_LIGHTGREY);
  if (WiFi.status() != WL_CONNECTED)
  {
    tft.drawRect(0,0,240,320,TFT_RED);
    tft.drawRect(1,1,238,318,TFT_RED);
  }
  else 
  {
    tft.drawRect(0,0,240,320,TFT_GREEN);
    tft.drawRect(1,1,238,318,TFT_GREEN);
  }
  tft.setFreeFont(LABEL1_FONT);
  menuButton.initButton(&tft, 120, 300, 150, 35, TFT_WHITE, TFT_WHITE, TFT_BLACK, "NASTAVENI", 1);
  menuButton.setLabelDatum(0,4,4);
  menuButton.drawButton();
  int xpos = 120, ypos =4;
  tft.setTextColor(TFT_BLACK,TFT_LIGHTGREY);
  for (int i = 0; i < 8; i++) 
  { 
    if(i<7)
    {
      tft.fillRect(temperature_labels_position[i].x_pos-30,temperature_labels_position[i].y_pos,60,26,TFT_WHITE);
    }
    tft.drawString(labels[i], xpos - 90, ypos + 8,4); // Draw the label outside the box to the left
    ypos += 33; // Move to the next position
  }
  if(cerpadlo_jede)tft.fillCircle(cerpadlo_status_position.x_pos,cerpadlo_status_position.y_pos,15,TFT_GREEN);
  else tft.fillCircle(cerpadlo_status_position.x_pos,cerpadlo_status_position.y_pos,15,TFT_DARKGREY);
  ypos =4;
  tft.setFreeFont(LABEL1_FONT);
  tft.setTextColor(TFT_BLACK,TFT_WHITE);
  for (int i = 0; i < 7; i++) 
  { 
    if(i<5)
    {
      tft.drawCentreString("0,0", temperature_labels_position[i].x_pos,temperature_labels_position[i].y_pos,4); 
    }
    else
    {
      tft.drawCentreString("0", temperature_labels_position[i].x_pos,temperature_labels_position[i].y_pos,4); 
    }
    
  }

  
}

TFT_eSPI_Button setButton[3];
void settingsPage()
{
  current_page = 2;
  basePage("NASTAVENI");
  tft.setFreeFont(LABEL2_FONT);
  setButton[0].setLabelDatum(0,4,4);
  setButton[0].initButton(&tft, 120, 100, 200, 40, TFT_WHITE, TFT_WHITE, TFT_BLACK, "TERMOSTAT", 1);
  setButton[0].drawButton();
  setButton[1].setLabelDatum(0,4,4);
  setButton[1].initButton(&tft, 120, 160, 200, 40, TFT_WHITE, TFT_WHITE, TFT_BLACK, "CERPADLO", 1);
  setButton[1].drawButton();
  setButton[2].setLabelDatum(0,4,4);
  setButton[2].initButton(&tft, 120, 220, 200, 40, TFT_WHITE, TFT_WHITE, TFT_BLACK, "WIFI", 1);
  setButton[2].drawButton();
}


TFT_eSPI_Button therstatButtonMin;
TFT_eSPI_Button therstatButtonMax;
TFT_eSPI_Button therstatButtonAlarm;
tft_position_t positionsTherstatLabels[3]={{150,70},{150,140},{150,210}};
void thermostatPage()
{
  current_page = 3;
  basePage("TERMOSTAT");
  tft.setTextColor(TFT_BLACK,TFT_LIGHTGREY);
  tft.drawString("ZAP",60,70);
  tft.drawString("VYP",60,140);
  tft.drawString("ALARM",30,210);
  therstatButtonMin.initButton(&tft, 180, 80, 100, 60, TFT_WHITE, TFT_WHITE, TFT_BLACK, "", 1);
  therstatButtonMin.drawButton();
  therstatButtonMax.initButton(&tft, 180, 150, 100, 60, TFT_WHITE, TFT_WHITE, TFT_BLACK, "", 1);
  therstatButtonMax.drawButton();
  therstatButtonAlarm.initButton(&tft, 180, 220, 100, 60, TFT_WHITE, TFT_WHITE, TFT_BLACK, "", 1);
  therstatButtonAlarm.drawButton();
  tft.drawString(String(switch_on_temp),positionsTherstatLabels[0].x_pos,positionsTherstatLabels[0].y_pos);
  tft.drawString(String(switch_off_temp),positionsTherstatLabels[1].x_pos,positionsTherstatLabels[1].y_pos);
  tft.drawString(String(alarm_temp),positionsTherstatLabels[2].x_pos,positionsTherstatLabels[2].y_pos);
}


TFT_eSPI_Button btnCerpManual;
TFT_eSPI_Button btnCerpAuto;
TFT_eSPI_Button btnCerpOn;
TFT_eSPI_Button btnCerpOff;
bool cerp_is_manual = false;

void cerpadloPage()
{
  current_page = 4;
  basePage("CERPADLO");
  btnCerpManual.initButton(&tft, 60, 120, 110, 60, TFT_WHITE, TFT_WHITE, TFT_BLACK, "MANUAL", 1);
  btnCerpManual.drawButton();
  btnCerpAuto.initButton(&tft, 180, 120, 110, 60, TFT_WHITE, TFT_WHITE, TFT_BLACK, "AUTO", 1);
  btnCerpAuto.drawButton();
  if(cerp_is_manual)
  {
    btnCerpOn.initButton(&tft, 60, 200, 80, 60, TFT_WHITE, TFT_WHITE, TFT_GREEN, "ON", 1);
    btnCerpOn.drawButton();
    btnCerpOff.initButton(&tft, 180, 200, 80, 60, TFT_WHITE, TFT_WHITE, TFT_RED, "OFF", 1);
    btnCerpOff.drawButton();
  }
}

TFT_eSPI_Button wifiButton[2];
void wifiPage()
{
  String _ipadr = "IP:" ;
  _ipadr += WiFi.localIP().toString().c_str();
  uint16_t xpos;
  previous_page = current_page;
  current_page = 5;
  basePage("WIFI");
  wifiButton[0].initButton(&tft, 120, 113, 240, 35, TFT_WHITE, TFT_WHITE, TFT_BLACK, "", 1);
  wifiButton[0].drawButton();
  wifiButton[1].initButton(&tft, 120, 213, 240, 35, TFT_WHITE, TFT_WHITE, TFT_BLACK, "", 1);
  wifiButton[1].drawButton();
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setFreeFont(LABEL3_FONT);
  xpos = (240 -(password.length() * 20))/2;
  tft.drawCentreString("heslo",120,60,3);
   tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawString(password,xpos,100);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString("ssid",120,160,3);
  xpos = (240 -(ssid.length() * 20))/2;
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawString(ssid,xpos,200);
  tft.setFreeFont(LABEL1_FONT);
  tft.setTextColor(TFT_BLUE, TFT_LIGHTGREY);
  tft.drawString(_ipadr,xpos-40,240);
}

TFT_eSPI_Button alarmButton;
void AlarmPage()
{
  previous_page = current_page;
  current_page = 6;
  tft.fillRect(10,80,220,150,TFT_RED);
  alarmButton.initButton(&tft, 120, 180, 180, 40, TFT_WHITE, TFT_WHITE, TFT_BLACK, "RESET", 1);
  alarmButton.drawButton();
  tft.setTextColor(TFT_WHITE, TFT_RED);
  tft.drawCentreString("ALARM",120,130,4);

}

TFT_eSPI_Button btnOk,btnBack;
void basePage(String page_name)
{
  uint16_t xpos;
  xpos = (240 -(page_name.length() * 20))/2;
  tft.fillScreen(TFT_LIGHTGREY);
  if (WiFi.status() != WL_CONNECTED)
  {
    tft.drawRect(0,0,240,320,TFT_RED);
    tft.drawRect(1,1,238,318,TFT_RED);
  }
  else 
  {
    tft.drawRect(0,0,240,320,TFT_GREEN);
    tft.drawRect(1,1,238,318,TFT_GREEN);
  }
  tft.setFreeFont(LABEL2_FONT);
  tft.setTextColor(TFT_BLACK,TFT_LIGHTGREY);
  tft.drawString(page_name,xpos,10);
  tft.setTextColor(TFT_BLACK,TFT_LIGHTGREY);
  btnBack.initButton(&tft, 65, 297, 75, 35, TFT_WHITE, TFT_BLACK, TFT_WHITE, "ZPET", 1);
  btnBack.setLabelDatum(0,4,4);
  btnBack.drawButton();
  btnOk.initButton(&tft, 185, 297, 75, 35, TFT_WHITE, TFT_BLACK, TFT_WHITE, "OK", 1);
  btnOk.setLabelDatum(0,4,4);
  btnOk.drawButton();
}

void R_W_i2c()
{
  uint64_t i_o_mask = 0;
  uint8_t bytesReceived = Wire.requestFrom(I2C_ADDR, 16); // Request 15 bytes from address I2C_ADDR
  for (size_t i = 0; i < bytesReceived; i++)
  {
    xSemaphoreTake(I2Csemaphore,portMAX_DELAY);
    I2C_rec_data[i] = Wire.read();
    xSemaphoreGive(I2Csemaphore);
  }
  bool crcOK = checkI2CrecDataChecksum();//checkI2CrecDataCRC();
  // Serial.print("crc OK  ");Serial.println(crcOK);
  if(crcOK)
       show_temperatures();

  uint8_t val = i2c_outputs.data_arr[0];//show_temp_enable = false;
  Wire.beginTransmission(8); // Start transmission to address 8
  Wire.write(val); // Write 8 bytes from i2c_outputs
  uint8_t error = Wire.endTransmission(true); // End transmission and send stop condition

}

// CRC8 calculation (polynomial 0x31, initial value 0x00)
uint8_t crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x7;
            else
                crc <<= 1;
        }
    }
    return crc;
}

// Simple checksum calculation: sum of bytes modulo 256
uint8_t checksum(const uint8_t *data, size_t len)
{
    uint16_t sum = 0;
    for (size_t i = 0; i < len; i++)
    {
        sum += data[i];
    }
    return (uint8_t)(sum & 0xFF);
}

// Check checksum for I2C_rec_data[0-24] vs I2C_rec_data[25]
bool checkI2CrecDataChecksum()
{
    uint8_t calc_sum = checksum(I2C_rec_data, 15);
    return (calc_sum == I2C_rec_data[15]);
}

// Check CRC8 for I2C_rec_data[0-14] vs I2C_rec_data[15]
bool checkI2CrecDataCRC()
{
    uint8_t calc_crc = crc8(I2C_rec_data, 15);
    return (calc_crc == I2C_rec_data[15]);
}




void show_temperatures()
{
    String s = "";
    uint8_t idx = 0;
    for (int i = 0; i < 14; i += 2) 
    {
      xSemaphoreTake(I2Csemaphore,portMAX_DELAY);
        temperature_values[idx] = (float)(I2C_rec_data[i + 1] + (I2C_rec_data[i + 2] * 256));
      xSemaphoreGive(I2Csemaphore);  
        if(idx <= 4  ) 
        {
          temperature_values[idx] /= 10.0;
          if(temperature_values[idx] >100.0 || temperature_values[idx] < 1.0){idx++;continue;}
        }
        // else
        // {
        //   if(temperature_values[idx] >800.0 || temperature_values[idx] < 1.0){idx++;continue;}
        // }
          // Serial.print(idx);
          // Serial.print("-");
          // Serial.println(temperature_values[idx]);
        if (tmp_temperature_values[idx] != temperature_values[idx] || show_temp_enable == true)//zmena musi byt mensi nez 0.5 stupne
        {
          tmp_temperature_values[idx] = temperature_values[idx];
          // Serial.print(idx);Serial.print("-");Serial.println(temperature_values[idx]);
          if(idx<5)
          {
            s = String(temperature_values[idx] , 1);
          }
          else
          {
            int x = (int)temperature_values[idx];
            s = String(x,DEC);
          }  
#ifdef DEBUG           
          Serial.print(idx);Serial.print(">");Serial.println(s);
#endif          
          if(current_page == 1)
              tft.drawCentreString(s, temperature_labels_position[idx].x_pos, temperature_labels_position[idx].y_pos, 4);
          if(client_connected)ChangedTempToPage(idx);
      }    
        idx++;
    }
    show_temp_enable = false;
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
#ifdef DEBUG 
    Serial.print("x1 = ");Serial.print(x1);
    Serial.print(", y1 = ");Serial.print(y1);
    Serial.print("x2 = ");Serial.print(x2);
    Serial.print(", y2 = ");Serial.println(y2);
    Serial.print("xCalM = ");Serial.print(xCalM);
    Serial.print(", xCalC = ");Serial.print(xCalC);
    Serial.print("yCalM = ");Serial.print(yCalM);
    Serial.print(", yCalC = ");Serial.println(yCalC);
#endif
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

void touchHandler()
{
  TS_Point p;
  uint16_t x_,y_;
  if(!ts.touched())return;
#ifdef DEBUG 
  Serial.print("t");
#endif  
  //  char msg[5];
   p = ts.getPoint();
   x_ = p.x;
   y_ = p.y;
   xpt2046_convert_raw_xy(&x_, &y_);
   if (previous_page > 0) 
   {
    // Serial.println("OK");
       
       if (current_page == 1) //main page
       {
           
           if (menuButton.contains(x_, y_)) 
           {
            previous_page = 1;
            settingsPage();
           }
              
       } 
       else if (current_page == 2) //settings page
       {
           if (btnOk.contains(x_, y_)) 
           {
               previous_page = 1;
               returnBack();
               
           } 
           else if (btnBack.contains(x_, y_))
           {
              previous_page = 1;
              returnBack();
           }
           else if(setButton[0].contains(x_, y_))
           {
              previous_page = 2;
              thermostatPage();
           }
           else if(setButton[1].contains(x_, y_))
           {
              previous_page = 2;
              cerpadloPage();
           }
           else if(setButton[2].contains(x_, y_))
           {
              previous_page = 2;
              wifiPage();
           }    
       } 
 
       else if (current_page == 3) //termostat page
       {
        
        if(therstatButtonMin.contains(x_, y_))
        {
           keypad.Init();
           keypad.setText(String(switch_on_temp));
           keypad.drawKeypad(Keypad::NUMERIC,current_page);//0 = NUMERIC
           current_page = 7;
           current_button = 0;
           
        }
        
        if(therstatButtonMax.contains(x_, y_))
        {
           keypad.Init();
           keypad.setText(String(switch_off_temp));
           keypad.drawKeypad(Keypad::NUMERIC,current_page);//0 = NUMERIC
           current_page = 7;
           current_button = 1;
           
        }       
        
        if(therstatButtonAlarm.contains(x_, y_))
        {
           keypad.Init();
           keypad.setText(String(alarm_temp));
           keypad.drawKeypad(Keypad::NUMERIC,current_page);//0 = NUMERIC
           current_page = 7;
           current_button = 2;
        }        
        if (btnOk.contains(x_, y_)) 
        {
 
            EEwriteFloat(switch_on_temp, eeSwOnPtr); 
            EEwriteFloat(switch_off_temp, eeSwOffPtr); 
            EEwriteFloat(alarm_temp, eeAlarmPtr);
            EEPROM.commit();

            ThermostatToPage();
            previous_page = 2;
            returnBack();
        } 
        else if (btnBack.contains(x_, y_))
        {
           previous_page = 2;
           returnBack();
        }
       } 
       else if (current_page == 4) //cerpadlo page
       {
        if (btnCerpManual.contains(x_, y_)) 
        {
          cerp_is_manual = true;
          cerpadloPage();

        }
        else if (btnCerpAuto.contains(x_, y_)) 
        {
          cerp_is_manual = false;
          cerpadloPage();
        }
        else if (btnCerpOn.contains(x_, y_)) 
        {

        }
        else if (btnCerpOff.contains(x_, y_)) 
        {

        }                
        else if (btnOk.contains(x_, y_)) 
        {
            previous_page = 2;
            returnBack();
        } 
        else if (btnBack.contains(x_, y_))
        {
           previous_page = 2;
           returnBack();
        }
       } 
       else if (current_page == 5) //wifi page
       {

                if(wifiButton[0].contains(x_, y_))
        {
           keypad.Init();
           keypad.setText(password);
           keypad.drawKeypad(Keypad::ALPHA_L,current_page);//0 = NUMERIC
           current_page = 7;
           current_button = 0;

        }
        else if(wifiButton[1].contains(x_, y_))
        {
           keypad.Init();
           keypad.setText(ssid);
           keypad.drawKeypad(Keypad::ALPHA_L,current_page);//0 = NUMERIC
           current_page = 7;
           current_button = 1;

        }
        if (btnOk.contains(x_, y_)) 
        {
          // Serial.println("save1");
            char _txt[20];
            uint8_t len = password.length();          
            password.toCharArray(_txt,len+1);
            for(uint8_t i = 0; i<=len; i++)
            {
#ifdef DEBUG               
              Serial.print(_txt[i]);
#endif              
              EEPROM.write(i,_txt[i]);
            }
            for(uint8_t i = len; i<25; i++){EEPROM.write(i,0x0);}
#ifdef DEBUG             
            Serial.println();
#endif            
            len = ssid.length();          
            ssid.toCharArray(_txt,len+1);
            for(uint8_t i = 0; i<=len; i++)
            {
#ifdef DEBUG               
              Serial.print(_txt[i]);
#endif              
              EEPROM.write(i+25,_txt[i]);            
            }
            for(uint8_t i = len+25; i<50; i++){EEPROM.write(i,0x0);}
            EEPROM.commit();
            previous_page = 2;
            InitWIFI();
            returnBack();
        } 
        else if (btnBack.contains(x_, y_))
        {
           previous_page = 2;
           returnBack();
        }
       } 
       else if (current_page == 6) //alarm page
       {

        if(is_alarm && alarmButton.contains(x_, y_))
        {
#ifdef DEBUG           
          Serial.println("btn alrm");
#endif
          if(temperature_values[0] < alarm_temp )
          {  
#ifdef DEBUG             
            Serial.println("btn alrm deact"); 
#endif
            AlarmDeactivate();
            previous_page = 1;
            returnBack();
          }
        }

       }
       else if (current_page == 7) 
       {
          if(keypad.btnBack.contains(x_, y_))
          {
             previous_page = keypad.getPrevPage();
             returnBack();
          }
          else if(keypad.btnOk.contains(x_, y_))
          {
            previous_page = keypad.getPrevPage();
            if(previous_page == 3)
            {
             switch(current_button)
             {
              case 0: switch_on_temp = keypad.getText().toFloat();break;
              case 1: switch_off_temp = keypad.getText().toFloat();break;
              case 2: alarm_temp = keypad.getText().toFloat();break;
             }
            }

            else if(previous_page == 4)
            {

            }
            else if(previous_page == 5)
            {
             switch(current_button)
             {
              case 0: password = keypad.getText();break;
              case 1: ssid = keypad.getText();break;
             }
            }
             
             returnBack();
          }
          else keypad.touchHandle(x_, y_);
       }


   } 
   else 
   {
      
   }

   delay(100);
}

void returnBack()
{
  switch(previous_page)
  {
    case 1: mainPage(); show_temp_enable = true;show_temperatures(); break;
    case 2: settingsPage(); break;
    // case 3: alarmsPage(); break;
    case 3: thermostatPage(); break;
    case 4: cerpadloPage(); break;
    case 5: wifiPage(); break;
    default : mainPage(); break;

  }
}


void thermostat()
{
  if(temperature_values[0] >= alarm_temp ) AlarmActivate();

  if(temperature_values[0] >= switch_on_temp )
  {
    i2c_outputs.data_arr[0] |=1;//OUT1 on
    if(client_connected  && cerpadlo_jede == false) 
        CerpadloStatusToPage("on"); 
    cerpadlo_jede = true;

    if(current_page == 1)
        tft.fillCircle(cerpadlo_status_position.x_pos,cerpadlo_status_position.y_pos,15,TFT_GREEN);
  
  }
  else if(temperature_values[0] <= switch_off_temp )
  {
    i2c_outputs.data_arr[0] &= ~(1);//OUT1 off
    if(client_connected && cerpadlo_jede == true) 
        CerpadloStatusToPage("off");
    cerpadlo_jede = false;
    
    if(current_page == 1)
        tft.fillCircle(cerpadlo_status_position.x_pos,cerpadlo_status_position.y_pos,15,TFT_DARKGREY);
  }

}

void EEwriteFloat(float val,uint16_t addr)
{
  uint8_t _u8 =0;
  uint32_t _u32 = 0;
  _u32 = (uint32_t)(val * 10);
  _u8 = _u32;
  //  Serial.print("ee>>");
  // Serial.print(_u8);
  // Serial.print(" ");
  EEPROM.write(addr,_u8);
  _u32 >>=8;
  _u8 = _u32;
  // Serial.print(_u8);
  // Serial.print(" ");
  EEPROM.write(addr+1,_u8);
  _u32 >>=8;
  _u8 = _u32;
  // Serial.print(_u8);
  // Serial.print(" ");
  EEPROM.write(addr+2,_u8);
  _u32 >>=8;
  _u8 =_u32;
  // Serial.print(_u8);
  // Serial.print(" ");
  EEPROM.write(addr+3,_u8);
  
}

float EEreadFloat(uint16_t addr)
{ 
   uint8_t _u8 =0;
   uint32_t _u32 =0;
   _u32 = EEPROM.read(addr+3);
   _u32 <<=8;
   _u32 |= EEPROM.read(addr+2);
   _u32 <<=8;
   _u32 |= EEPROM.read(addr+1);
   _u32 <<=8;
   _u32 |= EEPROM.read(addr);


// Serial.print("readEE>>");Serial.println(_u32);
   return (float) (_u32/10);

}