#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include "background.h"


#define TFT_CS     21
#define TFT_DC      7
#define TFT_RST     6
#define TFT_MOSI    5
#define TFT_SCLK    4
#define TFT_BLK     3

#define BUZZER_PIN 20

#define BTN_MODE    8
#define BTN_UP      9
#define BTN_DOWN   10
#define BTN_ALARM   2

#define SCREEN_WIDTH  284
#define SCREEN_HEIGHT  76

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
GFXcanvas16 canvas(SCREEN_WIDTH, SCREEN_HEIGHT);


int hours = 12, minutes = 0, seconds = 0;
int alarmHour = 7, alarmMinute = 0;
bool alarmEnabled = false;
bool alarmRinging = false;
unsigned long lastTick = 0;

uint8_t currentBgIndex = 0;

enum Mode { MODE_CLOCK, MODE_SET_ALARM_H, MODE_SET_ALARM_M };
Mode currentMode = MODE_CLOCK;

void setup() {
  Serial.begin(115200);

  pinMode(TFT_BLK, OUTPUT);
  digitalWrite(TFT_BLK, HIGH);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  pinMode(BTN_MODE, INPUT_PULLUP);
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_ALARM, INPUT_PULLUP);

  tft.init(76, 284);
  tft.setRotation(1);

  drawUI();
}

void loop() {

  if (millis() - lastTick >= 1000) {
    lastTick = millis();
    seconds++;
    if (seconds >= 60) {
      seconds = 0;
      minutes++;
      if (minutes >= 60) {
        minutes = 0;
        hours = (hours + 1) % 24;
      }
    }

    if (alarmEnabled && hours == alarmHour && minutes == alarmMinute && seconds == 0) {
      alarmRinging = true;
    }

    if (alarmRinging) {
      tone(BUZZER_PIN, 2000, 200);
    }

    drawUI();
  }

  
  if (Serial.available()) {
    String timeStr = Serial.readStringUntil('\n');
    timeStr.trim();
    if (timeStr.length() == 8 && timeStr.charAt(2) == ':' && timeStr.charAt(5) == ':') {
      hours = timeStr.substring(0, 2).toInt();
      minutes = timeStr.substring(3, 5).toInt();
      seconds = timeStr.substring(6, 8).toInt();
      drawUI();
    }
  }

  handleButtons();
}

void printShadowText(const char* text, int16_t x, int16_t y, uint8_t size, uint16_t fgColor, uint16_t shadowColor) {
  canvas.setTextSize(size);
  
  canvas.setTextColor(shadowColor);
  canvas.setCursor(x + 2, y + 2);
  canvas.print(text);

  canvas.setTextColor(fgColor);
  canvas.setCursor(x, y);
  canvas.print(text);
}

void drawUI() {
 
  canvas.drawRGBBitmap(0, 0, bg_images[currentBgIndex], SCREEN_WIDTH, SCREEN_HEIGHT);

  char timeBuf[9];
  snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", hours, minutes, seconds);

  char alarmBuf[20];
  snprintf(alarmBuf, sizeof(alarmBuf), "ALM %02d:%02d [%s]", 
           alarmHour, alarmMinute, alarmEnabled ? "ON" : "OFF");

  uint16_t timeColor   = ST7789_WHITE;
  uint16_t shadowColor = ST7789_BLACK;

  if (currentBgIndex == 2) { // Arctic background adjustment
    timeColor   = ST7789_NAVY;
    shadowColor = ST7789_WHITE;
  }

  
  printShadowText(timeBuf, 93, 14, 3, timeColor, shadowColor);

  
  if (currentMode == MODE_SET_ALARM_H) {
    printShadowText("> SET ALM HOUR ", 60, 48, 2, ST7789_YELLOW, ST7789_BLACK);
  } else if (currentMode == MODE_SET_ALARM_M) {
    printShadowText("> SET ALM MIN  ", 60, 48, 2, ST7789_YELLOW, ST7789_BLACK);
  } else {
    uint16_t statusColor = alarmEnabled ? ST7789_GREEN : ST7789_RED;
    printShadowText(alarmBuf, 60, 48, 2, statusColor, ST7789_BLACK);
  }

  
  tft.drawRGBBitmap(0, 0, canvas.getBuffer(), SCREEN_WIDTH, SCREEN_HEIGHT);
}

void handleButtons() {
  // Mode
  if (digitalRead(BTN_MODE) == LOW) {
    if (alarmRinging) { alarmRinging = false; delay(200); return; }
    currentMode = static_cast<Mode>((currentMode + 1) % 3);
    drawUI();
    delay(200);
  }

  // UP
  if (digitalRead(BTN_UP) == LOW) {
    if (alarmRinging) { alarmRinging = false; delay(200); return; }
    if (currentMode == MODE_SET_ALARM_H) alarmHour = (alarmHour + 1) % 24;
    else if (currentMode == MODE_SET_ALARM_M) alarmMinute = (alarmMinute + 1) % 60;
    drawUI();
    delay(200);
  }

  // down and BG
  if (digitalRead(BTN_DOWN) == LOW) {
    if (alarmRinging) { alarmRinging = false; delay(200); return; }
    if (currentMode == MODE_CLOCK) {
      currentBgIndex = (currentBgIndex + 1) % 4;
    } else if (currentMode == MODE_SET_ALARM_H) {
      alarmHour = (alarmHour - 1 + 24) % 24;
    } else if (currentMode == MODE_SET_ALARM_M) {
      alarmMinute = (alarmMinute - 1 + 60) % 60;
    }
    drawUI();
    delay(200);
  }

  // enable
  if (digitalRead(BTN_ALARM) == LOW) {
    if (alarmRinging) {
      alarmRinging = false;
    } else {
      alarmEnabled = !alarmEnabled;
    }
    drawUI();
    delay(200);
  }
}