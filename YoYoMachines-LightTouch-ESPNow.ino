//#define DEV
//#define STAGING

#define FADE_3 22
#define FADE_1 21

#define EXTERNAL_BUTTON 23
#define CAPTOUCH T0

#define LED_BUILTIN 2
#define LED_BUILTIN_ON HIGH

int BUTTON_BUILTIN = 0;

bool disconnected = false;

unsigned long wificheckMillis;
unsigned long wifiCheckTime = 5000;


enum PAIRED_STATUS {
  remoteSetup,
  localSetup,
  pairedSetup
};
int currentPairedStatus = remoteSetup;

enum SETUP_STATUS {
  setup_pending,
  setup_client,
  setup_server,
  setup_finished
};
int currentSetupStatus = setup_pending;

#define PROJECT_SLUG "ESP32-SOCKETIO"
#define VERSION "v0.2"
#define ESP32set
#define WIFICONNECTTIMEOUT 240000
#define SSID_MAX_LENGTH 31

String savedText;    //UNIQUE ID
char channelID[24];  // fixed 24 chars
bool portalIsOpen = false;

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>

#include <AceButton.h>
using namespace ace_button;

Preferences prefs;
WebServer server(80);
DNSServer dns;

//rgb led variables
#include <FastLED.h>
#define WS2812PIN 5
#define NUMPIXELS 2
#define PIXELUPDATETIME 30
#define PIXELUPDATETIMELONG 5000
#define USERLED 0
#define REMOTELED 1
#define RGBLEDPWMSTART 120
#define FASTLONGFADE 120
unsigned long LONGFADEMINUTESMAX = 360;
#define LONGFADECHECKMILLIS 120000
unsigned long prevLongFadeVal[NUMPIXELS] = { 0, 0 };
uint8_t hue[NUMPIXELS];
uint8_t saturation[NUMPIXELS];
uint8_t value[NUMPIXELS];
bool ledChanged[NUMPIXELS] = { false, false };
unsigned long prevPixelMillis;
bool isLongFade[NUMPIXELS] = { false, false };
unsigned long prevlongPixelMillis;
unsigned long longFadeMinutes[NUMPIXELS];
unsigned long prevLongFadeMillis[NUMPIXELS];
bool isRemoteLedFading = false;
CRGB leds[NUMPIXELS];
bool readyToFadeRGB[NUMPIXELS] = { false, false };
bool isFadingRGB[NUMPIXELS] = { false, false };
unsigned long fadeTimeRGB[NUMPIXELS];
#define RGBFADEMILLIS 6

typedef struct command_struct {
  byte cmd;
  char uniqueID[24];
  uint16_t sendHue;
};


/// Led Settings ///
bool isBlinking = false;
bool readyToBlink = false;
unsigned long blinkTime;
int blinkDuration = 200;

// Touch settings and config
class CapacitiveConfig : public ButtonConfig {
public:
  uint8_t _pin;
  uint16_t _threshold;
  CapacitiveConfig(uint8_t pin, uint16_t threshold) {
    _pin = pin;
    _threshold = threshold;
  }
  void setThreshold(uint16_t CapThreshold) {
    _threshold = CapThreshold;
  }
protected:
  int readButton(uint8_t /*pin*/) override {
    uint16_t val = touchRead(_pin);
    return (val < _threshold) ? LOW : HIGH;
  }
};

int TOUCH_THRESHOLD = 60;
int TOUCH_HYSTERESIS = 20;
#define LONG_TOUCH 1500
#define LONG_PRESS 10000
CapacitiveConfig touchConfig(CAPTOUCH, TOUCH_THRESHOLD);
AceButton buttonTouch(&touchConfig);
bool isSelectingColour = false;

//Button Settings
AceButton buttonBuiltIn(BUTTON_BUILTIN);
AceButton buttonExternal(EXTERNAL_BUTTON);

void handleButtonEvent(AceButton*, uint8_t, uint8_t);
int longButtonPressDelay = 5000;

//reset timers
bool isResetting = false;
unsigned long resetTime;
int resetDurationMs = 4000;

String myID = "";



void setup() {
  setupPixels();
  Serial.begin(115200);
  setupPins();
  setupPrefs();
  initEspNow();

  LONGFADEMINUTESMAX = checkFadingLength();
  setupCapacitiveTouch();

  //create 10 digit ID
  myID = generateID();

  digitalWrite(LED_BUILTIN, 0);
  Serial.println("setup complete");
}

void loop() {

  if (portalIsOpen == false) {
    rgbLedHandler();
    ledHandler();
    buttonBuiltIn.check();
    buttonExternal.check();
    buttonTouch.check();
    checkReset();
  } else {
    portalHandler();
  }
}
