/*******************************************************************************
 *
 * etch-timer Firmware
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
#include "debouncer.cpp"
#include "display_chars.cpp"
#include "ds18b20obj.cpp"

// hardware wiring
#define GPIO_ENCODER_UP           D5
#define GPIO_ENCODER_DOWN         D6
#define GPIO_ENCODER_BUTTON       D7
#define GPIO_RELAY                D8
#define GPIO_DISPLAY_SDA          D0
#define GPIO_DISPLAY_SCL          D1

// serial port baudrate
#define SERIAL_BAUD               115200

// maximal serial input line length
#define SERIAL_INPUT_BUFFER_SIZE  256

// configuration version (required to locate config in eeprom)
#define CONFIG_VERSION            "002"

// start address for config-struct in eeprom
#define CONFIG_START               64


// global variables
boolean is_running = false;
int countdown_timer;
int eeprom_save_timer;
int temp_timer;
int current_time = 100;
int temp_read_delay = 2500;
Adafruit_ssd1306syp display(GPIO_DISPLAY_SDA, GPIO_DISPLAY_SCL);
SimpleTimer timer;
SimpleTimer timer_temp;

float current_temp;
bool lock_display = false;

Debouncer btn_debounce(200);
Debouncer enc_debounce(10);
DS18B20 temp;

// settings struct stored in eeprom, default values only set if no config found
struct eeprom_config_struct {
  char version[4];
  int step_size;
  int time;
  boolean debug_output_enabled;
} eeprom_config = {
  CONFIG_VERSION,
  1,
  42,
  true
};


/**
 * start / stop the timer
 **/

void run_timer(){
  if (!is_running){
    digitalWrite(GPIO_RELAY, 1);
    countdown_timer = timer.setInterval(100, countdown_minus);
    is_running = true;
  }
}

void stop_timer(){
  if (is_running){
    is_running = false;
    timer.deleteTimer(countdown_timer);
    digitalWrite(GPIO_RELAY, 0);
    update_display();
  }
}


/**
 * Save time value to eeprom
 **/
void save_time(){
  eeprom_config.time = current_time;
  eeprom_save_config();
}


/**
 * User rotates encoder
 **/
void encoder_move(){
  if (is_running){
    return;
  }
  if (!enc_debounce.check()){
    return;
  }
  if (digitalRead(GPIO_ENCODER_UP) == digitalRead(GPIO_ENCODER_DOWN)){
    current_time -= eeprom_config.step_size;
  }
  else {
    current_time += eeprom_config.step_size;
  }
  timer.deleteTimer(eeprom_save_timer);
  eeprom_save_timer = timer.setTimeout(350, save_time);
  update_display();
}



/**
 * User pushes encoder
 **/
void encoder_push(){

  // debounce first
  if (!btn_debounce.check()){
    return;
  }

  if (current_time<1){
    current_time = eeprom_config.time;
    update_display();
    return;
  }

  if (is_running){
    stop_timer();
  }
  else {
    run_timer();
  }
}



/**
 * Function gets called every 10/s when timer is running
 **/
void countdown_minus(){
  current_time -= 1;

  // timer finished
  if (current_time<=0){
    stop_timer();
  }
  update_display();
}



void start_temp_read(){
  temp.read();
}

void print_trend_status(int trendn){
  if (trendn == 2){
    Serial.print("rising");
  }
  else if (trendn == 1){
    Serial.print("falling");
  }
  else {
    Serial.print("unknown");
  }
}

void temp_cb(float ct){
  current_temp = ct;
  //Serial.println("hist:");
  /*for (int i = 0; i<16; i++){
    Serial.print(i);
    Serial.print("=");
    temp.get_buffer(i);
  }

  int td = temp.get_trend_divi();
  int tc = temp.get_trend_count();
  int tcw = temp.get_trend_countw();

  Serial.print("divi=");
  print_trend_status(td);
  Serial.print(" count=");
  print_trend_status(tc);
  Serial.print(" count_weight=");
  print_trend_status(tcw);
  Serial.println("");*/

  update_display();
}

/**
 * Update display with current values
 **/
void update_display(){
  if (lock_display){
    return;
  }

  display.clear();
  display.setTextSize(2);
  display.setCursor(0,0);
  //display.drawBitmap(0, 10, big_star, 64, 64, WHITE );
  display.println((float)current_temp);
  if (is_running){
    display.drawBitmap(111, 0, sym_lamp, 16, 16, WHITE );
  }
  //else {
  //  display.drawBitmap(80, 0, sym_smile, 32, 16, WHITE );
  //}
  display.setCursor(0,20);
  display.setTextSize(4);
  display.println((int)current_time);

  display.update();
}



/**
 * arduino-init function
 **/
void setup() {

  // setup gpio pins
  pinMode(GPIO_ENCODER_UP, INPUT_PULLUP);
  pinMode(GPIO_ENCODER_DOWN, INPUT_PULLUP);
  pinMode(GPIO_ENCODER_BUTTON, INPUT_PULLUP);
  pinMode(GPIO_RELAY, OUTPUT);

  // setup interrupts for encoder and button
  attachInterrupt(GPIO_ENCODER_DOWN, encoder_move, FALLING);
  attachInterrupt(GPIO_ENCODER_BUTTON, encoder_push, RISING);
  sei();

  // setup uart
  Serial.begin( SERIAL_BAUD );

  // setup eeprom
  EEPROM.begin( sizeof( eeprom_config ) + CONFIG_START );
  delay( 50 );
  eeprom_load_config();
  current_time = eeprom_config.time;

  // setup display
  display.initialize();
  update_display();

  // temp
  temp_timer = timer_temp.setInterval(temp_read_delay, start_temp_read);

  temp.init(D4);
  temp.on_temp(temp_cb);
}


/**
 * arduino-main entry point after setup()
 **/
void loop() {
  // do serial stuff first
  if ( Serial.available() ) {
    serial_char_event();
  }
  temp.run();
  timer.run();
  timer_temp.run();
  //loop_ticks++;
  delay(1);
}
