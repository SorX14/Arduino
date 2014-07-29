void actionCommand() {
  // Called when there is a command in the serial buffer
  
  /** Valid command list
  
  L:25525525500    Send command to self
  A:1/0            Turn alarm on and off
  
  **/
  
  #if defined(SER_DEBUG)     
    debug();
  #endif
  
  if (inData[0] == 'L' && strlen(inData) == 13) { // LED
    // Convert the incoming string to a payload
    conformLED();

    byte a[5] = {led_data.red, led_data.green, led_data.blue, led_data.duration, led_data.next_id};
    useRamp((const Ramp*) a);
  } else if (inData[0] == 'A' && strlen(inData) == 2) { // ALARM
    bool gotAck = false;
    for (int i = 0; i <= 5; i++) {
      radio.Send(ALARMID, inData, 2, true);    
      if (waitForAck(ALARMID)) {
        gotAck = true;
        break;
      }       
    }
  } else if (inData[0] == 'R' && strlen(inData) == 2) {
      int remote_code = inData[1] - '0';
      
      #if defined(SER_DEBUG)     
         Serial.print("REMOTE CODE: ");
         Serial.println(remote_code);
      #endif
      
      switch (remote_code) {
        case 0: // ON
          irsend.sendNEC(0xC1AA09F6, 32);           
        break;
        
        case 1: // OFF
          irsend.sendNEC(0xC1AA8976, 32);           
        break;
        
        case 2: // UP
          irsend.sendNEC(0xC1AA0DF2, 32);
        break;
        
        case 3: // DOWN
          irsend.sendNEC(0xC1AA4DB2, 32);
        break;
        
        case 4: // LEFT
          irsend.sendNEC(0xC1AACD32, 32);
        break;
        
        case 5: // RIGHT
          irsend.sendNEC(0xC1AA8D72, 32);
        break;
        
        case 6: // 2D/3D
          irsend.sendNEC(0xC1AAC43B, 32);
        break;

        case 7: // MENU
          irsend.sendNEC(0xC1AA59A6, 32);
        break;        

        case 8: // ENTER
          irsend.sendNEC(0xC1AAA15E, 32);
        break;
        
        case 9: // ESC
          irsend.sendNEC(0xC1AA21DE, 32);
        break;
      }        
  }
}

void debug() {
  Serial.print("Action command: ");
  Serial.print(inData);
  Serial.print(" (");
  Serial.print(strlen(inData));
  Serial.println(") BYTES: ");
  
  for (int b = 0; b <= strlen(inData); b++) {
    Serial.print("|");
    Serial.print((byte)inData[b]);
    Serial.print("|");
  }
  Serial.println("");
}

void conformLED() {
  char red[4];
    char green[4];
    char blue[4];
    char dur[2];
    char next_id[2];
    
    red[0] = inData[2];
    red[1] = inData[3];
    red[2] = inData[4];
    red[3] = '\0';
    
    green[0] = inData[5];
    green[1] = inData[6];
    green[2] = inData[7];
    green[3] = '\0';
    
    blue[0] = inData[8];
    blue[1] = inData[9];
    blue[2] = inData[10];
    blue[3] = '\0';
    
    dur[0] = inData[11];
    dur[1] = '\0';
    
    next_id[0] = inData[12];
    next_id[1] = '\0';

    led_data.id = 0;
    led_data.red = (byte) constrain(atoi(red), 0, 255);
    led_data.blue = (byte) constrain(atoi(blue), 0, 255);
    led_data.green = (byte) constrain(atoi(green), 0, 255);
    led_data.duration = (byte) atoi(dur);
    led_data.next_id = (byte) atoi(next_id);
}

static bool waitForAck(int node_id) {
  long now = millis();
  
  while (millis() - now <= ACK_TIME) {
    if (radio.ACKReceived(node_id)) {
      return true;
    }
  }
  
  return false;
}

