#include "Arduino.h"
#include "config.h"
#include "display.cpp"
#include <SimpleTimer.h>
#include <pcf8574_esp.h>


#include "serial-term/SerialTerm.cpp"
#include "ds18b20obj.cpp"
#include "pcf8574encoder.cpp"

PCF8574 pcf8574_enc(PCF8574_ENC_ADDR, I2C_PIN_SDA, I2C_PIN_SCL);
PCF8574 pcf8574_rel(PCF8574_REL_ADDR, I2C_PIN_SDA, I2C_PIN_SCL);


PCF8574Encoder timer_encoder(ENC_TIMER_PORT_BTN,ENC_TIMER_PORT_DOWN,ENC_TIMER_PORT_UP);
PCF8574Encoder temp_encoder(ENC_TEMP_PORT_BTN,ENC_TEMP_PORT_DOWN,ENC_TEMP_PORT_UP);

Display display;
int current_time = 0;
bool pcf_data_incoming = false;
float wanted_temp = 45.0;
DS18B20 temp;
SimpleTimer timer;
SimpleTimer timer_temp;
int temp_timer;
int temp_read_delay = 2500;
int countdown_timer;
bool timer_is_running = false;


uint8_t pcf_data;
SerialTerm term;


void pcf_int(){
  pcf_data_incoming = true;
  pcf_data = pcf8574_enc.read8();
}

void start_temp_read(){
  temp.read();
}


void stop_timer(){
  if (timer_is_running){
    pcf8574_rel.write(REL_TIMER, 1);
    display.timer_is_running = false;
    timer_is_running = false;
    timer.deleteTimer(countdown_timer);
    display.update_timer_value(current_time);
  }
}

void run_timer(){
  if (!timer_is_running){
    pcf8574_rel.write(REL_TIMER, 0);

    display.timer_is_running = true;
    countdown_timer = timer.setInterval(1000, [](){
      current_time -= 1;
      if (current_time<=0){
        stop_timer();
      }
      display.update_timer_value(current_time);

    });
    timer_is_running = true;
    display.update_timer_value(current_time);
  }
}

void init_term(){
  term.begin(115200);
  term.on("show-menu", [](){
    display.show_menu();
  });

  term.on("hide_menu", [](){
    display.hide_menu();
  });
}


void setup() {

  pinMode(PCF8574_PIN_INT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PCF8574_PIN_INT), pcf_int, CHANGE);
  sei();

  display.init();
  init_term();

  display.update_wanted_temp_value(wanted_temp);

  timer_encoder.up([](){
    display.tick_timer_encoder();
    current_time++;
    display.update_timer_value(current_time);
  });

  timer_encoder.down([](){
    display.tick_timer_encoder();
    if (current_time>0){
      current_time--;
      display.update_timer_value(current_time);
    }
  });

  timer_encoder.button_longpress([](){
    if (current_time<1){
      return;
    }
    if (timer_is_running){
      stop_timer();
    }
    else {
      run_timer();
    }
  });
  timer_encoder.button_shortpress([](){
    if (timer_is_running){
      stop_timer();
    }
  });


  temp_encoder.down([](){
    display.tick_temp_encoder();
    if (wanted_temp>0){
      wanted_temp=wanted_temp-0.25;
      display.update_wanted_temp_value(wanted_temp);
    }
  });

  temp_encoder.up([](){
    display.tick_temp_encoder();
    wanted_temp=wanted_temp+0.25;
    display.update_wanted_temp_value(wanted_temp);
  });

  temp_encoder.button_longpress([](){
    term.debug("long");
  });

  temp_encoder.button_shortpress([](){
    term.debug("short");
  });

  temp_timer = timer_temp.setInterval(temp_read_delay, start_temp_read);
  temp.init(DS18B20_PIN);
  temp.on_temp([](float t_new){
    display.update_temp_value(t_new);
  });

}



void loop() {
  if (pcf_data_incoming){
    timer_encoder.process(pcf_data);
    temp_encoder.process(pcf_data);
    pcf_data_incoming = false;
  }
  timer_temp.run();
  term.run();
  temp.run();
  timer.run();
  timer_temp.run();

  if (timer_encoder.button_press_active){
    timer_encoder.process_button(pcf8574_enc.read8());
  }
  if (temp_encoder.button_press_active){
    temp_encoder.process_button(pcf8574_enc.read8());
  }
}
