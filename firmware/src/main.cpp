#include "Arduino.h"
#include <EEPROM.h>
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
DS18B20 temp;
SerialTerm term;

SimpleTimer timer;
SimpleTimer timer_eeprom;
SimpleTimer timer_temp;

int temp_timer;
int countdown_timer;
int eeprom_save_timer;
bool pcf_data_incoming = false;
uint8_t pcf_data;
bool the_heat_is_on = false;

struct eeprom_config_struct {
  char version[4];
  int timer_value;
  int timer_step_size;
  bool timer_is_running;
  float wanted_temp;
  float temp_step_size;
  bool ts_is_running;
  bool ts_is_warmup;
  float temp_grace_value;
  uint8_t temp_precision;
  int temp_read_delay;
} eeprom_config = {
  CONFIG_VERSION,
  200,
  1,
  false,
  42.00,
  0.25,
  false,
  false,
  3.0,
  10,
  2500
};


void eeprom_load_config() {
  // If nothing is found we load default settings.
  if ( EEPROM.read( CONFIG_START + 0 ) == CONFIG_VERSION[0] &&
       EEPROM.read( CONFIG_START + 1 ) == CONFIG_VERSION[1] &&
       EEPROM.read( CONFIG_START + 2 ) == CONFIG_VERSION[2] ) {
    for ( unsigned int t = 0; t < sizeof( eeprom_config ); t++ ) {
      *( ( char* ) &eeprom_config + t ) = EEPROM.read( CONFIG_START + t );
    }
  }
}

/**
 * save eeprom-struct
 **/
void eeprom_save_config() {
  for ( unsigned int t = 0; t < sizeof( eeprom_config ); t++ ) {
    EEPROM.write( CONFIG_START + t, *( ( char* ) &eeprom_config + t ) );
  }
  EEPROM.commit();
}


void eeprom_save_config_delayed(){
  timer_eeprom.deleteTimer(eeprom_save_timer);
  eeprom_save_timer = timer_eeprom.setTimeout(550, eeprom_save_config);
}


void pcf_int(){
  pcf_data_incoming = true;
  pcf_data = pcf8574_enc.read8();
}

void start_temp_read(){
  temp.read();
}


void stop_timer(){
  if (eeprom_config.timer_is_running){
    pcf8574_rel.write(REL_TIMER, 1);
    display.timer_is_running = false;
    eeprom_config.timer_is_running = false;
    timer.deleteTimer(countdown_timer);
    display.update_timer_value(eeprom_config.timer_value);
    eeprom_save_config_delayed();
  }
}

void run_timer(){
  if (!eeprom_config.timer_is_running){

    pcf8574_rel.write(REL_TIMER, 0);

    display.timer_is_running = true;
    countdown_timer = timer.setInterval(1000, [](){
      eeprom_config.timer_value -= 1;
      if (eeprom_config.timer_value<=0){
        stop_timer();
      }
      display.update_timer_value(eeprom_config.timer_value);
      eeprom_save_config_delayed();
    });
    eeprom_config.timer_is_running = true;
    display.update_timer_value(eeprom_config.timer_value);
    eeprom_save_config_delayed();
  }
}


void start_thermostat(){
  if (eeprom_config.ts_is_running){
    return;
  }
  pcf8574_rel.write(REL_HEATER, 0);
  the_heat_is_on = true;
  eeprom_config.ts_is_running = true;
  eeprom_config.ts_is_warmup = true;
  eeprom_save_config_delayed();
  term.debug("ts start");

}


void stop_thermostat(){
  if (!eeprom_config.ts_is_running){
    return;
  }
  pcf8574_rel.write(REL_PUMP, 1);
  pcf8574_rel.write(REL_HEATER, 1);
  eeprom_config.ts_is_running = false;
  eeprom_config.ts_is_warmup = false;
  the_heat_is_on = false;
  eeprom_save_config_delayed();
  term.debug("ts stop");
}


