#include <Arduino.h>
#include <SPI.h>
#include <TFT_ST7735.h>
#include <EEPROM.h>
#include "config.h"

// font
#include "_usr/frutiger.c"
#include "_usr/console.c"


#define DISPLAY_COLOR_GRAY        0x39A7
#define DISPLAY_COLOR_TIMER_OFF   0x8D5F
#define DISPLAY_COLOR_TIMER_ON    0xCC5F
#define DISPLAY_COLOR_TEMP_WANTED 0x4DA4
#define DISPLAY_COLOR_TEMP_LOWER  0x31F
#define DISPLAY_COLOR_TEMP_HIGHER 0xE984

#define DISPLAY_COLOR_TEMP_STEP   0xF201
#define DISPLAY_COLOR_ERROR       0xF041


class Display {

  int last_time = 0;
  float last_wanted_temp = 0.0;
  float last_temp = 0.0;
  bool menu_open = false;

  String format_time(int wt = -1){
    String ret = "";
    int minutes = (int) wt / 60;
    int secs = wt % 60;
    String str_min = String(minutes, DEC);
    String str_sec = String(secs, DEC);
    if (minutes<10) ret+="0";
    ret+=str_min;
    ret+=":";
    if (secs<10) ret+="0";
    ret+=str_sec;
    return ret;
  }

  String format_int(int wt){
    String ret = String();
    int minutes = (int) wt / 60;
    int secs = wt % 60;
    String str_min = String(minutes, DEC);
    String str_sec = String(secs, DEC);
    if (minutes<10) ret+="0";
    ret+=str_min;
    ret+=":";
    if (secs<10) ret+="0";
    ret+=str_sec;
    return ret;
  }

  String format_temp(float tt){
    char buffer[12];
    dtostrf(tt, 5, 2, buffer);
    String str_temp =  dtostrf(tt, 5, 2, buffer);
    return str_temp;
  }

  public:


  TFT_ST7735* tft;
  bool timer_is_running=false;
  bool step_timer_active = false;
  int timer_step_size = 0;
  float temp_grace_value = 0;
  bool temp_grace_active = false;
  bool temp_is_error;

  Display() {
  }

  void init(){
    tft = new TFT_ST7735(DISPLAY_PIN_CS, DISPLAY_PIN_DC);
    tft->begin();
    tft->setRotation(135);
    tft->setTextScale(1);
    redraw_main_screen();
  }

  void redraw_main_screen(){

    tft->clearScreen();
    tft->setInternalFont();
    tft->setTextColor(DISPLAY_COLOR_GRAY);
    tft->println("TIMER");

    update_timer_value(last_time);

    tft->setTextColor(DISPLAY_COLOR_GRAY);
    tft->setCursor(0, 45);
    tft->setInternalFont();
    tft->println("WANTED");

    update_wanted_temp_value(last_wanted_temp);

    tft->setTextColor(DISPLAY_COLOR_GRAY);
    tft->setCursor(114, 45);
    tft->setInternalFont();
    tft->println("CURRENT");
    update_temp_value(last_temp);

  }

  void show_timer_step(){
    step_timer_active = true;
    tft->fillRect(0,0,120,30, BLACK);
    tft->setCursor(0, 0);
    tft->setInternalFont();
    tft->setTextColor(DISPLAY_COLOR_GRAY);
    tft->println("STEP SIZE");
    tft->setTextColor(DISPLAY_COLOR_TEMP_STEP);
    tft->setCursor(0, 8);
    font_console();
    tft->println(timer_step_size);
  }

  void hide_timer_step(){
    step_timer_active = false;
    tft->fillRect(0,0,120,30, BLACK);
    tft->setCursor(0, 0);
    tft->setInternalFont();
    tft->setTextColor(DISPLAY_COLOR_GRAY);
    tft->println("TIMER");
    if (timer_is_running){
      tft->setTextColor(DISPLAY_COLOR_TIMER_ON);
    } else {
      tft->setTextColor(DISPLAY_COLOR_TIMER_OFF);
    }
    font_console();
    tft->setCursor(0, 8);
    tft->println(format_time(last_time));
  }


  void update_timer_step_size(int value){
    tft->setCursor(0, 8);
    font_console();
    String ts_old = String(timer_step_size);
    String ts_new = String(value);
    uint16_t curr_dispcolor = DISPLAY_COLOR_TEMP_STEP;
    for (int i = 0; i < ts_old.length(); i++){
      if ( (ts_old.charAt(i) != ts_new.charAt(i) ) ||
           ((value % 60)==0) || ((last_time % 60)==0) ) {
        tft->setTextColor(BLACK);
      }
      else {
        tft->setTextColor(curr_dispcolor);
      }
      tft->print(ts_old.charAt(i));
    }
    tft->setCursor(0, 8);
    tft->setTextColor(curr_dispcolor);
    tft->print(ts_new);
    timer_step_size=value;
  }

