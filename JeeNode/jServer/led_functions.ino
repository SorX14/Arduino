static void setupLED () {
  for (byte i = 0; i < sizeof stdRamps / sizeof *stdRamps; ++i)  // set up the default ramps
    saveRamp(i, stdRamps + i);
}

static void setLeds () {
  int r = (byte) (((word) (now[0] >> 22) + 1) >> 1);
  int g = (byte) (((word) (now[1] >> 22) + 1) >> 1);
  int b = (byte) (((word) (now[2] >> 22) + 1) >> 1);
  
  r -= 5; // Was 7
  
  r = constrain(r, 0, 255);
  g = constrain(g, 0, 255);
  b = constrain(b, 0, 255);
  
  
  // set to bits 30..23, but rounded by one extra bit (i.e. bit 22)
  analogWrite(LED_R, r);
  analogWrite(LED_G, g);
  analogWrite(LED_B, b);
}

static void useRamp (const void* ptr) {
  const Ramp* ramp = (const Ramp*) ptr;
  
  bool gotAck = false;
  for (int i = 0; i <= 5; i++) {
    #if defined(SER_DEBUG)     
      Serial.println("Sending to remote node...");
    #endif
    radio.Send(LEDID, ramp, 6, true);    
    if (waitForAck(LEDID)) {
      gotAck = true;
      #if defined(SER_DEBUG)
        Serial.print("Got ack");
      #endif
      break;
    }       
  }

    #if defined(SER_DEBUG)     
      Serial.print("0|SET");  
      Serial.print("R:");
      Serial.print(ramp->colors[0]);
      Serial.print("G:");
      Serial.print(ramp->colors[1]);
      Serial.print("B:");
      Serial.println(ramp->colors[2]);
    #endif
    
    nextRamp = ramp->chain;
    duration = ramp->steps * 100;
    for (byte i = 0; i < 3; ++i) {
      long target = (long) ramp->colors[i] << 23;
      if (duration > 0)
        delta[i] = (target - now[i]) / duration;
      else
        now[i] = target;
    }
    setLeds();
}

static void loadRamp (byte pos) {
  if (pos < RAMP_LIMIT) {
    word addr = EEPROM_BASE + pos * sizeof (Ramp);
    Ramp ramp;
    for (byte i = 0; i < sizeof (Ramp); ++i)
      ((byte*) &ramp)[i] = EEPROM.read(addr+i);
    useRamp(&ramp);
  }
}

static void saveRamp (byte pos, const void* data) {
  if (pos < RAMP_LIMIT) {
    word addr = EEPROM_BASE + pos * sizeof (Ramp);
    for (byte i = 0; i < sizeof (Ramp); ++i)
      EEPROM.write(addr+i, ((const byte*) data)[i]);
  }
}