void process_temp(float t_new){
  display.update_temp_value(t_new);
  if (eeprom_config.ts_is_running){
    if (eeprom_config.ts_is_warmup){
      if (t_new>=eeprom_config.wanted_temp){
        pcf8574_rel.write(REL_PUMP, 0);
        term.debug("start pump");
        eeprom_config.ts_is_warmup = false;
        eeprom_save_config_delayed();
        return;
      }
    }
    else {

      if (t_new>(eeprom_config.wanted_temp + eeprom_config.temp_grace_value)){
          if (the_heat_is_on){
            term.debug("turn off heat");
            pcf8574_rel.write(REL_HEATER, 1);
            the_heat_is_on = false;
          }
      }
      else if (t_new<(eeprom_config.wanted_temp - eeprom_config.temp_grace_value)){
          if (!the_heat_is_on){
            term.debug("turn on heat");
            pcf8574_rel.write(REL_HEATER, 0);
            the_heat_is_on = true;
          }
      }

    }

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

  term.on("info", [](){
    term.printf("timer_value = %i\n\r", eeprom_config.timer_value );
    term.printf("timer_is_running = %s\n\r", eeprom_config.timer_is_running ? "true" : "false" );
    term.printf("timer_step_size = %i\n\r", eeprom_config.timer_step_size );
    Serial.print("temp_wanted = ");
    Serial.println(eeprom_config.wanted_temp);
    term.printf("temp_is_running = %s\n\r", eeprom_config.ts_is_running ? "true" : "false" );
    term.printf("temp_warmup_is_running = %s\n\r", eeprom_config.ts_is_running ? "true" : "false" );
    Serial.print("temp_step_size = ");
    Serial.println(eeprom_config.temp_step_size);
    Serial.print("temp_grace_value = ");
    Serial.println(eeprom_config.temp_grace_value);
    term.printf("temp_precision = %i\n\r", eeprom_config.temp_precision );
    term.printf("temp_read_delay = %i\n\r", eeprom_config.temp_read_delay );
  });


  term.on("set-temp-resolution", [](){
    uint8_t precision = term.intArg(0);
    if (precision<9 || precision>12){
      term.debug("ERROR: precision needs to be between 9 and 12");
      return;
    }
    temp.set_resolution(precision);
    eeprom_save_config();
  });

  term.on("set-temp-delay", [](){
    int delay = term.intArg(0);
    if (delay<250){
      term.debug("ERROR: delay needs to be bigger than 250ms");
      return;
    }
    eeprom_config.temp_read_delay = delay;
    timer_temp.deleteTimer(temp_timer);
    temp_timer = timer_temp.setInterval(eeprom_config.temp_read_delay, start_temp_read);
    eeprom_save_config();
  });

  term.on("set-temp-grace-value", [](){
    float new_val = term.floatArg(0);
    if (new_val<0.1){
      term.debug("ERROR: value needs to be bigger than 0.09C");
      return;
    }
    eeprom_config.temp_grace_value = new_val;
    eeprom_save_config();
  });

  term.on("set-temp-stepsize", [](){
    float new_val = term.floatArg(0);
    if (new_val<0.01){
      term.debug("ERROR: value needs to be bigger than 0.009C");
      return;
    }
    eeprom_config.temp_step_size = new_val;
    eeprom_save_config();
  });

  term.on("set-timer-stepsize", [](){
    int new_val = term.intArg(0);
    if (new_val<1){
      term.debug("ERROR: value needs to be bigger than 0.009C");
      return;
    }
    eeprom_config.timer_step_size = new_val;
    eeprom_save_config();
  });

}


void setup() {

  pinMode(PCF8574_PIN_INT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PCF8574_PIN_INT), pcf_int, CHANGE);
  sei();

  display.init();
  init_term();

  timer_encoder.up([](){
    display.tick_timer_encoder();
    eeprom_config.timer_value += eeprom_config.timer_step_size;
    display.update_timer_value(eeprom_config.timer_value);
    eeprom_save_config_delayed();
  });

  timer_encoder.down([](){
    display.tick_timer_encoder();
    if (eeprom_config.timer_value>0){
      eeprom_config.timer_value -= eeprom_config.timer_step_size;
      display.update_timer_value(eeprom_config.timer_value);
      eeprom_save_config_delayed();
    }
  });

  timer_encoder.button_longpress([](){
    if (eeprom_config.timer_value<1){
      return;
    }
    if (eeprom_config.timer_is_running){
      stop_timer();
    }
    else {
      run_timer();
    }
  });
  timer_encoder.button_shortpress([](){
    if (eeprom_config.timer_is_running){
      stop_timer();
    }
  });


  temp_encoder.down([](){
    display.tick_temp_encoder();
    if (eeprom_config.wanted_temp>0){
      eeprom_config.wanted_temp -= eeprom_config.temp_step_size;
      display.update_wanted_temp_value(eeprom_config.wanted_temp);
      eeprom_save_config_delayed();
    }
  });

  temp_encoder.up([](){
    display.tick_temp_encoder();
    eeprom_config.wanted_temp += eeprom_config.temp_step_size;
    display.update_wanted_temp_value(eeprom_config.wanted_temp);
    eeprom_save_config_delayed();
  });

  temp_encoder.button_longpress([](){
    if (!eeprom_config.ts_is_running){
      start_thermostat();
    }
    else {
      stop_thermostat();
    }
  });

  temp_encoder.button_shortpress([](){
    stop_thermostat();
  });

  temp_timer = timer_temp.setInterval(eeprom_config.temp_read_delay, start_temp_read);
  temp.init(DS18B20_PIN, eeprom_config.temp_precision);
  temp.on_temp([](float t_new){
    process_temp(t_new);
  });


  EEPROM.begin( sizeof( eeprom_config ) + CONFIG_START );
  eeprom_load_config();

  display.update_wanted_temp_value(eeprom_config.wanted_temp);
  if (eeprom_config.ts_is_running){
    eeprom_config.ts_is_running = false;
    start_thermostat();
  }

  display.update_timer_value(eeprom_config.timer_value);
  if (eeprom_config.timer_is_running){
    eeprom_config.timer_is_running = false;
    run_timer();
  }
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
  timer_eeprom.run();
  timer_temp.run();

  if (timer_encoder.button_press_active){
    timer_encoder.process_button(pcf8574_enc.read8());
  }
  if (temp_encoder.button_press_active){
    temp_encoder.process_button(pcf8574_enc.read8());
  }
}
