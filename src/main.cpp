#include <Arduino.h>
#include <menu.h>
#include <menuIO/u8g2Out.h>
#include <menuIO/chainStream.h>
#include <menuIO/rotaryEventIn.h>
#include <mytask.h>

// some example libraries to handle the rotation and clicky part
// of the encoder. These will generate our events.
#include <qdec.h> //https://github.com/SimpleHacks/QDEC
#include <AceButton.h> // https://github.com/bxparks/AceButton

#define LEDPIN 2

// Encoder
const int ROTARY_PIN_BUT  = 27;
const int16_t ROTARY_PIN_A = 25; // the first pin connected to the rotary encoder
const int16_t ROTARY_PIN_B = 26; // the second pin connected to the rotary encoder

using namespace ::ace_button;
using namespace ::SimpleHacks;
QDecoder qdec(ROTARY_PIN_A, ROTARY_PIN_B, true); // rotary part
AceButton button(ROTARY_PIN_BUT); // button part
//--//

// Initial basic Task
uint8_t travStepPin = 22;
//travelDist,pulsesPerMM,travTime,travStepPin
Pan pan;


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

unsigned int timeOn=10;
unsigned int timeOff=90;

int exitMenuOptions = 0; //Forces the menu to exit and cut the copper tape

//customizing a prompt look!
//by extending the prompt class
//this prompt will count seconds and update himself on the screen.
class altPrompt:public prompt {
public:
  unsigned int t=0;
  unsigned int last=0;
  altPrompt(constMEM promptShadow& p):prompt(p) {}
  Used printTo(navRoot &root,bool sel,menuOut& out, idx_t idx,idx_t len,idx_t) override {
    last=t;
    return out.printRaw(String(t).c_str(),len);
  }
  virtual bool changed(const navNode &nav,const menuOut& out,bool sub=true) {
    t=millis()/1000;
    return last!=t;
  }
};

//customizing a menu prompt look
class confirmExit:public menu {
public:
  confirmExit(constMEM menuNodeShadow& shadow):menu(shadow) {}
  Used printTo(navRoot &root,bool sel,menuOut& out, idx_t idx,idx_t len,idx_t p) override {
    return idx<0?//idx will be -1 when printing a menu title or a valid index when printing as option
      menu::printTo(root,sel,out,idx,len,p)://when printing title
      out.printRaw((constText*)F("Exit"),len);//when printing as regular option
  }
};

result doAlert(eventMask e, prompt &item);

result action1(eventMask e,navNode& nav, prompt &item);


result resRunPan() {
  delay(500);
  exitMenuOptions = 1;
  return proceed;
}

result rotate() {
  delay(500);
  exitMenuOptions = 2;
  return proceed;
}

result showEvent(eventMask e) {
  Serial.print("event: ");
  Serial.println(e);
  return proceed;
}

// this function is defined below because we need to refer
// to the navigation system (suspending the menu)
result systemExit();

result action2(eventMask e,navNode& nav, prompt &item) {
  Serial.print("action2 event:");
  Serial.println(e);
  Serial.flush();
  return proceed;
}


result inputTravelDist(eventMask e,navNode& nav, prompt &item) {
  Serial.print("inputTravelDist event:");
  Serial.println(pan.travelDist);
  Serial.flush();
  return proceed;
}
//using the customized menu class
//note that first parameter is the class name
altMENU(confirmExit,subMenu2,"Exit?",doNothing,noEvent,wrapStyle,(Menu::_menuData|Menu::_canNav)
  ,OP("Yes",systemExit,enterEvent)
  ,EXIT("Cancel")
);

MENU(subMenuPan,"Pan-Menu",doNothing,anyEvent,wrapStyle
  ,FIELD(timeOn,"On","ms",0,1000,10,1, doNothing, noEvent, noStyle)  
  ,OP("Pan test",resRunPan,enterEvent)
  ,EXIT("<Back")
);

MENU(subMenuRotate,"Rotate-Menu",doNothing,anyEvent,wrapStyle
  ,FIELD(timeOn,"On","ms",0,1000,10,1, doNothing, noEvent, noStyle)  
  ,OP("Rotate test",rotate,enterEvent)
  ,EXIT("<Back")
);

int selTest=0;
SELECT(selTest,selMenu,"Select",doNothing,noEvent,wrapStyle
  ,VALUE("Zero",0,doNothing,noEvent)
  ,VALUE("One",1,doNothing,noEvent)
  ,VALUE("Two",2,doNothing,noEvent)
);

int chooseTest=-1;
CHOOSE(chooseTest,chooseMenu,"Choose",doNothing,noEvent,wrapStyle
  ,VALUE("First",1,doNothing,noEvent)
  ,VALUE("Second",2,doNothing,noEvent)
  ,VALUE("Third",3,doNothing,noEvent)
  ,VALUE("Last",-1,doNothing,noEvent)
);

MENU(mainMenu, "Slider Menu", doNothing, anyEvent, wrapStyle
  ,FIELD(pan.travelDist,"Distance","mm",pan.minTravelDist,pan.maxTravelDist,pan.travelDistInc,pan.travelDistIncFine, inputTravelDist, enterEvent , noStyle)  
  ,FIELD(pan.travelTime,"Time","s",1,3600,1,1, inputTravelDist, enterEvent , noStyle)  
  ,OP("Pan test",resRunPan,enterEvent)
  //,altOP(altPrompt,"",doNothing,noEvent)
  //,OP("Op1",action1,anyEvent)
  //,OP("Op2",action2,enterEvent)
  //,OP("Alert test",doAlert,enterEvent)
  //,FIELD(timeOn,"On","ms",0,1000,10,1, doNothing, noEvent, noStyle)
  //,SUBMENU(selMenu)
  //,SUBMENU(chooseMenu)
  //,SUBMENU(subMenuPan)
  //,SUBMENU(subMenuRotate) 
  ,EXIT("<Back")
);

