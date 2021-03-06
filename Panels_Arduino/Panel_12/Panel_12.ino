/*
This example will receive multiple universes via Artnet and control a strip of ws2811 leds via 
Paul Stoffregen's excellent OctoWS2811 library: https://www.pjrc.com/teensy/td_libs_OctoWS2811.html
This example may be copied under the terms of the MIT license, see the LICENSE file for details
*/

#define USE_OCTOWS2811

#include <Artnet.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SPI.h>

#include<OctoWS2811.h>
#include<FastLED.h>

boolean debug = 0; //activate serial printing, !! waits for a serial input to start loop

// BEGIN PER PANEL DATA:

int ledsInRibs[] = {5, 9, 13, 18, 22, 27, 31, 36, 41, 45, 50, 54, 59, 64, 68, 73, 74, 74, 74, 74, 74, 74, 74, 74, 73, 68, 63, 58, 54, 49, 45, 40, 36, 31, 26, 22, 16, 12, 8, 4};

#define NUM_LEDS_PER_STRIP 410

int universeLength[] = {161, 136, 113, 132, 147, 148, 148, 148, 147, 131, 161, 85, 155 };

int ribsInUniverse[] = {8, 3, 2, 2, 2, 2, 2, 2, 2, 2, 3, 2, 8};

int universeOffsets[] = { 0, 161, 297, 410, 542, 820, 968, 1230, 1378, 1640, 1771, 1932, 2050};

byte ip[] = {192, 168, 2, 12  };

int START_SUBNET = 5;
int START_UNIVERSE = 11;
int UNIVERSE_OFFSET = START_SUBNET * 16 + START_UNIVERSE;

//END PER PANEL DATA

#define NUM_STRIPS 8

CRGB leds[NUM_STRIPS * NUM_LEDS_PER_STRIP];

// Artnet settings
Artnet artnet;
const int startUniverse = 0; // CHANGE FOR YOUR SETUP most software this is 1, some software send out artnet first universe as zero.
const int numberOfChannels = NUM_STRIPS * NUM_LEDS_PER_STRIP * 3; // Total number of channels you want to receive (1 led = 3 channels)
byte channelBuffer[numberOfChannels]; // Combined universes into a single array
const int DMXmax = 510; // limit number of channels read from each universe (in case the artnet software always sends 512 value)

//int universeLength[17] = { 
//151, 133, 112, 130, 
//148, 167, 131, 130, 
//131, 130, 87,  169, 
//150, 131, 163, 152, 88};

// Change ip and mac address for your setup

char mac_string[20];

//everything on the network needs a unique MAC
#if defined(__MK20DX128__) || defined(__MK20DX256__)
// Teensy 3.x has MAC burned in
static byte mac[6];
void read(uint8_t word, uint8_t *mac, uint8_t offset) {
FTFL_FCCOB0 = 0x41; // Selects the READONCE command
FTFL_FCCOB1 = word; // read the given word of read once area

// launch command and wait until complete
FTFL_FSTAT = FTFL_FSTAT_CCIF;
while(!(FTFL_FSTAT & FTFL_FSTAT_CCIF));

*(mac+offset) = FTFL_FCCOB5; // collect only the top three bytes,
*(mac+offset+1) = FTFL_FCCOB6; // in the right orientation (big endian).
*(mac+offset+2) = FTFL_FCCOB7; // Skip FTFL_FCCOB4 as it's always 0.
}
void read_mac() {
read(0xe,mac,0);
read(0xf,mac,3);
}
#else
void read_mac() {}
byte mac[] = {
0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; // you can find this written on the board of some Arduino Ethernets or shields
#endif

void print_mac() {
size_t count = 0;
for(uint8_t i = 0; i < 6; ++i) {
if (i!=0) count += Serial.print(":");
count += Serial.print((*(mac+i) & 0xF0) >> 4, 16);
count += Serial.print(*(mac+i) & 0x0F, 16);
}
sprintf(mac_string, "%02X:%02X:%02X:%02X:%02X:%02X",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
Serial.println();
}



void setup()
{

read_mac();
if (debug){
Serial.begin(115200);
while(!Serial.available()){;} //if debug, wait until you send something to start the sketch
Serial.print("mac : ");
print_mac();
// for (int i ; i<3 ; i++){
// Serial.print(mac[i],HEX);
// Serial.print("-");
// }
// Serial.println(mac[3],HEX);
Serial.print("IP : ");
for (int i ; i<3 ; i++){
Serial.print(ip[i]);
Serial.print(".");
}
Serial.println(ip[3]);
Serial.print(NUM_STRIPS);
Serial.print(" strips of ");
Serial.print(NUM_LEDS_PER_STRIP);
Serial.println(" leds");
}

artnet.begin(mac, ip);
LEDS.addLeds<OCTOWS2811>(leds, NUM_LEDS_PER_STRIP).setCorrection( 0xDFFAF0 );;
  LEDS.setBrightness(170);
initTest();
// this will be called for each packet received
artnet.setArtDmxCallback(onDmxFrame);
}

long previousMillis = 0;        // will store last time LED was updated

// the follow variables is a long because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
long interval = 15;           // interval at which to blink (milliseconds)

void loop()
{
  // we call the read function inside the loop
    artnet.read();

  unsigned long currentMillis = millis();

  if(currentMillis - previousMillis > interval) {
   // save the last time you blinked the LED 
    previousMillis = currentMillis;   
     //draw the frame
    for (int i = 0; i < NUM_LEDS_PER_STRIP * NUM_STRIPS; i++)
    {
    leds[i] = CRGB(channelBuffer[(i) * 3], channelBuffer[(i * 3) + 1], channelBuffer[(i * 3) + 2]);
    } 
    LEDS.show();
  }
}

void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data)
{
 
   universe = universe - UNIVERSE_OFFSET; 
  if(debug){
  Serial.println(universe);
  }
  
 int bufferIndex;

// read universe and put into the right part of the display buffer
  int universeOffset = universeOffsets[universe] * 3;
  
  for (int i = 0 ; i < universeLength[universe] * 3 ; i++){
    int bufferIndex = i + universeOffset;
    channelBuffer[bufferIndex] = byte(data[i]);
  }
  
  
}




void initTest()
{
for (int i = 0 ; i < NUM_LEDS_PER_STRIP * NUM_STRIPS ; i++)
leds[i] = CRGB(64,0,0);
LEDS.show();

delay (250);
for (int i = 0 ; i < NUM_LEDS_PER_STRIP * NUM_STRIPS ; i++)
leds[i] = CRGB(0,64,0);
LEDS.show();

delay (250);
for (int i = 0 ; i < NUM_LEDS_PER_STRIP * NUM_STRIPS ; i++)
leds[i] = CRGB(0,0,64);
LEDS.show();

}
