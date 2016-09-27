#include <Arduino.h>
#include <OneWire.h>
#include <stdio.h>
#include <string.h>



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
  DS18B20() {
  }

	void init(int onewire_pin, uint8_t res = 10){
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

	void get_buffer(uint8_t index){
		Serial.println( buffer[index] );
	}

	int get_trend_divi(){
		float diff[16];
  	float tot = 0;
  	for (int i = 0; i<15; i++){
    	diff[i] = buffer[i+1] - buffer[i];
    	tot+=diff[i];
  	}
		tot = (tot/15);
		float tt = (buffer[16] - buffer[0] / 16);
		Serial.print("variance=");
		Serial.print(tt);
		Serial.print(" trend=");
		Serial.println(tot);

		if (tot>0){
			//falling
			return 1;
		}
		if (tot<0){
			//rising
			return 2;
		}
		//unclear
		return 0;
	}


	int get_trend_count(){
	  float base = buffer[0];
	  int upper = 0;
	  int lower = 0;
		for (int i=15; i>0; i--){
	    if (buffer[i]>base){
	      lower++;
	    }
	    if (buffer[i]<base){
	      upper++;
	    }
	  }
		if (upper>lower){
    	//rising
			return 2;
  	}
  	if (lower>upper){
    	//falling
			return 1;
  	}
		// unclear
		return 0;
	}



	int get_trend_countw(){
	  float base = buffer[0];
	  float upper=0;
	  float lower=0;
	  int g=1;
		for (int i=15; i>0; i--){
	    if (buffer[i]>base){
	      lower += ((float) (buffer[i]-base) * (1*g) );
	    }
	    if (buffer[i]<base){
	      upper += ((float) (base-buffer[i]) * (1*g) );
	    }
	    g++;
	  }

		if (upper > lower){
    	//rising
			return 2;
  	}
  	if (lower > upper){
			//falling
			return 1;
  	}
		// unclear
		return 0;

	}


  void run(){
		if (sensor->read() && read_is_active){
			read_temp();
		}
  }

};
