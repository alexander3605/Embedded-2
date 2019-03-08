#include "mbed.h"
#include <stdint.h>
#include <cstdio>
#include <inttypes.h>
#include <string>
#include <sstream>
#include "SHA256.h"


//using namespace std;

//////////////////////////////////
// GLOBAL VARIABLES
int8_t orState = 0;    //Rotot offset at motor state 0
int8_t intState = 0;
int8_t intStateOld = 0;
RawSerial pc(SERIAL_TX, SERIAL_RX);
Thread threadReceive, threadDecode;
Mutex NewKey_Mutex;
uint64_t KEY;
Timer t;
//////////////////////////////////
    
#include "Coms.h"
#include "Motor.h"

void photoISR(){
    intState = readRotorState();
    motorOut((intState-orState-lead+6)%6); //+6 to make sure the remainder is positive
    
}


//Main
int main() {
    
    I1.rise(&photoISR);
    I2.rise(&photoISR);
    I3.rise(&photoISR);
    I1.fall(&photoISR);
    I2.fall(&photoISR);
    I3.fall(&photoISR);
    
    
    //Initialise the serial port
    
    pc.printf("Hello\n\r");
    
    //Run the motor synchronisation
    orState = motorHome();
    //pc.printf("Rotor origin: %x\n\r",orState);
    //orState is subtracted from future rotor state inputs to align rotor and motor states
    threadReceive.start(callback(Receiver));
    threadDecode.start(callback(decodeSerialInput));
    
    //////////////////////
    // HASHING FUNCTION //
    //////////////////////
    
    SHA256 mySHA256 = SHA256();
    uint8_t sequence[] = {0x45,0x6D,0x62,0x65,0x64,0x64,0x65,0x64,
    0x20,0x53,0x79,0x73,0x74,0x65,0x6D,0x73,
    0x20,0x61,0x72,0x65,0x20,0x66,0x75,0x6E,
    0x20,0x61,0x6E,0x64,0x20,0x64,0x6F,0x20,
    0x61,0x77,0x65,0x73,0x6F,0x6D,0x65,0x20,
    0x74,0x68,0x69,0x6E,0x67,0x73,0x21,0x20,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    uint64_t* key = (uint64_t*)((int)sequence + 48);
    uint64_t* nonce = (uint64_t*)((int)sequence + 56);
    
    uint8_t hash[32];
    
    int counter = 0;
    t.start();
    while (1) {
        mySHA256.computeHash(hash,sequence,sizeof(sequence));
        counter++;
        if(t.read() >= 1){  //if a second has passed
            t.stop();
            t.reset();
            t.start();
            // pc.printf("%d", counter);    //hashes per second
            // pc.printf(" hashes per second \n\r");
            counter = 0;
            }
        if((hash[0] == 0) && (hash[1] == 0)){
            // pc.printf("SUCCESS \n\r");
            
            for(int i = 0; i <8; i++){
                // pc.printf("%#02x", nonce[i]);
                // pc.printf(", ");
            }
            // pc.printf("\n");
        }
        *nonce+=1;
    }
}


