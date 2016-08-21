#include <SPI.h>
#include <TFT_ST7735.h>
#include <EEPROM.h>
#include <SimpleTimer.h>
#include "debouncer.cpp"
#include "pcf8574encoder.cpp"

#include "serial-term/SerialTerm.cpp"
#include <pcf8574_esp.h>

#include "_fonts/mono_mid.c"


/*
 DISPLAY_PIN_CLK  D5
 DISPLAY_PIN_MOSI D7
*/


// wirning
// DISPLAY_PIN_CLK      D5
// DISPLAY_PIN_MOSI     D7
#define DISPLAY_PIN_CS  D8
#define DISPLAY_PIN_DC  D1
#define PCF8574_PIN_INT D4
#define PCF8574_PIN_SDA D2
#define PCF8574_PIN_SCL D3

#define PCF8574_ADDR    32

// serial port baudrate
#define SERIAL_BAUD               115200

// configuration version (required to locate config in eeprom)
#define CONFIG_VERSION            "002"

// start address for config-struct in eeprom
#define CONFIG_START               64





TFT_ST7735 tft = TFT_ST7735(DISPLAY_PIN_CS, DISPLAY_PIN_DC);
PCF8574 pcf8574(PCF8574_ADDR, PCF8574_PIN_SDA, PCF8574_PIN_SCL);


int countdown_timer;
SimpleTimer timer;
SerialTerm term;
Debouncer btn_debounce(10);
Debouncer enc_debounce(10);
PCF8574Encoder test_encoder(0,1,2);

int current_time = 0;
int last_current_time = 0;
bool is_running = false;
uint16_t timer_disp_color = GREEN;

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
  tft.setCursor(0, 0);
  tft.setTextColor(BLACK);
  tft.print(format_time(last_current_time));
  tft.setCursor(0, 0);
  tft.setTextColor(timer_disp_color);
  tft.print(format_time());
}






void setup() {

  pinMode(PCF8574_PIN_INT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PCF8574_PIN_INT), pcf_int, CHANGE);
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

  tft.begin();
  tft.setTextScale(2);
  tft.setFont(&mono_mid);
  tft.setTextColor(GREEN);
  tft.println(format_time());
  tft.setTextColor(RED);
  tft.println("00:00");
  tft.setTextColor(GREEN);

}



void loop(void) {
  if (pcf_data_incoming){
    test_encoder.process(pcf_data);
    pcf_data_incoming = false;
  }
  timer.run();
  term.run();
}
