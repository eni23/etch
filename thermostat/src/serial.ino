/*******
 * Serial functions
 * Provide a simple serial terminal for getting and setting wifi and
 * trigger configuration.
 ****/


char serial_rcv_data[SERIAL_INPUT_BUFFER_SIZE];
uint8_t serial_rcv_data_pos = 0;


void serial_char_event() {
  char current_char = Serial.read();
  // backspace: delete last char in buffer
  if ( current_char == 0x7F && serial_rcv_data_pos > 0 ) {
    serial_rcv_data[serial_rcv_data_pos-1];
    serial_rcv_data_pos--;
    Serial.print("\b");
    Serial.print(" ");
    Serial.print("\b");
    return;
  }
  // carriage return recived = new line
  if (current_char == 13) {
    Serial.println( current_char );
    serial_rcv_data[serial_rcv_data_pos] = '\0';
    serial_process_input();
    return;
  }
  // put every other char than cr into buffer
  Serial.print( current_char );
  serial_rcv_data[serial_rcv_data_pos] = current_char;
  serial_rcv_data_pos++;
}


/**
 * serial debug message
 **/
void debug( const char* message ) {
  if ( eeprom_config.debug_output_enabled ) {
   Serial.println( message );
  }
}

int intArg(String serdata, int index){
  String s_str = "";
  s_str = get_str_token( serdata, ' ', index+1 );
  int s;
  s = s_str.toInt();
  return s;
}

float floatArg(String serdata, int index){
  String s_str = "";
  s_str = get_str_token( serdata, ' ', index+1 );
  float s;
  s = s_str.toFloat();
  return s;
}

/**
 * this function parses every line recived by serial port and exectues an action
 * if command found.
 * to keep it simple, everything is done by this single function and input is
 * processed as strings, not char-arrays..
 **/
void serial_process_input() {

  // split incoming line by whitespace and parse first chuck as possible command
  String serdata( serial_rcv_data );
  String command = "";
  command = get_str_token( serdata, ' ', 0 );

  // empty buffer and zero counter
  memset(serial_rcv_data, 0, sizeof( serial_rcv_data ) );
  serial_rcv_data_pos = 0;

  // simple pong for recognition
  if (  command ==  "ping" ) {
    Serial.println( "pong" );
    return;
  }


  if ( command == "step-size" ) {
    String s_str = "";
    s_str = get_str_token( serdata, ' ', 1 );
    int s;
    s = s_str.toInt();
    if (s>0){
      eeprom_config.step_size = s;
      Serial.print("step_size=");
      Serial.println((int)eeprom_config.step_size);
      eeprom_save_config();
    }
    else {
      Serial.print("step_size=");
      Serial.println((int)eeprom_config.step_size);
    }
    return;
  }

  if (command == "set-time") {
    String s_str = "";
    s_str = get_str_token( serdata, ' ', 1 );
    int s;
    s = s_str.toInt();
    if (s>0){
      current_time = s;
      save_time();
      update_display();
    }
    return;
  }

  if (command == "run") {
    if (is_running){
      debug("allready running");
    }
    else {
      if (current_time<=0){
        current_time = eeprom_config.time;
      }
      run_timer();
    }
    return;
  }

  if (command == "stop") {
    if (!is_running){
      debug("timer is not running");
    }
    else {
      stop_timer();
    }
    return;
  }

  if (command == "temp") {
    temp.read();
    return;
  }

  if (command == "temp-delay") {
    int delc = intArg(serdata,0);
    if (delc<20){
      debug("delay needs to be at least 20ms");
    }
    temp_read_delay = delc;
    timer_temp.deleteTimer(temp_timer);
    temp_timer = timer_temp.setInterval(temp_read_delay, start_temp_read);
    debug("new timer set up");
    return;
  }

  if (command == "temp-precision") {
    int precision = intArg(serdata,0);
    if (precision<9 || precision > 12){
      debug("precision needs to be between 9 and 12 (bits)");
      return;
    }
    temp.set_resolution(precision);
    return;
  }

  // reboot device
  if ( command == "disp-raw" ) {
    int width = intArg(serdata,0);
    int height = intArg(serdata,1);
    int nc = intArg(serdata,2);
    uint8_t pxbuffer[nc+1];
    int cc = 0;
    Serial.print(nc);
    Serial.println(" go");
    delay(100);
    while (cc<nc){
      pxbuffer[cc] = Serial.read();
      cc++;
    }
    lock_display=true;
    display.clear();
    display.drawBitmap(0, 17, pxbuffer, width, height, WHITE );
    display.update();
    debug("ok");
    return;
  }

  // turn on printing debug messages
  if ( command == "show-debug" ) {
    eeprom_config.debug_output_enabled = true;
    eeprom_save_config();
    debug("debug output enabled");
    return;
  }

  // turn off printing debug messages
  if ( command == "hide-debug" ) {
    //eeprom_config.debug_output_enabled = false;
    //eeprom_save_config();
    return;
  }

  // reboot device
  if ( command == "reboot" ) {
    return;
  }

  if ( command == "_gpio" ) {
    String port_str = "";
    port_str = get_str_token( serdata, ' ', 1 );

    String state_str = "";
    state_str = get_str_token( serdata, ' ', 2 );

    int port = port_str.toInt();
    int state = state_str.toInt();
    if (state > 0){
      digitalWrite(port, HIGH);
      debug("turn on");
    }
    else {
      digitalWrite(port, HIGH);
      debug("turn off");
    }


  }

  // print detailed systeminfos
  if ( command == "info" ) {
    Serial.print( "current_time = " );
    Serial.println( current_time );
    Serial.print( "eeprom.time = " );
    Serial.println( eeprom_config.time );
    Serial.print( "eeprom.step_size = " );
    Serial.println( eeprom_config.step_size );
    if (eeprom_config.debug_output_enabled){
      Serial.println( "eeprom.debug_output_enabled = true" );
    } else {
      Serial.println( "eeprom.debug_output_enabled = false" );
    }
    Serial.print( "esp8266.free_heap = " );
    Serial.println( ESP.getFreeHeap() );
    Serial.print( "esp8266.chip_id = " );
    Serial.println( ESP.getChipId() );
    Serial.print( "esp8266.flash_size = " );
    Serial.println( ESP.getFlashChipSize() );
    Serial.print( "esp8266.flash_speed = " );
    Serial.println( ESP.getFlashChipSpeed() );
    Serial.print( "esp8266.cycle_count = " );
    Serial.println( ESP.getCycleCount() );
    return;
  }

  // if nothing matches, command not found
  debug("unknown command");
}
