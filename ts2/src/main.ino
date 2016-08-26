#include <EEPROM.h>
#include <SimpleTimer.h>
#include <pcf8574_esp.h>


#include "serial-term/SerialTerm.cpp"
#include "ds18b20obj.cpp"
#include "debouncer.cpp"
#include "pcf8574encoder.cpp"
#include "config.h"
#include "display.cpp"


PCF8574 pcf8574(PCF8574_ADDR, PCF8574_PIN_SDA, PCF8574_PIN_SCL);



int countdown_timer2;
int countdown_timer;
SimpleTimer timer;
SerialTerm term;
Debouncer btn_debounce(10);
Debouncer enc_debounce(10);
PCF8574Encoder test_encoder(0,1,2);
PCF8574Encoder test_encoder2(3,4,5);
DS18B20 temp;
SimpleTimer timer_temp;
int temp_timer;
int temp_read_delay = 2500;

int current_time = 0;
int last_current_time = 0;
bool is_running = false;
uint16_t timer_disp_color = GREEN;
float current_temp;
float last_current_temp;
uint16_t temp_disp_color = YELLOW;


int e2_current_time = 0;
int e2_last_current_time = 0;
bool e2_is_running = false;
uint16_t timer2_disp_color = RED;


bool pcf_data_incoming = false;
uint8_t pcf_data;

void pcf_int(){
  pcf_data_incoming = true;
  pcf_data = pcf8574.read8();
}


void countdown_minus(){
  last_current_time = current_time;
  current_time -= 1;
  // timer finished
  if (current_time<=0){
    stop_timer();
  }
  update_display();
}


void run_timer(){
  if (!is_running){
    countdown_timer = timer.setInterval(1000, countdown_minus);
    timer_disp_color=CYAN;
    is_running = true;
  }
}

void stop_timer(){
  if (is_running){
    timer_disp_color = GREEN;
    is_running = false;
    timer.deleteTimer(countdown_timer);
    update_display();
  }
}

void countdown_minus2(){
  e2_last_current_time = e2_current_time;
  e2_current_time -= 1;
  // timer finished
  if (e2_current_time<=0){
    stop_timer2();
  }
  update_display2();
}


void run_timer2(){
  if (!e2_is_running){
    countdown_timer2 = timer.setInterval(1000, countdown_minus2);
    timer2_disp_color=MAGENTA;
    e2_is_running = true;
  }
}

void stop_timer2(){
  if (e2_is_running){
    timer2_disp_color = RED;
    e2_is_running = false;
    timer.deleteTimer(countdown_timer2);
    update_display2();
  }
}


void start_temp_read(){
  temp.read();
}

void temp_cb(float ct){
}

void log( const char* message ) {
  term.debug(message);
}

String format_time(int wt = -1){
  if (wt<0){
    wt=current_time;
  }
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


void update_display(){
  String ts_old = format_time(last_current_time);
  String ts_new = format_time();
  tft.setCursor(0, 0);
  for (int i = 0; i < ts_old.length(); i++){
    if ( (ts_old.charAt(i) != ts_new.charAt(i) ) ||
         ((current_time % 60)==0) || ((last_current_time % 60)==0) ) {
      tft.setTextColor(BLACK);
    }
    else {
      tft.setTextColor(timer_disp_color);
    }
    tft.print(ts_old.charAt(i));
  }
  tft.setCursor(0, 0);
  tft.setTextColor(timer_disp_color);
  tft.print(format_time());
}





uint16_t cc2_col = RED;
void update_display2(){
  String ts_old = format_time(e2_last_current_time);
  String ts_new = format_time(e2_current_time);
  tft.setCursor(0, 0);
  tft.println();
  for (int i = 0; i < ts_old.length(); i++){
    if ( (ts_old.charAt(i) != ts_new.charAt(i) ) ||
         ((e2_current_time % 60)==0) || ((e2_last_current_time % 60)==0) ) {
      tft.setTextColor(BLACK);
    }
    else {
      tft.setTextColor(timer2_disp_color);
    }
    tft.print(ts_old.charAt(i));
  }
  tft.setCursor(0, 0);
  tft.println();
  tft.setTextColor(timer2_disp_color);
  tft.print(ts_new);
}


String format_temp(float tt){
  String str_tt = String(tt);
  str_tt+=" °C";
  return str_tt;
}


void update_display_temp(){
  String ts_old = format_temp(last_current_temp);
  String ts_new = format_temp(current_temp);
  tft.setCursor(0, 0);
  tft.println();
  tft.println();
  for (int i = 0; i < ts_old.length(); i++){
    if ( (ts_old.charAt(i) != ts_new.charAt(i) ) ) {
      tft.setTextColor(BLACK);
    }
    else {
      tft.setTextColor(temp_disp_color);
    }
    tft.print(ts_old.charAt(i));
  }
  tft.setCursor(0, 0);
  tft.println();
  tft.println();
  tft.setTextColor(temp_disp_color);
  tft.print(format_temp(current_temp));
}


void setup() {

  pinMode(PCF8574_PIN_INT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PCF8574_PIN_INT), pcf_int, CHANGE);

  attachInterrupt(digitalPinToInterrupt(D6), [](){
    log("ID6");
  }, CHANGE);

  sei();

  term.begin(115200);

  test_encoder.up([](){
    last_current_time = current_time;
    current_time++;
    update_display();
  });

  test_encoder.down([](){
    last_current_time = current_time;
    if (current_time>0){
      current_time--;
      update_display();
    }
  });
  test_encoder.button([](){
    if (current_time<1){
      return;
    }
    if (is_running){
      stop_timer();
    }
    else {
      run_timer();
      update_display();
    }
  });

  test_encoder2.up([](){
    e2_last_current_time = e2_current_time;
    e2_current_time++;
    update_display2();
  });

  test_encoder2.down([](){
    e2_last_current_time = e2_current_time;
    if (e2_current_time>0){
      e2_current_time--;
      update_display2();
    }
  });
  test_encoder2.button([](){
    if (e2_current_time<1){
      return;
    }
    if (e2_is_running){
      stop_timer2();
    }
    else {
      run_timer2();
      update_display2();
    }
  });

  term.on("t1", [](){
    int wt=term.intArg(0);
    last_current_time = current_time;
    current_time = wt;
    update_display();
  });

  term.on("t2", [](){
    int wt=term.intArg(0);
    e2_last_current_time = e2_current_time;
    e2_current_time = wt;
    update_display2();
  });

  temp_timer = timer_temp.setInterval(temp_read_delay, start_temp_read);
  temp.init(DS18B20_PIN);
  temp.on_temp([](float t_new){
      last_current_temp = current_temp;
      current_temp = t_new;
      update_display_temp();
  });

  tft.begin();
  tft.setTextScale(1.5);
  set_font();
  tft.setTextColor(GREEN);
  tft.println(format_time());
  tft.setTextColor(RED);
  tft.println("00:00");
  tft.setTextColor(GREEN);

}



void loop(void) {
  if (pcf_data_incoming){
    test_encoder.process(pcf_data);
    test_encoder2.process(pcf_data);
    pcf_data_incoming = false;
  }
  timer.run();
  timer_temp.run();
  term.run();
  temp.run();
}
