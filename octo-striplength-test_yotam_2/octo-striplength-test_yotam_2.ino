#define USE_OCTOWS2811

#include<OctoWS2811.h>
#include<FastLED.h>

//match num_ribs and length of ledsInRibs AND ribsInStrips

#define LONGEST_STRIP 500
#define NUM_RIBS 40
#define NUM_STRIPS 8

CRGB leds[LONGEST_STRIP * NUM_STRIPS];

// PANEL 3
//int ledsInRibs[] = {2, 8, 13, 18, 22, 27, 31, 35, 40, 45, 50, 54, 59, 63, 68, 72, 74, 75, 75, 75, 75, 74, 75, 75, 74, 75, 73, 69, 64, 60, 55, 51, 46, 41, 37, 32, 28, 23, 18, 14, 9, 4 };
int ledsInRibs[NUM_RIBS]={};

int ribsInStrips[] = {7,6,4,4,4,5,10,0};
int currentRib = 0;


// Pin layouts on the teensy 3:
// OctoWS2811: 2,14,7,8,6,20,21,5

void setup() {
  LEDS.addLeds<OCTOWS2811>(leds, LONGEST_STRIP);
  LEDS.setBrightness(64);
  //1 into each of the rows
 for (int i = 0; i < NUM_RIBS; i++){
   ledsInRibs[i] = 0; 
 }
//  LEDS.show();
}

void colorStrip(int ribNum, int ribLength){
  
}

int ribToStrip(int rib){
  int totalRibs = 0;
  for (int i = 0; i < NUM_STRIPS; i++){
    totalRibs += ribsInStrips[i];
    if (rib < totalRibs){
      return i;
    }
  }
}

void loop() { 
  //zero them out
  for(int i = 0; i < NUM_STRIPS; i++) {
    for (int j = 0; j < LONGEST_STRIP; j++){
      leds[i * LONGEST_STRIP + j] = CRGB(64, 64, 64);
    }
  }
  
  //iterate over the ledsInRibs
  int totalLEDs = 0; 
  int stripNumber = 0;
  for (int rib = 0; rib < NUM_RIBS; rib++){
    int tmpStripNumber = ribToStrip(rib);
    if (tmpStripNumber != stripNumber){
      stripNumber = tmpStripNumber;
      totalLEDs = 0; 
    }
    CRGB color = CRGB::Red;
    if (rib % 3 == 1){
      color = CRGB::Blue;
    } else if (rib % 3 == 2){
      color = CRGB::Green;
    }
    int ledsInRib = ledsInRibs[rib];
    for (int l = 0; l < ledsInRib; l++){
      leds[ribToStrip(rib) * LONGEST_STRIP + l + totalLEDs] = color; 
    }
    totalLEDs += ledsInRib;
  }

  LEDS.show();
  //serial comm
  char recvBytes[10];
  int recvBytesCounter = 0;
  while (Serial.available() > 0) {
    // read the incoming byte:
    byte incomingByte = Serial.read();
    if (incomingByte == 13){
      recvBytes[recvBytesCounter] = '\0';
      int numberLeds = atoi(recvBytes);
      recvBytesCounter = 0;
      ledsInRibs[currentRib] = numberLeds;
    } else if (incomingByte == 110){ // n = next row
      currentRib++;
    } else if (incomingByte == 112) { // p = print all values
      Serial.flush();
      Serial.print("int ledsInRibs[] = ");
      Serial.print("{");
       for(int i = 0; i < NUM_RIBS; i++) {
        Serial.print(ledsInRibs[i]);
        Serial.print(", ");
       }
       Serial.println("}");
       //longest strip calculation
       int longestStrip = 0;
       int ribCount = 0; 
       for (int i = 0; i < NUM_STRIPS; i++){
         int ribsInStrip = ribsInStrips[i];
         int stripTotal = 0;
         for (int r = ribCount; r < ribsInStrip + ribCount; r++){
           stripTotal +=  ledsInRibs[r];
         }
         if (stripTotal > longestStrip){
           longestStrip = stripTotal;
         }
         ribCount += ribsInStrip;
       }
//       longestStrip++;
       Serial.print("#define NUM_LEDS_PER_STRIP ");
       Serial.println(longestStrip);
       //cluster LED ribs
       int universeLength[24];
       int universeOffsets[24];
       int ribsInUniverse[24];
       int total = 0; 
       int universes = 0;
       int currentStrip = 0; 
       int lastUniverseRib = 0;
       for (int i = 0; i < NUM_RIBS; i++){
         int ledsInRib = ledsInRibs[i];
         //if it's over 170 or if it's a strip edge
         if (total + ledsInRib > 170 || currentStrip != ribToStrip(i)){
           currentStrip = ribToStrip(i);
           universeLength[universes] = total;
           ribsInUniverse[universes] = i - lastUniverseRib;
           lastUniverseRib = i;
           total = 0;
           universes++;
         }
        total += ledsInRib;
       }
       ribsInUniverse[universes] = NUM_RIBS - lastUniverseRib;
       universeLength[universes] = total;
       
       //calculate the universe offsets
       int ribIndex = 0;
       currentStrip = 0;
       int lastStripUniverse = 0;
       for (int i = 0; i < universes; i++){
          ribIndex += ribsInUniverse[i];
          if (currentStrip != ribToStrip(ribIndex)){ 
             currentStrip = ribToStrip(ribIndex);
             lastStripUniverse = i + 1;
             Serial.println(lastStripUniverse);
           }
           universeOffsets[i] = currentStrip * longestStrip;
           for (int u = lastStripUniverse; u < i + 1; u++){
             universeOffsets[i] +=  universeLength[u];
           }
       }
 
       //print the universe lengths
       Serial.print("int universeLength[] = {");
       for (int i = 0; i < universes + 1; i++){
         Serial.print(universeLength[i]);
         Serial.print(", ");
       }
       Serial.println("};");
       
       Serial.print("int ribsInUniverse[] = {");
       for (int i = 0; i < universes + 1; i++){
         Serial.print(ribsInUniverse[i]);
         Serial.print(", ");
       }
       Serial.println("};");
       
       //universe offsets
       Serial.print("int universeOffsets[] = { 0, ");
       for (int i = 0; i < universes; i++){
         Serial.print(universeOffsets[i]);
         Serial.print(", ");
       }
       Serial.println("};");

    } else {
      recvBytes[recvBytesCounter++] = incomingByte; 
    }
  }
//  LEDS.delay(10);
}
