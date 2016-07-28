#include <EEPROM.h>
#include <SimpleTimer.h>
#include <Adafruit_ssd1306syp.h>
#include "debouncer.cpp"
#include "icons.cpp"
#include "serial-term/SerialTerm.cpp"


// hardware wiring
#define GPIO_ENCODER_UP           D5
#define GPIO_ENCODER_DOWN         D6
#define GPIO_ENCODER_BUTTON       D7
#define GPIO_RELAY                D8
#define GPIO_DISPLAY_SDA          D0
#define GPIO_DISPLAY_SCL          D1

// serial port baudrate
#define SERIAL_BAUD               115200

// configuration version (required to locate config in eeprom)
#define CONFIG_VERSION            "002"

// start address for config-struct in eeprom
#define CONFIG_START               64

// global variables
boolean is_running = false;
int countdown_timer;
int eeprom_save_timer;
int current_time = 100;
Adafruit_ssd1306syp display(GPIO_DISPLAY_SDA, GPIO_DISPLAY_SCL);
SimpleTimer timer;

Debouncer btn_debounce(200);
Debouncer enc_debounce(10);

SerialTerm term;

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


/**
 * Update display with current values
 **/
void update_display(){
  display.clear();
  display.setTextSize(2);
  display.setCursor(0,0);
  display.println(eeprom_config.time);
  if (is_running){
    display.drawBitmap(111, 0, sym_lamp_on, 16, 16, WHITE );
  }
  //else {
  //  display.drawBitmap(111, 0, sym_lamp_off, 16, 16, WHITE );
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
  term.begin(115200);

  // setup terminal functions
  term.on("step-size", [](){
    int ss = term.intArg(0);
    if (ss<1){
      term.printf( "error parsing arg1 as int\n\r");
      return;
    }
    eeprom_config.step_size = ss;
    term.printf("new step_size=%i", ss);
    eeprom_save_config();
  });

  term.on("set-time",[](){
    int tc = term.intArg(0);
    if (tc<1){
      term.printf("error parsing arg1 as int\n\r");
      return;
    }
    current_time = tc;
    save_time();
    update_display();
  });

  term.on("run", []() {
    if (is_running){
      term.debug("allready running");
    }
    else {
      if (current_time<=0){
        current_time = eeprom_config.time;
      }
      run_timer();
    }
    return;
  });

  term.on("stop", []() {
    if (!is_running){
      term.debug("timer is not running");
    }
    else {
      stop_timer();
    }
    return;
  });


  // setup eeprom
  EEPROM.begin( sizeof( eeprom_config ) + CONFIG_START );
  delay( 50 );
  eeprom_load_config();
  current_time = eeprom_config.time;

  // setup display
  display.initialize();
  update_display();
}


/**
 * arduino-main entry point after setup()
 **/
void loop() {
  // do serial stuff first
  timer.run();
  term.run();
  //loop_ticks++;
  delay(1);
}