result initialize(eventMask e,navNode& nav, prompt &item) {
  Serial.print("action1 event:");
  Serial.println(e);
  Serial.flush();
  exitMenuOptions = 0; // Return to the menu
  mainMenu.dirty = true;
  return proceed;
}


result action1(eventMask e,navNode& nav, prompt &item) {
  Serial.print("action1 event:");
  Serial.println(e);
  Serial.flush();
  exitMenuOptions = 0; // Return to the menu
  mainMenu.dirty = true;
  return proceed;
}


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

result alert(menuOut& o,idleEvent e) {
  if (e==idling) {
    o.setCursor(0,0);
    o.print("alert test");
    o.setCursor(0,1);
    o.print("[select] to continue...");
  }
  return proceed;
}

result doAlert(eventMask e, prompt &item) {
  nav.idleOn(alert);
  return proceed;
}


bool running=true;//lock menu if false

result systemExit() {
  Serial.println();
  Serial.println("Terminating...");
  //do some termiination stuff here
  
  Serial.println("done.");
  running=false;//prevents the menu from running again!
  nav.idleOn();//suspend the menu system
 
  return quit;

}

// This is the ISR (interrupt service routine) for rotary events
// We will convert/relay events to the RotaryEventIn object
// Callback config in setup()
void IsrForQDEC(void) { 
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

void setup() {
  Serial.begin(115200);
  while(!Serial);

  pinMode(LEDPIN, OUTPUT);
  int ledCtrl=LOW;
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

  // setup OLED disaply
  Wire.begin(SDA,SCL);
  u8g2.begin();
  u8g2.setFont(fontName);

  //do {
  //  u8g2.drawStr(0,fontY,"RotaryEventIn demo");
  //} while(u8g2.nextPage());

  // appear
  for(int c=255;c>0;c--) {
    u8g2.setContrast(255-255.0*log(c)/log(255));
    delay(12);
  }
}
void measure_important_function(void) {
    const unsigned MEASUREMENTS = 5000;
    uint64_t start = esp_timer_get_time();
    uint64_t end = esp_timer_get_time();
   
}

// TODO Logik -- exitMenuOptions steuert den Task - Numerierung siehe loop
void runPan() {
    u8g2.clear();
    u8g2.setBitmapMode(1);
    //u8g2.setFont(fontName);
    u8g2.drawStr(12, 13, "Running Pan");
    u8g2.drawFrame(11, 20, 100, 12);
    u8g2.drawStr(53, 45, "     ");
    u8g2.drawBox(12, 25, 0, 10);
    u8g2.sendBuffer();
    //u8g2.drawBox(12, 29, 107, 10); -> 100%
    volatile int progress = 0;

    //uint64_t start = esp_timer_get_time();
    Pan pan;
    pan.travelTime = 30;
    pan.travelDist = 200;

    //pan.travelTime = 2;
    //pan.travelDist = 100;
    digitalWrite(pan.travDirPin, HIGH); // Enables the motor to move in a particular direction
    //Serial.println("direction pin HIGH");
    while (progress < 100) {
        pan.executeChunk();
        //u8g2.drawBox(12, 25, progress, 10);
        //u8g2.updateDisplayArea(12,25,100,10);        
        //char buffer[6];
        //sprintf(buffer, "%d %", progress);    
        //u8g2.drawStr(53,45, buffer);

        progress += 1; // for demonstration only
    }
    //u8g2.sendBuffer();
    progress = 0;
    delay(500);
    digitalWrite(pan.travDirPin, LOW); //Changes the direction of rotation
    while (progress < 100) {
        pan.executeChunk();
        //u8g2.drawBox(12, 25, progress, 10);
        //u8g2.updateDisplayArea(12,25,100,10);
        //char buffer[6];
        //sprintf(buffer, "%d %", progress);    
        //u8g2.drawStr(53,45, buffer);

        progress += 1; // for demonstration only
    }  
    //u8g2.sendBuffer();
    progress = 0;
  //Serial.println("direction pin LOW");y(1500);
    //pinMode(pan.enablePin, INPUT); // disable motors     
    //uint64_t end = esp_timer_get_time();
    //printf("%llu milliseconds",(end - start)/1000);

    //char buffer2[12];
    //sprintf(buffer2, "%d", (end - start)/1000);
    //u8g2.drawStr(53, 70,buffer2);
    //    u8g2.sendBuffer();
    exitMenuOptions = 0; // Return to the menu
    //mainMenu.dirty = true;
    //subMenuPan.dirty = true;
}


void loop() {
  constexpr int menuFPS = 1000 / 30;
  static unsigned long lastMenuFrame = - menuFPS;
  //unsigned long now = millis();
  //put your main code here, to run repeatedly:
  switch (exitMenuOptions) {
    case 1: {
        //delay(500); // Pause to allow the button to come up
        runPan(); 
        break;
      }
    case 2: {
        delay(500); // Pause to allow the button to come up
        //runRotate();
        break;
      }
    default: // Do the normal program functions with ArduinoMenu
     {
      //if (now - lastMenuFrame >= menuFPS) {
        button.check(); // acebutton check, rotary is on ISR
        nav.doInput(); // menu check
      //}
      }
  }
  if (nav.changed(0)) {//only draw if menu changed for gfx device
    u8g2.firstPage();
    do nav.doOutput(); while(u8g2.nextPage());
  }
}