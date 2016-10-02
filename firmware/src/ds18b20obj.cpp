#include <Arduino.h>
#include <OneWire.h>
#include <stdio.h>
#include <string.h>



typedef void (*DS18B20Callback) (float temp);
typedef void (*DS18B20CallbackError) ();


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
	DS18B20CallbackError error_callback;
	DS18B20CallbackError error_fixed_callback;

	void buffer_add(float value) {
		for (int k = 16; k >0; k--){
		  buffer[k]=buffer[k-1];
		}
		buffer[0] = value;
		//buffer[buffer_idx] = value;
	 	//buffer_idx = (buffer_idx + 1) % 16;
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
		bool was_ee = is_error;
		bool ee = true;
	  for ( i = 0; i < 9; i++ ) {
	    temp_data[ i ] = sensor->read();
			if (temp_data[i]!=255) {
				ee = false;
			}
	  }
		if (ee){
			is_error = true;
			read_is_active = false;
			error_callback();
			return;
		}
		else {
			is_error = false;
		}
		if (was_ee){
			error_fixed_callback();
		}

	  raw_temp = ( temp_data[ 1 ] << 8 ) | temp_data[ 0 ];
	  temp_c = (float) raw_temp / 16.0;
		buffer_add(temp_c);
		callback(temp_c);
		read_is_active = false;
	}

	public:

	bool is_error = false;

  DS18B20() {
  }

	void init(int onewire_pin, uint8_t res = 10){
		sensor = new OneWire(onewire_pin);
		resolution = res;
		// avoid segfaults
		callback = [](float t){ return; };
		error_callback = [](){ return; };
		error_fixed_callback = [](){ return; };
	}

	void on_temp(DS18B20Callback cbx){
		callback = cbx;
	}

	void on_error(DS18B20CallbackError cbx){
		error_callback = cbx;
	}

	void on_error_fixed(DS18B20CallbackError cbx){
		error_fixed_callback = cbx;
	}

	void set_resolution(uint8_t res){
		resolution = res;
	}

	void read() {
		start_read();
	}

	void get_buffer(uint8_t index){
		Serial.println( buffer[index] );
	}


  void run(){
		if (sensor->read() && read_is_active){
			read_temp();
		}
  }

};
