#include <MilliTimer.h>
#include <EEPROM.h>
#include <RFM12B.h>
#include <IRremote.h>

/*** NODE DEFINES ***/
#define GATEWAYID 1
#define LEDID 2
#define BOILERID 3
#define ALARMID 4
#define NETWORKID 99
#define ACK_TIME 50
//#define SER_DEBUG 1

/*** LED DEFINES AND VARIABLES ***/
// R:6 G:9 B:5
#define LED_R 6
#define LED_G 9
#define LED_B 5
#define EEPROM_BASE 0x100 // store ramps starting at this offset
#define RAMP_LIMIT 100    // room for ramps 0..99, stored in EEPROM

// Infrared sending module
IRsend irsend;

typedef struct {
  byte colors[3];
  byte steps;
  byte chain;
} Ramp;

typedef struct {
    int R;   
    int G;
    int B;
} Payload;

typedef struct {
  byte id;
  byte red;
  byte green;
  byte blue;
  byte duration;
  byte next_id;
} payload;

payload led_data;

Payload tx;

long now[3];
long delta[3];
word duration;
byte nextRamp;
MilliTimer timer;

static Ramp stdRamps[] = {
  {   0,   0,   0, 0, 0 }, // 0: instant off
  { 255,  85,  30, 0, 0 }, // 1: instant warm white
  { 255, 150,  75, 0, 0 }, // 2: instant cold white
  {   0,   0,   0, 5, 0 }, // 3: 5s off
  { 255,  85,  30, 5, 0 }, // 4: 5s warm white
  { 255, 150,  75, 5, 0 }, // 5: 5s cold white
  { 255,   0,   0, 5, 7 }, // 6: 5s red -> green -> blue
  {   0, 255,   0, 5, 8 }, // 7: 5s green -> blue -> red
  {   0,   0, 255, 5, 6 }, // 8: 5s blue -> red -> green
  {  20,  10,  10, 1, 0 }, // 9: instant faint red'ish yellow
};
/** LED END **/

long previous_millis = 0;
#define SER_ARR_LEN 30
char inData[SER_ARR_LEN];
int index = 0;

RFM12B radio;

void setup() {
  bitSet(TCCR1B, WGM12); // fix timer 1 so it also runs in fast PWM mode, to match timer 0
  
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  
  setupLED();
  
  radio.Initialize(GATEWAYID, RF12_868MHZ, NETWORKID); 
  
  Serial.begin(115200);
  Serial.println("Starting...");

  loadRamp(9);
}

void loop() {
  if (timer.poll(10)) {
    if (duration > 0) {
      --duration;
      for (byte i = 0; i < 3; ++i)
        now[i] += delta[i];
      setLeds();
    } else if (nextRamp != 0)
      loadRamp(nextRamp);
  }
  
  if (radio.ReceiveComplete()) {
    if (radio.CRCPass()) {
      
      Serial.print(radio.GetSender());
      Serial.print("|");
      for (byte i = 0; i < *radio.DataLen; i++) {//can also use radio.GetDataLen() if you don't like pointers
        Serial.print((char)radio.Data[i]);
      }
      
      if (radio.ACKRequested()) {
        radio.SendACK();
      }
    } else {
      Serial.print(radio.GetSender());
      Serial.print("|BAD-CRC");
    }
    Serial.println();
  }
  
  // Serial incoming data from PC
  while (Serial.available() > 0) {
    char inChar = Serial.read();
    #if defined(SER_DEBUG)     
      Serial.println(inChar);
    #endif
    
    if (index > SER_ARR_LEN) {
      memset(inData, 0, SER_ARR_LEN);
    } else if (inChar == '\n' || inChar == '\r') {
      // Deal with the incoming data
      actionCommand();
      index = 0;
      memset(inData, 0, SER_ARR_LEN); // Fill the buffer with 0's
    } else {
      inData[index] = inChar;
      index++;
      inData[index] = '\0';
    }
  }
  
  
  // Every 10 seconds send its current state
  unsigned long current_millis = millis();
  if (current_millis - previous_millis > 60000) {
    previous_millis = current_millis;
    Serial.println("SELF|ALIVE");
  }
}
