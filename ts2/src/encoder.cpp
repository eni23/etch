#include <Arduino.h>



#define ENCODER_DIR_PLUS		1
#define ENCODER_DIR_MINUS		2


class Encoder {

	bool unprocessed_frames = false;
	bool enc_is_inverse = false;
	uint8_t enc_frame_num = 0;
	bool enc_frame_active = false;
	uint8_t enc_frame_data[3][2];

  public:
  Encoder(bool init_inverse = false) {
		enc_is_inverse = init_inverse;
  }

  void add_frame(uint8_t frmd_plus, uint8_t frmd_minus){
		enc_frame_data[enc_frame_num][0] = frmd_plus;
		enc_frame_data[enc_frame_num][1] = frmd_minus;
		enc_frame_num++;
		if (enc_frame_num>2){
			unprocessed_frames = true;
      enc_frame_num=0;
		}
	}

	// return values
	// 0=invalid, 1=plus 2=minus
	uint8_t process(){
		if (!unprocessed_frames){
			return 0;
		}
		uint8_t enc_dir = 0;
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

	bool ready(){
		if (enc_frame_num==0 && unprocessed_frames){
			return true;
		}
		return false;
	}

};
