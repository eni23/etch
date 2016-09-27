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

class Display {
  TFT_ST7735* tft;

  int last_time = 0;
  float last_wanted_temp;
  float last_temp;

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

  String format_temp(float tt){
    char buffer[12];
    dtostrf(tt, 5, 2, buffer);
    String str_temp =  dtostrf(tt, 5, 2, buffer);
    return str_temp;
  }

  public:

  bool timer_is_running=false;

  Display() {
  }

  void init(){
    tft = new TFT_ST7735(DISPLAY_PIN_CS, DISPLAY_PIN_DC);
    tft->begin();
    tft->setRotation(135);
    tft->setTextScale(1);
    //font_console();
    tft->setTextColor(DISPLAY_COLOR_GRAY);
    tft->println("TIMER");

    /*font_console();
    tft->setTextColor(DISPLAY_COLOR_TIMER_OFF);
    tft->println("00:00");
    */
    update_timer_value(0);

    tft->setTextColor(DISPLAY_COLOR_GRAY);
    tft->setCursor(0, 45);
    tft->setInternalFont();
    tft->println("WANTED");

    /*font_console();
    tft->setTextColor(DISPLAY_COLOR_TEMP_WANTED);
    tft->setCursor(0, 53);
    tft->println("45.28");
    */
    update_wanted_temp_value(0);


    tft->setTextColor(DISPLAY_COLOR_GRAY);
    tft->setCursor(114, 45);
    tft->setInternalFont();
    tft->println("CURRENT");
    update_temp_value(0);

    /*font_console();
    tft->setTextColor(DISPLAY_COLOR_TEMP_LOWER);
    tft->setCursor(85, 53);
    tft->println("29.99");
    */
  }




  void update_timer_value(int value){
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



  void update_wanted_temp_value(float value){
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

  void font_frutiger(){
    tft->setFont(&frutiger);
  }

  void font_console(){
    tft->setFont(&console);
  }


};
