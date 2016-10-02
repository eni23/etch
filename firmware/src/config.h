// wirning

// Display
// CLK and MOSI cannot be changed sice we use hardware-spi
// DISPLAY_PIN_CLK      D5
// DISPLAY_PIN_MOSI     D7
#define DISPLAY_PIN_CS  D0
#define DISPLAY_PIN_DC  D8

// DS18B20 temperature sensor
#define DS18B20_PIN     D1
#define PCF8574_PIN_INT D4
#define I2C_PIN_SDA     D2
#define I2C_PIN_SCL     D3


// I2C port expander encoder
#define PCF8574_ENC_ADDR   32

#define ENC_TIMER_PORT_BTN  0
#define ENC_TIMER_PORT_UP   1
#define ENC_TIMER_PORT_DOWN 2

#define ENC_TEMP_PORT_BTN   3
#define ENC_TEMP_PORT_UP    4
#define ENC_TEMP_PORT_DOWN  5

// I2C port expander relais
#define PCF8574_REL_ADDR   33
#define REL_TIMER           0
#define REL_HEATER          1
#define REL_PUMP            2


// serial port baudrate
#define SERIAL_BAUD               115200

// configuration version (required to locate config in eeprom)
#define CONFIG_VERSION            "12c"

// start address for config-struct in eeprom
#define CONFIG_START               0
