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
  if ( debug_output_enabled ) {
   Serial.println( message );
  }
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


  if ( command == "ss" ) {
    String s_str = "";
    s_str = get_str_token( serdata, ' ', 1 );
    float s;
    s = s_str.toFloat();
    step_size = s;
    Serial.print("step_size=");
    Serial.println((float)step_size);
    return;
  }


  // turn on printing debug messages
  if ( command == "show-debug" ) {
    debug_output_enabled = true;
    debug("debug output enabled");
    return;
  }

  // turn off printing debug messages
  if ( command == "hide-debug" ) {
    debug_output_enabled = false;
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
