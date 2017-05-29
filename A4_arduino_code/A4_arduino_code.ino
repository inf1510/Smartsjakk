#include <Wire.h>
#include <LiquidTWI2.h>
#include "LedControl.h"
#include <SPI.h>           // SPI library
#include <SdFat.h>         // SDFat Library
#include <SdFatUtil.h>     // SDFat Util Library
#include <SFEMP3Shield.h>  // Mp3 Shield Library

LiquidTWI2 lcd(0);

SdFat sd;
SFEMP3Shield MP3player;
const uint8_t volume = 35;
const uint16_t monoMode = 1;

const byte whitePin = 18;
const byte blackPin = 19;
const byte menuPin = 3;

byte full = B11111111;
byte empty = B00000000;

// Modes:
// 0 - Before start
// 1 - Running
// 2 - Paused
// 3 - Ended
int mode = 0;
boolean whiteTurn = true;

int minutes = 10; 
int seconds = 0;
int minutes2 = 10;
int seconds2 = 0;
int dec1 = 0;
int dec2 = 0;
int addTime = 0;
int timeModes[7][2] = {{3, 0}, {3, 2}, {5, 2}, {10, 0}, {15, 10}, {30, 0}, {45, 45}};
int timeMode = 3;
int i = 0; int j = 0;
int standings = 32;

static unsigned long last_interrupt_time = 0;
static unsigned long timer_time = 0;
static unsigned long last_music_time = 0;

LedControl lc=LedControl(11,13,12,4);
LedControl lc2=LedControl(30,34,32,4);
 
void setup() {
  lcd_setup();
  lcd.setMCPType(LTI_TYPE_MCP23017);
  lcd.begin(20, 4);
  lcd.setCursor(0, 0);
  lcd.print("<             Music>");

  attachInterrupt(digitalPinToInterrupt(whitePin), white , RISING);
  attachInterrupt(digitalPinToInterrupt(blackPin), black , RISING);
  attachInterrupt(digitalPinToInterrupt(menuPin), menu , RISING);

  pinMode(40, INPUT);

  setupMatrix();

  initSD();
  initMP3Player();
}

void music(){
  unsigned long music_time = millis();
  if (music_time - last_music_time > 200){
    last_music_time = music_time;
    if (MP3player.isPlaying()){
      MP3player.stopTrack();
    } else {
      playTrack(6 + random(5));
    }
  }
}

void black() {
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 200){
    last_interrupt_time = interrupt_time;
    switch (mode){
      case 0: increaseTime();
        break;
      case 1:
        if (!whiteTurn){
          whiteTurn = true;
          addTimeBlack();
          updateMatrix();
        }
        break;
      case 2: // do nothing
        break;
      case 3: // do nothing
        break;
    }
  }
}

void white() {
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 200){
    last_interrupt_time = interrupt_time;
    switch (mode){
      case 0: decreaseTime();
        break;
      case 1:
        if (whiteTurn){
          whiteTurn = false;
          addTimeWhite();
          updateMatrix();
        }
        break;
      case 2: reset();
        break;
      case 3: // do nothing
        break;
    }
  }
}

void addTimeWhite(){
  seconds += addTime;
  if (seconds > 59){
    seconds -= 60;
    minutes++;  
  }
}

void addTimeBlack(){
  seconds2 += addTime;
  if (seconds2 > 59){
    seconds2 -= 60;
    minutes2++;  
  }
}

void menu() {
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 200){
    last_interrupt_time = interrupt_time;
    switch (mode){
      case 0: mode = 1;
        setupMatrix();
        if (!MP3player.isPlaying()){
          playTrack(5);
          delay(3000);
        }
        break;
      case 1: mode = 2;
        break;
      case 2: mode = 1;
        break;
      case 3: reset();
        break;
    }
  }
}

void increaseTime(){
  if (timeMode != 6) {timeMode++;}
  minutes = timeModes[timeMode][0];
  minutes2 = timeModes[timeMode][0];
  addTime = timeModes[timeMode][1];
}

void decreaseTime(){
  if (timeMode != 0) {timeMode--;}
  minutes = timeModes[timeMode][0];
  minutes2 = timeModes[timeMode][0];
  addTime = timeModes[timeMode][1];
}

void reset(){
  seconds = 0;
  seconds2 = 0;
  minutes = 10;
  minutes2 = 10;
  dec1 = 0;
  dec2 = 0;
  addTime = 0;
  whiteTurn = true;
  mode = 0;
  standings = 32;
}
 
