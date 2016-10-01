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
bool pcf_data_incoming = false;
DS18B20 temp;
SimpleTimer timer;
SimpleTimer timer_eeprom;
SimpleTimer timer_temp;
int temp_timer;
int temp_read_delay = 2500;
int countdown_timer;
int eeprom_save_timer;

uint8_t pcf_data;
SerialTerm term;


struct eeprom_config_struct {
  char version[4];
  int timer_value;
  int timer_step_size;
  bool timer_is_running;
  float wanted_temp;
  float temp_step_size;
  bool ts_is_running;
  bool ts_is_warmup;
} eeprom_config = {
  CONFIG_VERSION,
  200,
  1,
  false,
  42.00,
  0.25,
  false,
  false,
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
  term.debug("saved");
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

void init_term(){
  term.begin(115200);
  term.on("show-menu", [](){
    display.show_menu();
  });

  term.on("hide_menu", [](){
    display.hide_menu();
  });

  term.on("info", [](){
    Serial.print("wanted_temp = ");
    Serial.println(eeprom_config.wanted_temp);
    term.printf("timer_value = %i\n\r", eeprom_config.timer_value );
  });

  term.on("load-info", [](){
    eeprom_load_config();
    Serial.print("wanted_temp = ");
    Serial.println(eeprom_config.wanted_temp);
    term.printf("timer_value = %i\n\r", eeprom_config.timer_value );
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
    eeprom_config.timer_value+=eeprom_config.timer_step_size;
    display.update_timer_value(eeprom_config.timer_value);
    eeprom_save_config_delayed();
  });

  timer_encoder.down([](){
    display.tick_timer_encoder();
    if (eeprom_config.timer_value>0){
      eeprom_config.timer_value-=eeprom_config.timer_step_size;
      display.update_timer_value(eeprom_config.timer_value);
      //eeprom_save_config();
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
    pcf8574_rel.toggle(REL_PUMP);
    term.debug("toggle pump");
  });

  temp_encoder.button_shortpress([](){
    pcf8574_rel.toggle(REL_HEATER);
    term.debug("toggle heater");
  });

  temp_timer = timer_temp.setInterval(temp_read_delay, start_temp_read);
  temp.init(DS18B20_PIN);
  temp.on_temp([](float t_new){
    display.update_temp_value(t_new);
  });


  EEPROM.begin( sizeof( eeprom_config ) + CONFIG_START );
  eeprom_load_config();

  display.update_wanted_temp_value(eeprom_config.wanted_temp);

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
