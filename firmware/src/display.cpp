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

#define DISPLAY_COLOR_TIMER_STEP   0xF201
#define DISPLAY_COLOR_TEMP_STEP   0xF201
#define DISPLAY_COLOR_ERROR       0xF041

#define DISPLAY_COLOR_DOT_DISABLED 0x39E7
#define DISPLAY_COLOR_DOT_TIMER    0xCC5F
#define DISPLAY_COLOR_DOT_HEATER   0xF1C0
#define DISPLAY_COLOR_DOT_PUMP     0x37F


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

  void overdraw_text( uint16_t t,
                      uint16_t l,
                      uint16_t color,
                      String ts_old,
                      String ts_new,
                      int value=-1) {
      tft->setCursor(t, l);
      for ( int i = 0; i < ts_old.length(); i++ ){

        if ( (ts_old.charAt(i) != ts_new.charAt(i) ) ||
          ( value>-1 && (value % 60)==0) || ( value>-1 && (last_time % 60)==0 ) ) {
            tft->setTextColor(BLACK);
          }
          else {
            tft->setTextColor(color);
          }
          tft->print(ts_old.charAt(i));
        }
        tft->setCursor(t, l);
        tft->setTextColor(color);
        tft->print(ts_new);
  }

  void display_dot(char tt, uint16_t l, uint16_t t, uint16_t color){
    tft->fillCircle(l, t, 6, color);
    tft->setInternalFont();
    tft->setCursor(l-2,t-2);
    tft->setTextColor(BLACK);
    tft->print(tt);
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


    display_dot('T', 15, 115, DISPLAY_COLOR_DOT_DISABLED);
    display_dot('H', 35, 115, DISPLAY_COLOR_DOT_DISABLED);
    display_dot('P', 55, 115, DISPLAY_COLOR_DOT_DISABLED);
  }


  void timer_start(){
    display_dot('T', 15, 115, DISPLAY_COLOR_DOT_TIMER);
  }
  void timer_stop(){
    display_dot('T', 15, 115, DISPLAY_COLOR_DOT_DISABLED);
  }
  void heater_start(){
    display_dot('H', 35, 115, DISPLAY_COLOR_DOT_HEATER);
  }
  void heater_stop(){
    display_dot('H', 35, 115, DISPLAY_COLOR_DOT_DISABLED);
  }
  void pump_start(){
    display_dot('P', 55, 115, DISPLAY_COLOR_DOT_PUMP);
  }
  void pump_stop(){
    display_dot('P', 55, 115, DISPLAY_COLOR_DOT_DISABLED);
  }

  void show_timer_step(){
    step_timer_active = true;
    tft->fillRect(0,0,120,30, BLACK);
    tft->setCursor(0, 0);
    tft->setInternalFont();
    tft->setTextColor(DISPLAY_COLOR_GRAY);
    tft->println("STEP SIZE");
    tft->setTextColor(DISPLAY_COLOR_TIMER_STEP);
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

    font_console();
    overdraw_text(  0,
                    8,
                    DISPLAY_COLOR_TIMER_STEP,
                    format_time(last_time),
                    format_time(value),
                    value
                  );
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
    overdraw_text(  0,
                    8,
                    curr_dispcolor,
                    format_time(last_time),
                    format_time(value),
                    value
                  );
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
    overdraw_text(  0,
                    53,
                    DISPLAY_COLOR_TEMP_STEP,
                    format_temp(temp_grace_value),
                    format_temp(value));
    temp_grace_value=value;
  }


  void update_wanted_temp_value(float value){
    if ( temp_grace_active ) {
      return;
    }
    font_console();
    String ts_old = format_temp(last_wanted_temp);
    String ts_new = format_temp(value);
    overdraw_text(  0,
                    53,
                    DISPLAY_COLOR_TEMP_WANTED,
                    format_temp(last_wanted_temp),
                    format_temp(value));
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
    overdraw_text(  85,
                    53,
                    curr_dispcolor,
                    format_temp(last_temp),
                    format_temp(value));
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
