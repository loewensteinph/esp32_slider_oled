#include <Arduino.h>
#include <menu.h>
#include <menuIO/u8g2Out.h>
#include <menuIO/chainStream.h>
#include <menuIO/rotaryEventIn.h>
#include <mytask.h>
#include "soc/rtc_wdt.h"
#include "esp_int_wdt.h"
#include "esp_task_wdt.h"
// some example libraries to handle the rotation and clicky part
// of the encoder. These will generate our events.
#include <qdec.h> //https://github.com/SimpleHacks/QDEC
#include <AceButton.h> // https://github.com/bxparks/AceButton

static portMUX_TYPE my_mutex;

volatile int progress = 0;
TaskHandle_t  Core0TaskHnd ;  
TaskHandle_t  Core1TaskHnd ; 

Pan pan;

#define LEDPIN 2

bool menuIdle = false;

// Encoder
const int ROTARY_PIN_BUT  = 27;
const int16_t ROTARY_PIN_A = 25; // the first pin connected to the rotary encoder
const int16_t ROTARY_PIN_B = 26; // the second pin connected to the rotary encoder

using namespace ::ace_button;
using namespace ::SimpleHacks;
QDecoder qdec(ROTARY_PIN_A, ROTARY_PIN_B, true); // rotary part
AceButton button(ROTARY_PIN_BUT); // button part
//--//

// Display
// LOLIN32 I2C SSD1306 128x64 display
// https://github.com/olikraus/u8g2

#include <Wire.h>
#define fontName u8g2_font_7x13_mf
#define fontX 7
#define fontY 16
#define offsetX 0
#define offsetY 3
#define U8_Width 128
#define U8_Height 64
#define USE_HWI2C
#define fontMarginX 2
#define fontMarginY 2
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);;//allow contrast change

const colorDef<uint8_t> colors[6] MEMMODE={
  {{0,0},{0,1,1}},//bgColor
  {{1,1},{1,0,0}},//fgColor
  {{1,1},{1,0,0}},//valColor
  {{1,1},{1,0,0}},//unitColor
  {{0,1},{0,0,1}},//cursorColor
  {{1,1},{1,0,0}},//titleColor
};
//--//


// AndroidMenu 
// https://github.com/neu-rah/ArduinoMenu
#define MAX_DEPTH 2

int exitMenuOptions = 0; //Forces the menu to exit and cut the copper tape

result inputTravelDist(eventMask e,navNode& nav, prompt &item) {
  pan.recalcFigures();
  float enteredInterval = pan.interval;
  if(enteredInterval<pan.minInterval)
  {
    // TODO: Handle GUI give feedback
     Serial.print("inputTravelDist event: too fast!"); 
     return proceed;
  }
  Serial.print("inputTravelDist event:");
  Serial.println(pan.travelDist);
  Serial.flush();
  return proceed;
}

result resRunPan() {
  delay(500);
  exitMenuOptions = 1;
  return proceed;
}

result idle(menuOut& o,idleEvent e) {
  switch(e) {
    case idleStart:o.print("suspending menu!");break;
    case idling:o.print("suspended...");break;
    case idleEnd:o.print("resuming menu.");break;
  }
  return proceed;
}

result drawPan(menuOut& o, idleEvent e) {
  u8g2.clearBuffer();
  while (exitMenuOptions == 1) {
    u8g2.setFont(fontName);
    u8g2.drawStr(35, 13, "Running Pan");
    u8g2.drawStr(33, 32, String(progress).c_str());
    u8g2.drawStr(60, 32, "% done");
    u8g2.sendBuffer();
    delay(100);
  }  
  if (e == idling) {
    o.setCursor(0, 0);
    o.print("Pan done");
    o.setCursor(0, 1);
    o.print("press [select]");
    o.setCursor(0, 2);
    o.print("to continue...");
    return proceed;
  }
  return proceed;
    //mainMenu.dirty = true; // Return to the menu
}

void runPanWorkload() {
    menuIdle = true;
    //pan.travelTime = 10;
    //pan.travelDist = 200;
    digitalWrite(pan.travDirPin, HIGH); // Enables the motor to move in a particular direction
    //Serial.println("direction pin HIGH");
    pan.recalcFigures();
    pan.enableMotors();
    while (progress <= 100) {
        pan.executeChunk();
        progress += 1; // for demonstration only
    }
    progress = 0;
    exitMenuOptions = 0; // Return to the menu
    menuIdle = false;
}
result doPan(eventMask e, prompt &item);


MENU(mainMenu, "Slider Menu", doNothing, anyEvent, wrapStyle
  ,FIELD(pan.travelDist,"Distance","mm",pan.minTravelDist,pan.maxTravelDist,pan.travelDistInc,pan.travelDistIncFine, inputTravelDist, enterEvent , noStyle)  
  ,FIELD(pan.travelTime,"Time","s",1,3600,1,1, inputTravelDist, enterEvent , noStyle)  
  ,OP("Pan test",doPan,enterEvent)
  ,EXIT("<Back")
);

RotaryEventIn reIn(
  RotaryEventIn::EventType::BUTTON_CLICKED | // select
  RotaryEventIn::EventType::BUTTON_DOUBLE_CLICKED | // back
  RotaryEventIn::EventType::BUTTON_LONG_PRESSED | // also back
  RotaryEventIn::EventType::ROTARY_CCW | // up
  RotaryEventIn::EventType::ROTARY_CW // down
); // register capabilities, see AndroidMenu MenuIO/RotaryEventIn.h file
MENU_INPUTS(in,&reIn);

