#include <Arduino.h>

typedef void (*PCF8574EncoderCallback) ();


class PCF8574Encoder {


	PCF8574EncoderCallback callback_up;
	PCF8574EncoderCallback callback_down;
	PCF8574EncoderCallback callback_button;

	bool unprocessed_frames = false;
	bool enc_is_inverse = false;
	uint8_t enc_port_btn;
	uint8_t enc_port_plus;
	uint8_t enc_port_minus;
	uint8_t enc_frame_num = 0;
	bool enc_frame_active = false;
	uint8_t enc_frame_data[3][2];

	uint8_t get_state(uint8_t _gs_states, uint8_t pos){
	  return (_gs_states & (1<<pos)) > 0;
	}


	// return values
	// 0=invalid, 1=plus 2=minus
	uint8_t process_enc_frames(){
		if (!unprocessed_frames){
			return 0;
		}
		uint8_t enc_dir = 0;
		// plus    = 10,00,01
		// minus   = 01,00,10
		for (uint8_t i=0; i<3; i++){
			uint8_t frm_plus  = enc_frame_data[i][0];
			uint8_t frm_minus = enc_frame_data[i][1];
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
		if ( enc_is_inverse ){
			if (enc_dir==1) return 2;
			if (enc_dir==2) return 1;
		}
		unprocessed_frames = false;
		return enc_dir;
	}


  public:
  PCF8574Encoder(uint8_t port_btn, uint8_t port_plus, uint8_t port_minus, bool init_inverse = false) {
		enc_port_btn   = port_btn;
		enc_port_plus  = port_plus;
		enc_port_minus = port_minus;
		enc_is_inverse = init_inverse;
		callback_up = [](){ return; };
		callback_down = [](){ return; };
		callback_button = [](){ return; };
  }

	void up(PCF8574EncoderCallback cb){ 		callback_up = cb;     }
	void down(PCF8574EncoderCallback cb){ 	callback_down = cb;   }
	void button(PCF8574EncoderCallback cb){ callback_button = cb; }


	void process(uint8_t data) {
    uint8_t enc_btn = get_state(data, enc_port_btn);
    uint8_t enc_plus = get_state(data, enc_port_plus);
		uint8_t enc_minus = get_state(data, enc_port_minus);

		// process button
		if (enc_btn<1){
			callback_button();
			return;
		}



		// process encoder
		if ((enc_plus<1 || enc_minus<1) && !enc_frame_active){
      enc_frame_active = true;
    }


		if (enc_frame_active){
			enc_frame_data[enc_frame_num][0] = enc_plus;
			enc_frame_data[enc_frame_num][1] = enc_minus;
			enc_frame_num++;
			if (enc_frame_num>2){
				unprocessed_frames = true;
				enc_frame_active = false;
      	enc_frame_num = 0;
				uint8_t enc_dir = process_enc_frames();
				if (enc_dir>0){
					if (enc_dir==1) {
						callback_up();
					}
					if (enc_dir==2) {
						callback_down();
					}
				}
			}
		}

	}



	bool ready(){
		if (enc_frame_num==0 && unprocessed_frames){
			return true;
		}
		return false;
	}

};
