#include <Arduino.h>
#include <OneWire.h>



typedef void (*DS18B20Callback) (float temp);


class DS18B20 {

	uint8_t i;
	uint8_t temp_data[ 12 ];
	int16_t raw_temp;
	float temp_c;
	bool read_is_active;
	uint8_t resolution;
	float buffer[16];
	uint8_t buffer_idx;
	OneWire* sensor;
	DS18B20Callback callback;

	void buffer_add(float value) {
		buffer[buffer_idx] = value;
	 	buffer_idx = (buffer_idx + 1) % 16;
	}

	void start_read(){
		if (read_is_active){
			return;
		}
		read_is_active = true;
	  byte resbyte = 0x1F;
	  if ( resolution == 12 ){
	    resbyte = 0x7F;
	  }
	  else if ( resolution == 11 ) {
	    resbyte = 0x5F;
	  }
	  else if ( resolution == 10 ) {
	    resbyte = 0x3F;
	  }
	  sensor->reset();
	  sensor->skip();
	  sensor->write( 0x4E );
	  sensor->write( 0 );
	  sensor->write( 0 );
	  sensor->write( resbyte );
	  sensor->write( 0x48 );
	  sensor->reset();
	  sensor->skip();
	  sensor->write( 0x44, 1 );
	}

	void read_temp(){
	  delay( 5 );
	  sensor->reset();
	  sensor->skip();
	  sensor->write( 0xBE );
	  for ( i = 0; i < 9; i++ ) {
	    temp_data[ i ] = sensor->read();
	  }
	  raw_temp = ( temp_data[ 1 ] << 8 ) | temp_data[ 0 ];
	  temp_c = (float) raw_temp / 16.0;
		read_is_active = false;
		buffer_add(temp_c);
		callback(temp_c);
		read_is_active = false;
	}

	public:
  DS18B20(int onewire_pin, uint8_t res = 10) {
		sensor = new OneWire(onewire_pin);
		resolution = res;
  }

	void on_temp(DS18B20Callback cbx){
		callback = cbx;
	}

	void set_resolution(uint8_t res){
		resolution = res;
	}

	void read() {
		start_read();
	}

  void run(){
		if (sensor->read() && read_is_active){
			read_temp();
		}
  }

};