MENU_OUTPUTS(out,MAX_DEPTH
  ,U8G2_OUT(u8g2,colors,fontX,fontY,offsetX,offsetY,{0,0,U8_Width/fontX,U8_Height/fontY})
  ,NONE
);

NAVROOT(nav,mainMenu,MAX_DEPTH,in,out);

result doPan(eventMask e, prompt &item) {
  exitMenuOptions = 1;
  nav.idleOn(drawPan);
  return proceed;
}

// This is the ISR (interrupt service routine) for rotary events
// We will convert/relay events to the RotaryEventIn object
// Callback config in setup()
void IRAM_ATTR IsrForQDEC(void) { 
  QDECODER_EVENT event = qdec.update();
  if (event & QDECODER_EVENT_CW) { reIn.registerEvent(RotaryEventIn::EventType::ROTARY_CW); }
  else if (event & QDECODER_EVENT_CCW) { reIn.registerEvent(RotaryEventIn::EventType::ROTARY_CCW); }

}

// This is the handler/callback for button events
// We will convert/relay events to the RotaryEventIn object
// Callback config in setup()
void handleButtonEvent(AceButton* /* button */, uint8_t eventType, uint8_t buttonState) {
  switch (eventType) {
    case AceButton::kEventClicked:
      reIn.registerEvent(RotaryEventIn::EventType::BUTTON_CLICKED);
      
      break;
    case AceButton::kEventDoubleClicked:
      reIn.registerEvent(RotaryEventIn::EventType::BUTTON_DOUBLE_CLICKED);
      break;
    case AceButton::kEventLongPressed:
      reIn.registerEvent(RotaryEventIn::EventType::BUTTON_LONG_PRESSED);
      break;
  }
}

bool initDone = false;

void initMenu() {
    // setup rotary encoder
  qdec.begin();
  attachInterrupt(digitalPinToInterrupt(ROTARY_PIN_A), IsrForQDEC, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ROTARY_PIN_B), IsrForQDEC, CHANGE);

  // setup rotary button
  pinMode(ROTARY_PIN_BUT, INPUT);
  ButtonConfig* buttonConfig = button.getButtonConfig();
  buttonConfig->setEventHandler(handleButtonEvent);
  buttonConfig->setFeature(ButtonConfig::kFeatureClick);
  buttonConfig->setFeature(ButtonConfig::kFeatureDoubleClick);
  buttonConfig->setFeature(ButtonConfig::kFeatureLongPress);
  buttonConfig->setFeature(ButtonConfig::kFeatureSuppressClickBeforeDoubleClick);
  buttonConfig->setFeature(ButtonConfig::kFeatureSuppressAfterClick);
  buttonConfig->setFeature(ButtonConfig::kFeatureSuppressAfterDoubleClick);

  nav.idleTask=idle;//point a function to be used when menu is suspended
  // setup OLED disaply
  Wire.begin(SDA,SCL);
  u8g2.begin();
  u8g2.setFont(fontName);
  //appear
  for(int c=255;c>0;c--) {
    u8g2.setContrast(255-255.0*log(c)/log(255));
    delay(12);
  }
}

void CoreTask0( void * parameter ) 
{ 
  if (initDone!=true)
  {
    initMenu();
    initDone=true;
  }
  for (;;) 
  { 
    //Serial.print("CoreTask0 runs on Core: "); 
    //Serial.println(xPortGetCoreID()); 
    switch (exitMenuOptions) {
      case 1: {
          delay(500); // Pause to allow the button to come up
          menuIdle = true;
          //runPanDisplay();
          break;
        }
      case 2: {
          delay(500); // Pause to allow the button to come up
          //runRotate();
          break;
        }
      default: // Do the normal program functions with ArduinoMenu
      {
          menuIdle = false;
          button.check(); // acebutton check, rotary is on ISR
          nav.doInput(); // menu check
      }
  }
   if (nav.changed(0)) {//only draw if menu changed for gfx device
    u8g2.firstPage();
    do nav.doOutput(); while(u8g2.nextPage());
  }
    yield();
    delay(40); // equals to 25FPS
  } 
} 

void setup()
{
  rtc_wdt_protect_off();
  rtc_wdt_disable();
  disableCore0WDT();
  disableLoopWDT();
  esp_task_wdt_delete(NULL);
  //vPortCPUInitializeMutex(&my_mutex);
  Serial.begin(115200);

  xTaskCreatePinnedToCore(CoreTask0,"CPU_0",2000,NULL,1,&Core0TaskHnd,0);

  pinMode(pan.travStepPin, OUTPUT);
  pinMode(pan.travDirPin, OUTPUT);
  pinMode(pan.enablePin, OUTPUT);
  digitalWrite(pan.enablePin, LOW);
  delay(500);
}

void loop()
{
  switch (exitMenuOptions) {
      case 1: {
          delay(500); // Pause to allow the button to come up
          runPanWorkload();
          //Serial.print ("Work Todo!!!!:");
          break;
        }
      case 2: {
          delay(40); // Pause to allow the button to come up
          //runRotate();
          break;
        }
      default: // Do the normal program functions with ArduinoMenu
      {
          //Serial.print ("Application CPU is on core:");
          //Serial.println (xPortGetCoreID());
          delay(400);
          break;
      }
  }
  delay(40);
}
