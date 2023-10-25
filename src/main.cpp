#include <Arduino.h>
#include <menu.h>
#include <menuIO/u8g2Out.h>
#include <menuIO/chainStream.h>
#include <menuIO/rotaryEventIn.h>
#include "soc/rtc_wdt.h"
#include "esp_int_wdt.h"
#include "esp_task_wdt.h"
// some example libraries to handle the rotation and clicky part
// of the encoder. These will generate our events.
#include <qdec.h> //https://github.com/SimpleHacks/QDEC
#include <AceButton.h> // https://github.com/bxparks/AceButton
#include "FastAccelStepper.h"
#include <Wire.h>
#include <mytask.h>

double progress = 0;
TaskHandle_t  Core0TaskHnd;  
static portMUX_TYPE my_mutex;

#define LEDPIN 2
Pan pan;
Rot rot;
Job job;

bool menuIdle = false;

const int ROTARY_PIN_BUT  = 27;
const int16_t ROTARY_PIN_A = 25; // the first pin connected to the rotary encoder
const int16_t ROTARY_PIN_B = 26; // the second pin connected to the rotary encoder


using namespace ::ace_button;
using namespace ::SimpleHacks;
QDecoder qdec(ROTARY_PIN_A, ROTARY_PIN_B, true); // rotary part
AceButton button(ROTARY_PIN_BUT); // button part


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
int testval = 0;

// AndroidMenu 
// https://github.com/neu-rah/ArduinoMenu
#define MAX_DEPTH 2

int exitMenuOptions = 0; //Forces no WorkloadTask active per Default per
bool tooFast;

result doPan(eventMask e, prompt &item);
result doRotate(eventMask e, prompt &item);
result doPanRotate(eventMask e, prompt &item);
result inputChangeHandler(menuOut& o,eventMask e,navNode& nav, prompt &item);
result disableMenuItem(eventMask e,navNode& nav,prompt& item);
result testHandler(menuOut& o,eventMask e,navNode& nav, prompt &item);

SELECT(job.pan.travelDir,dirPanMenu,"Trav Dir",doNothing,noEvent,wrapStyle
  ,VALUE(">",0,inputChangeHandler,enterEvent)
  ,VALUE("<",1,inputChangeHandler,enterEvent)
);

SELECT(job.rot.rotationDir,dirRotMenu,"Rot Dir",doNothing,noEvent,wrapStyle
  ,VALUE(">",0,inputChangeHandler,enterEvent)
  ,VALUE("<",1,inputChangeHandler,enterEvent)
);

const char* constMEM dummyMask[] MEMMODE={""};
char panSpeedCheck[] = " ";
char rotSpeedCheck[] = " ";

