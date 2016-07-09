/*******************************************************************************
 *
 * WaterRouter Wifi Firmware
 *
 * Author: Cyrill vW. < eni@e23.ch >
 *
 * This file needs to be compiled with Arduino-esp8266,
 * Website: https://github.com/esp8266/Arduino
 *
 ******************************************************************************/

#include <EEPROM.h>
#include <SimpleTimer.h>
#include <Adafruit_ssd1306syp.h>

// wiring
#define GPIO_ENCODER_UP       D5
#define GPIO_ENCODER_DOWN     D6
#define GPIO_ENCODER_BUTTON   D7
#define GPIO_RELAY            D8
#define GPIO_DISPLAY_SDA      D0
#define GPIO_DISPLAY_SCL      D1

// serial port baudrate
#define SERIAL_BAUD 115200
// maximal serial input line length
#define SERIAL_INPUT_BUFFER_SIZE 256
// configuration version (required to locate config in eeprom)
#define CONFIG_VERSION "003"
// start address for config-struct in eeprom
#define CONFIG_START 64

// display stuff
Adafruit_ssd1306syp display(GPIO_DISPLAY_SDA, GPIO_DISPLAY_SCL);

// timer stuff
SimpleTimer timer;

// global vars
boolean debug_output_enabled = true;
boolean is_running = false;
float step_size = 1;
int countdown_timer;
int current_time = 100;

// settings struct stored in eeprom, default values only set if no config found
struct eeprom_config_struct {
  char version[4];
  int step_size;
  int timer;
  boolean debug_output_enabled;
} eeprom_config = {
  CONFIG_VERSION,
  1,
  100,
  true
};

/**
 * User rotates encoder
 **/
void encoder1_move(){
  if (is_running){
    return;
  }
  if (digitalRead(GPIO_ENCODER_UP) == digitalRead(GPIO_ENCODER_DOWN)){
    current_time -= step_size;
  }
  else {
    current_time += step_size;
  }
  update_display();
}



/**
 * User pushes encoder
 **/
void encoder1_push(){
  if (current_time<1){
    timer.deleteTimer(countdown_timer);
    is_running = false;
    return;
  }

  if (is_running){
    is_running = false;
    timer.deleteTimer(countdown_timer);
    digitalWrite(GPIO_RELAY, 0);
  }
  else {
    digitalWrite(GPIO_RELAY, 1);
    countdown_timer = timer.setInterval(100, countdown_minus);
    is_running = true;
  }
}



/**
 * Function got called every 10/s when timer is running
 **/
void countdown_minus(){
  current_time -= 1;
  if (current_time<0){
    digitalWrite(GPIO_RELAY, 0);
    current_time = 0;
    timer.deleteTimer(countdown_timer);
    is_running = false;
  }
  update_display();
}



/**
 * Update display with current values
 **/
void update_display(){
  display.clear();
  display.setTextSize(2);
  display.setCursor(0,0);
  display.println((int)current_time);
  display.setTextSize(4);
  display.println((int)current_time);
  display.update();
}



/**
 * arduino-init function
 **/
void setup() {

  pinMode(GPIO_ENCODER_UP, INPUT_PULLUP);
  pinMode(GPIO_ENCODER_DOWN, INPUT_PULLUP);
  pinMode(GPIO_ENCODER_BUTTON, INPUT_PULLUP);
  pinMode(GPIO_RELAY, OUTPUT);

  attachInterrupt(GPIO_ENCODER_DOWN, encoder1_move, FALLING);
  attachInterrupt(GPIO_ENCODER_BUTTON, encoder1_push, FALLING);
  sei();
  Serial.begin( SERIAL_BAUD );

  EEPROM.begin( sizeof( eeprom_config ) + CONFIG_START );
  delay( 50 );
  eeprom_load_config();

  display.initialize();
  update_display();
}


/**
 * arduino-main entry point after setup()
 **/
void loop() {
  // do serial stuff first
  if ( Serial.available() ) {
    serial_char_event();
  }
  timer.run();
  //loop_ticks++;
  delay(1);
}
