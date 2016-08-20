#include <SPI.h>
#include <TFT_ST7735.h>
#include <EEPROM.h>
#include "debouncer.cpp"
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

SerialTerm term;
Debouncer btn_debounce(10);
Debouncer enc_debounce(10);




bool pcf_data_incoming = false;
uint8_t pcf_data = 0;

int wanted_temp = 0;


void pcf_int(){
  pcf_data_incoming = true;
  pcf_data = pcf8574.read8();
}



void log( const char* message ) {
  term.debug(message);
}

void setup() {

  pinMode(PCF8574_PIN_INT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PCF8574_PIN_INT), pcf_int, CHANGE);
  sei();

  term.begin(115200);

  term.on("foo", []() {
    term.debug("bar");
  });
  tft.begin();
  tft.setTextScale(2);
  tft.setFont(&mono_mid);
  tft.setTextColor(GREEN);
  tft.println(wanted_temp);
}


uint8_t get_state(uint8_t _gs_states, uint8_t pos){
  return (_gs_states & (1<<pos)) > 0;
}



uint8_t enc_frame_num = 0;
bool enc_frame_active = false;
uint8_t enc_frame_data[3];


void loop(void) {
  term.run();

  if (pcf_data_incoming){



    uint8_t enc_plus = get_state(pcf_data, 1);
    uint8_t enc_minus = get_state(pcf_data, 2);

    //uint8_t enc_minus = (pcf_data & (1<<2)) > 0;


    // start of a 4bit encoder frame
    if ((enc_plus<1 || enc_minus<1) && !enc_frame_active){
      enc_frame_active = true;
    }

    if (enc_frame_active){
      enc_frame_data[enc_frame_num] = pcf_data;
      enc_frame_num++;
      if (enc_frame_num>2){
        enc_frame_num=0;
        enc_frame_active=false;


        // process encoder frames
        bool frame_valid = true;
        uint8_t enc_dir = 0; // 0=invalid, 1=plus 2=minus
        for (uint8_t i=0; i<3; i++){
          uint8_t frm_plus  = get_state(enc_frame_data[i], 1);
          uint8_t frm_minus = get_state(enc_frame_data[i], 2);
          switch (i){
            case 0:
              if (frm_plus>frm_minus){
                enc_dir=1;
              }
              else if (frm_minus>frm_plus){
                enc_dir=2;
              }
              break;
            case 1:
              if (frm_minus!=0 || frm_plus!=0){
                enc_dir=0;
              }
              break;
            case 2:
              if ( (enc_dir==1 && frm_plus!=0 && frm_minus!=1) ||
                   (enc_dir==2 && frm_plus!=1 && frm_minus!=0)) {
                enc_dir=0;
              }
              break;
          }
        }

        if (enc_dir>0){
          int old_wanted_temp = wanted_temp;
          if (enc_dir==1) {
            wanted_temp++;
          }
          else {
            if (wanted_temp>0) wanted_temp--;
          }
          tft.setCursor(0, 0);
          tft.setTextColor(BLACK);
          tft.print(old_wanted_temp);
          tft.setCursor(0, 0);
          tft.setTextColor(GREEN);
          tft.print(wanted_temp);
        }
        term.printf("encoder=%i\n\r", enc_dir);

      }
    }
    pcf_data_incoming = false;
  }

}