MENU(mainMenu, "Slider Menu", disableMenuItem, anyEvent, wrapStyle
  ,FIELD(testval,"Test","1",1,3600,1,1, testHandler, enterEvent, noStyle)
  ,FIELD(job.pan.travelDist,"Distance","mm",job.pan.minTravelDist,job.pan.maxTravelDist,job.pan.travelDistInc,job.pan.travelDistIncFine, inputChangeHandler, enterEvent , noStyle)  
  ,FIELD(job.pan.travelTime,"Time","s",1,3600,1,1, inputChangeHandler, enterEvent , noStyle)
  ,EDIT("Speed", panSpeedCheck, dummyMask, doNothing, noEvent, noStyle)
  ,SUBMENU(dirPanMenu)
  ,OP("Pan test",doPan,enterEvent)  
  ,FIELD(job.rot.rotationAngle,"Rot Ang","\xb0",1,360,5,1, inputChangeHandler, enterEvent , noStyle)
  ,FIELD(job.rot.pulsesPerDeg,"PPerDeg","1",5.0,80.0,0.1,0.01, inputChangeHandler, enterEvent, noStyle)
  ,OP("Rot test",doRotate,enterEvent)  
  ,EDIT("Speed", rotSpeedCheck, dummyMask, doNothing, noEvent, noStyle)
  ,SUBMENU(dirRotMenu)
  //,FIELD(pan.interval,"Interval","s",1,3600,1,1, readOnly, anyEvent , noStyle)
  ,OP("PanRot test",doPanRotate,enterEvent)    
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

result disableMenuItem(eventMask e,navNode& nav,prompt& item) {
  if(nav.selected().getText()==std::string("Speed"))
  {
    mainMenu.dirty = true;
    nav.selected().disable();
  }
  return proceed;
}

result inputChangeHandler(menuOut& o,eventMask e,navNode& nav, prompt &item) {
  //apply configured time to rotation could be configured individual with additional menu entry
  job.rot.rotationTime = job.pan.travelTime;
  job.recalcFigures();
  if(job.pan.tooFast)
  {
      strcpy(panSpeedCheck, "!");
      return proceed;
      mainMenu.dirty = true;
  }
  else
  {
      strcpy(panSpeedCheck, " ");
      return proceed;
  }
  if(job.rot.tooFast)
  {
      strcpy(rotSpeedCheck, "!");
      return proceed;
      mainMenu.dirty = true;
  }
  else
  {
      strcpy(rotSpeedCheck, " ");
      return proceed;
  }
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
result drawJob(menuOut& o, idleEvent e) {  
  u8g2.clearBuffer();
  if(tooFast)
  {
    menuIdle = true;
    exitMenuOptions == 0;
    o.setCursor(0, 0);
    o.print("Too fast");
    o.setCursor(0, 1);
    o.print("press [select]");
    o.setCursor(0, 2);
    o.print("to continue...");
    Serial.print("Pan event: too fast!");     
    return proceed;
  }
  u8g2.setFont(fontName);
  u8g2.drawStr(35, 13, job.jobToStr(job.jobtype));    

  while (exitMenuOptions == 1 || job.panStepper->isRunning() || job.rotStepper->isRunning()) {
  Serial.print("progress: "); 
  Serial.println(job.jobProgress);     
    u8g2.drawStr(33, 32, String(progress).c_str());
    u8g2.drawStr(60, 32, "% done");
    u8g2.sendBuffer();
    delay(100);
  }
  while (exitMenuOptions == 2 || job.panStepper->isRunning() || job.rotStepper->isRunning()) {
    u8g2.drawStr(33, 32, String(progress).c_str());
    u8g2.drawStr(60, 32, "% done");
    u8g2.sendBuffer();
    delay(100);
  }
  while (exitMenuOptions == 3 || job.panStepper->isRunning() || job.rotStepper->isRunning()) {
    u8g2.drawStr(33, 32, String(progress).c_str());
    u8g2.drawStr(60, 32, "% done");
    u8g2.sendBuffer();
    delay(100);
  }
  if (e == idling) {
    o.setCursor(0, 1);
    o.print("done");    
    o.setCursor(0, 2);
    o.print("press [select]");
    o.setCursor(0, 3);
    o.print("to continue...");
    //job.pan.disableMotors();   
    progress = 0;
    return proceed;
  }
  return proceed;
}

result doPan(eventMask e, prompt &item) {
  job.jobtype = job.doPan;
  if (!job.pan.tooFast) //prevent workload if too fast
  {
    exitMenuOptions = 1;
  }
  nav.idleOn(drawJob);
  return proceed;
}

result doRotate(eventMask e, prompt &item) {
  Serial.print("rotationAngle: "); 
  Serial.println(job.rot.rotationAngle); 
  Serial.print("rotInterval: "); 
  //Serial.println(job.rot.rotationInterval);
  Serial.print("too Fast: "); 
  Serial.println(job.rot.tooFast);     
  job.jobtype = job.doRotate;
  if (!job.rot.tooFast) //prevent workload if too fast
  {
    exitMenuOptions = 2;
  }
  nav.idleOn(drawJob);
  return proceed;
}

result doPanRotate(eventMask e, prompt &item) {
  Serial.print("rotationAngle: "); 
  Serial.println(job.rot.rotationAngle);   
  job.jobtype = job.doPanRotate;  
  if (!job.pan.tooFast && !job.rot.tooFast) //prevent workload if too fast
  {
    exitMenuOptions = 3;
  }
  nav.idleOn(drawJob);
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

  //mainMenu[2].disable();
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
          menuIdle = true;
          //runRotate();
          break;
        }
      case 3: {
          delay(500); // Pause to allow the button to come up
          menuIdle = true;
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
//// BEGIN ORIGINAL

#define dirPinStepper    17
#define enablePinStepper 19
#define stepPinStepper   16
//#define rotStepPin   18
#define rotDirPin   5

FastAccelStepperEngine engine;
FastAccelStepper *stepper = NULL;
FastAccelStepper *stepper2 = NULL;


void setup() {
  Serial.begin(115200);
  xTaskCreatePinnedToCore(CoreTask0,"CPU_0",2000,NULL,1,&Core0TaskHnd,0);
  Serial.println("START");
  engine.init();
  job.panStepper = engine.stepperConnectToPin(job.pan.travStepPin, DRIVER_RMT);
  job.rotStepper = engine.stepperConnectToPin(job.rot.rotStepPin, DRIVER_RMT);

  if (job.panStepper) {
    Serial.println("HAVE STEPPER");
    job.panStepper->setDirectionPin(dirPinStepper);
    job.panStepper->setEnablePin(enablePinStepper);
    job.panStepper->setAutoEnable(true);

    job.rotStepper->setDirectionPin(rotDirPin);
    job.rotStepper->setEnablePin(enablePinStepper);
    job.rotStepper->setAutoEnable(true);
  } else {
    while (true) {
      Serial.println("NO STEPPER");
      delay(1000);
    }
  }
}
// TODO: move to mytask
unsigned long jobStart;  //some global variables available anywhere in the program
unsigned long jobDuaration;
unsigned long jobPassedTime;

void runRotateWorkload()
{
  job.recalcFigures();
  jobStart = millis();
  jobDuaration = job.pan.travelTime*1000;

  while (millis() < jobStart + jobDuaration){ //(progress <= 100) {
      jobPassedTime = millis() - jobStart;
      job.executeRotateChunk(progress);
      progress = ((double)jobPassedTime / (double)jobDuaration)*100;
  }
  exitMenuOptions = 0; // Return to the menu
}

void runPanWorkload()
{
  job.recalcFigures();
  jobStart = millis();
  jobDuaration = job.pan.travelTime*1000;

  while (millis() < jobStart + jobDuaration){ //(progress <= 100) {
      jobPassedTime = millis() - jobStart;
      job.executePanChunk(progress);
      progress = ((double)jobPassedTime / (double)jobDuaration)*100;
  }
  exitMenuOptions = 0; // Return to the menu
}

void runPanRotateWorkload()
{
  job.recalcFigures();
  jobStart = millis();
  jobDuaration = job.pan.travelTime*1000;

  while (millis() < jobStart + jobDuaration){ //(progress <= 100) {
      jobPassedTime = millis() - jobStart;
      job.executePanRotateChunk(progress);
      progress = ((double)jobPassedTime / (double)jobDuaration)*100;
  }
  exitMenuOptions = 0; // Return to the menu
}

int objectDistance = 1200;
int remainingDistance;
double rotCurrentAngle;
double previousCurrentAngle;
double deltaAngle;
const double pi = std::atan(1.0)*4;

void runTest()
{
  job.recalcFigures();
  
  jobStart = millis();
  jobDuaration = job.pan.travelTime*1000;

  while (millis() < jobStart + jobDuaration){ //(millis() < jobStart + jobDuaration){ //(progress <= 100) {
        
        jobPassedTime = millis() - jobStart;
        job.panStepper->moveTo(pan.panTargetPos);///100*progress); 

        if(job.panStepper->getCurrentPosition()<(job.pan.panTargetPos/2))
        {
          //Serial.println("before half:->");
          //Serial.println(job.panStepper->getCurrentPosition());
          rotCurrentAngle = atan2(objectDistance,job.pan.travelDist/2) * 180 / pi;
          job.rot.rotationAngle = rotCurrentAngle;
          job.rot.recalcFigures();
          //Serial.println("<-before half");                 
        } 
        if(job.panStepper->getCurrentPosition()>=(job.pan.panTargetPos/2))
        { 
          //Serial.println("after half:->");
          //Serial.println(job.panStepper->getCurrentPosition());
          rotCurrentAngle = atan2(objectDistance,job.pan.travelDist) * 180 / pi;
          job.rot.rotationAngle = rotCurrentAngle;//deltaAngle;
          job.rot.recalcFigures();
          //Serial.println("<-after half");
        }
        //deltaAngle = (rotCurrentAngle - previousCurrentAngle);
        //job.rot.rotationAngle = rotCurrentAngle;//deltaAngle;
        //job.rot.recalcFigures();
        //job.rotStepper->setSpeedInHz(job.rot.rotSpeedInHz);
        job.rotStepper->moveTo(job.rot.rotTargetPos);
      
        progress = ((double)jobPassedTime / (double)jobDuaration)*100;
  }
  exitMenuOptions = 0;
}

result testHandler(menuOut& o,eventMask e,navNode& nav, prompt &item) { 
   exitMenuOptions = 99;
   menuIdle = true;
   Serial.println("DONE!!!");
   //menuIdle = false;
  return proceed;
};

void loop() {
 switch (exitMenuOptions) {
      case 99: {
          Serial.println("GO!!!");          
          runTest();
          break;
        }  
      case 1: {        
          runPanWorkload();
          break;
        }
      case 2: {
          runRotateWorkload();
          break;
        }
      case 3: {
          runPanRotateWorkload();
          break;
        }        
      default: // Do the normal program functions with ArduinoMenu
      {
          delay(400);
          break;
      }
  }
  delay(40);
}