  void update_timer_value(int value){
    if ( step_timer_active ) {
      return;
    }
    uint16_t curr_dispcolor = DISPLAY_COLOR_TIMER_OFF;
    if (timer_is_running){
      curr_dispcolor=DISPLAY_COLOR_TIMER_ON;
    }
    font_console();
    String ts_old = format_time(last_time);
    String ts_new = format_time(value);
    tft->setCursor(0, 8);
    for (int i = 0; i < ts_old.length(); i++){
      if ( (ts_old.charAt(i) != ts_new.charAt(i) ) ||
           ((value % 60)==0) || ((last_time % 60)==0) ) {
        tft->setTextColor(BLACK);
      }
      else {
        tft->setTextColor(curr_dispcolor);
      }
      tft->print(ts_old.charAt(i));
    }
    tft->setCursor(0, 8);
    tft->setTextColor(curr_dispcolor);
    tft->print(format_time(value));
    last_time = value;
  }

  void show_temp_grace(){
    temp_grace_active = true;
    tft->fillRect(0,44,75,30, BLACK);
    tft->setCursor(0, 45);
    tft->setInternalFont();
    tft->setTextColor(DISPLAY_COLOR_GRAY);
    tft->println("GRACE");
    tft->setTextColor(DISPLAY_COLOR_TEMP_STEP);
    tft->setCursor(0, 53);
    font_console();
    update_temp_grace_value(temp_grace_value);
  }

  void hide_temp_grace(){
    temp_grace_active = false;
    tft->fillRect(0,44,75,30, BLACK);
    tft->setCursor(0, 45);
    tft->setInternalFont();
    tft->setTextColor(DISPLAY_COLOR_GRAY);
    tft->println("WANTED");
    update_wanted_temp_value(last_wanted_temp);
  }

  void update_temp_grace_value(float value){
    tft->setCursor(0, 53);
    font_console();
    String ts_old = format_temp(temp_grace_value);
    String ts_new = format_temp(value);
    uint16_t curr_dispcolor = DISPLAY_COLOR_TEMP_STEP;
    for (int i = 0; i < ts_old.length(); i++){
      if ( (ts_old.charAt(i) != ts_new.charAt(i) ) ) {
        tft->setTextColor(BLACK);
      }
      else {
        tft->setTextColor(curr_dispcolor);
      }
      tft->print(ts_old.charAt(i));
    }
    tft->setCursor(0, 53);
    tft->setTextColor(curr_dispcolor);
    tft->print(ts_new);
    temp_grace_value=value;
  }


  void update_wanted_temp_value(float value){
    if ( temp_grace_active ) {
      return;
    }
    font_console();
    String ts_old = format_temp(last_wanted_temp);
    String ts_new = format_temp(value);
    tft->setCursor(0, 53);
    for (int i = 0; i < ts_old.length(); i++){
      if ( (ts_old.charAt(i) != ts_new.charAt(i) ) ) {
        tft->setTextColor(BLACK);
      }
      else {
        tft->setTextColor(DISPLAY_COLOR_TEMP_WANTED);
      }
      tft->print(ts_old.charAt(i));
    }
    tft->setCursor(0, 53);
    tft->setTextColor(DISPLAY_COLOR_TEMP_WANTED);
    tft->print(format_temp(value));
    last_wanted_temp = value;
  }


  void update_temp_value(float value){
    if (menu_open) {
      return;
    }
    uint16_t curr_dispcolor = DISPLAY_COLOR_TEMP_LOWER;
    if (value>last_wanted_temp){
      curr_dispcolor = DISPLAY_COLOR_TEMP_HIGHER;
    }


    font_console();
    String ts_old = format_temp(last_temp);
    String ts_new = format_temp(value);
    tft->setCursor(85, 53);
    for (int i = 0; i < ts_old.length(); i++){
      if ( (ts_old.charAt(i) != ts_new.charAt(i) ) ) {
        tft->setTextColor(BLACK);
      }
      else {
        tft->setTextColor(curr_dispcolor);
      }
      tft->print(ts_old.charAt(i));
    }
    tft->setCursor(85, 53);
    tft->setTextColor(curr_dispcolor);
    tft->print(format_temp(value));
    last_temp = value;
  }

  void show_temp_error(){
    if (!temp_is_error){
      tft->fillRect(85,53,75,30, BLACK);
      tft->setCursor(85, 53);
      tft->setTextColor(DISPLAY_COLOR_ERROR);
      font_console();
      tft->println("ERROR");
      temp_is_error = true;
    }
  }

  void hide_temp_error(){
    tft->fillRect(85,53,75,30, BLACK);
    update_temp_value(last_temp);
    temp_is_error = false;
  }


  void font_frutiger(){
    tft->setFont(&frutiger);
  }

  void font_console(){
    tft->setFont(&console);
  }


};
