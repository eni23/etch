

/**
 * split a string and get chuck at desired position
 * example:
 * data="foo,bar,baz" separator="," index=2  => baz
 * data="foo/bar" separator="/" index=0  => foo
 **/
String get_str_token( String data, char separator, int index ) {
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length()-1;
  for( int i = 0; i <= maxIndex && found <= index; i++ ){
    if( data.charAt( i ) == separator || i == maxIndex ){
        found++;
        strIndex[0] = strIndex[1] + 1;
        strIndex[1] = ( i == maxIndex ) ? i + 1 : i;
    }
  }
  return found > index ? data.substring( strIndex[0], strIndex[1] ) : "";
}



/*******
 * EEPROM helpers
 * Simple load/save stuct-config into eeprom
 *******/

/**
 * load eeprom-struct if version check passes
 **/
void eeprom_load_config() {
  debug( "eeprom: load" );
  // If nothing is found we load default settings.
  if ( EEPROM.read( CONFIG_START + 0 ) == CONFIG_VERSION[0] &&
       EEPROM.read( CONFIG_START + 1 ) == CONFIG_VERSION[1] &&
       EEPROM.read( CONFIG_START + 2 ) == CONFIG_VERSION[2] ) {
    for ( unsigned int t = 0; t < sizeof( eeprom_config ); t++ ) {
      *( ( char* ) &eeprom_config + t ) = EEPROM.read( CONFIG_START + t );
    }
  }
}

/**
 * save eeprom-struct
 **/
void eeprom_save_config() {
  debug( "eeprom: save" );
  for ( unsigned int t = 0; t < sizeof( eeprom_config ); t++ ) {
    EEPROM.write( CONFIG_START + t, *( ( char* ) &eeprom_config + t ) );
  }
  EEPROM.commit();
}