void countDown() {
  unsigned long count_time = millis();
  if (count_time - timer_time > 40){
    timer_time = count_time; 
    if (whiteTurn) {
      if (dec1 > 0) {dec1 -= 1;}
      else{
        dec1 = 9;
        if (seconds > 0) {seconds -= 1;} 
        else {seconds = 59; minutes -= 1;}
      }
    }else{
      if (dec2 > 0) {dec2 -= 1;}
      else{
        dec2 = 9;
        if (seconds2 > 0) {seconds2 -= 1;}
        else {seconds2 = 59; minutes2 -= 1;}
      }
    }
    if (minutes == -1) {
       lcd.setCursor(2, 2);
      lcd.print("NO TIME!");
      mode = 3;
    }
    if (minutes2 == -1) {
      lcd.setCursor(10, 2);
      lcd.print("NO TIME!");
      mode = 3;
    }
  }
}

void lcd_setup(){
  for (i = 0; i < 4; i++){
    lc.shutdown(i, false);
    lc.setIntensity(i,12);
    lc.clearDisplay(i);
  
    lc2.shutdown(i,false);
    lc2.setIntensity(i,12);
    lc2.clearDisplay(i);
  }
}

void setupMatrix() {
  for (i = 0; i < 8; i++){
    for (j = 0; j < 4; j++){
      (j*8 + 7 - i > standings) ? lc.setColumn(j,i,full) : lc.setColumn(j,i,empty);
      (j*8 + 7 - i < 64-standings) ? lc2.setColumn(j,i,full) : lc2.setColumn(j,i,empty);
    }
  }
}

void updateMatrix(){
  if (whiteTurn){standings -= random(1, 4)*random(1, 4);}
  else {standings += random(1, 4)*random(1, 4);}
  if (standings > 64){standings = 64;}
  else if (standings < 0){standings = 0;}
  setupMatrix();
}

void loop() {
  switch (mode){
    case 0:
      lcd.setCursor(1, 0);
      lcd.print("Start ");
      lcd.setCursor(0, 1);
      lcd.print("----Choose Time-----");
      lcd.setCursor(2, 2);
      lcd.print("  ");
      (minutes < 10) ? lcd.print("0") : NULL;
      lcd.print(minutes);
      lcd.print(":");
      (seconds < 10) ? lcd.print("0") : NULL;
      lcd.print(seconds);
      lcd.print(" +00:");
      (addTime < 10) ? lcd.print("0") : NULL;
      lcd.print(addTime);
      lcd.print("  ");
      lcd.setCursor(0, 3);
      lcd.print("<-                +>");
      break;
    case 1:
      lcd.setCursor(1, 0);
      lcd.print("Pause ");
      lcd.setCursor(0, 1);
      lcd.print("------Playing-------");
      lcd.setCursor(0, 3);
      lcd.print("                    ");
      lcd.setCursor(2, 2);
      (minutes < 10) ? lcd.print("0") : NULL;
      lcd.print(minutes);
      lcd.print(":");
      (seconds < 10) ? lcd.print("0") : NULL;
      lcd.print(seconds);
      lcd.print(".");
      lcd.print(dec1);
      lcd.print("  ");
      (minutes2 < 10) ? lcd.print("0") : NULL;
      lcd.print(minutes2);
      lcd.print(":");
      (seconds2 < 10) ? lcd.print("0") : NULL;
      lcd.print(seconds2);
      lcd.print(".");
      lcd.print(dec2);
      countDown();
      break;
    case 2:
      lcd.setCursor(1, 0);
      lcd.print("Resume");
      lcd.setCursor(0, 1);
      lcd.print("-------Paused-------");
      lcd.setCursor(0, 3);
      lcd.print("<End Game");
      break;
    case 3:
      lcd.setCursor(1, 0);
      lcd.print("New Game");
      lcd.setCursor(0, 1);
      lcd.print("-----Game ended-----");
      break;
  }
  if (digitalRead(40)){music();}
}

void playTrack(int track){
  if (MP3player.isPlaying())
    MP3player.stopTrack();
  uint8_t result = MP3player.playTrack(track);
}

void stopTrack(){
  if (MP3player.isPlaying())
    MP3player.stopTrack();
}

void initSD(){
  if(!sd.begin(SD_SEL, SPI_HALF_SPEED)) 
    sd.initErrorHalt();
  if(!sd.chdir("/")) 
    sd.errorHalt("sd.chdir");
}

void initMP3Player()
{
  uint8_t result = MP3player.begin();
  if(result != 0){}
  MP3player.setVolume(volume, volume);
  MP3player.setMonoMode(monoMode);
}
