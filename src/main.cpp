// Import std libraries
#include <stdint.h>
#include <cstdio>
#include <inttypes.h>
#include <stdlib.h>
#include <string>
#include <algorithm>
#include <sstream>
#include <unistd.h>
#include "mbed.h"
#include "SHA256.h"

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
char* msg_swag;
string temp_str;
int stepCount;
int direction;
int velocity;
Thread motorCtrlT(osPriorityNormal, 1024);
int oldPosition;
int revsCount = 0;
//////////////////////////////////


//////////////////////////////////
// Func Declarations
void photoISR();
void putMessage(int n, char* s, uint64_t keyP);
//////////////////////////////////

// Import custom libraries
#include "Motor.h"
#include "Coms.h"

void photoISR(){
    intState = readRotorState();
    motorOut((intState-orState+lead+6)%6); //+6 to make sure the remainder is positive
    if(intState == orState) revsCount++;
    // Get direction
    int diff = intState - intStateOld;
    if (diff > 0){
        if (diff == 5) direction  = -1;
        else           direction  = 1;
    }
    else if (diff < 0){
        if (diff == -5) direction  = 1;
        else           direction  = -1;
    }    
    stepCount += direction;
    intStateOld = intState;
}


    
//Main
int main() {
    
    // Initialize PWM_OUT params
    pwm_out.period(0.002f);
    pwm_out = 1.0f;
    //Initialise the serial port
    pc.printf("Hello\n");
    
    threadReceive.start(callback(Receiver));
    threadDecode.start(callback(decodeSerialInput));
    motorCtrlT.start(callback(motorCtrlFn));
    
        
    //Run the motor synchronisation
    orState = motorHome();
    
    /// ATTACH INTERRUPTS
    I1.rise(&photoISR);
    I2.rise(&photoISR);
    I3.rise(&photoISR);
    I1.fall(&photoISR);
    I2.fall(&photoISR);
    I3.fall(&photoISR);
    CHA.rise(&encISR0);
    CHB.rise(&encISR1);
    CHA.fall(&encISR2);
    CHB.fall(&encISR3);
    ///////
    //photoISR(); //start rotation

    
    
//    pc.printf("Rotor origin: %x\n\r",orState);
    //orState is subtracted from future rotor state inputs to align rotor and motor states

    
    // HASHING FUNCTION //
    
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
//            msg_swag = "TrYiNg!\n\r";
//            putMessage(msg_swag);

            msg_swag = "hashes per second\n\r";
            //putMessage(counter, msg_swag,KEY);
            counter = 0;
            }
        if((hash[0] == 0) && (hash[1] == 0)){
            msg_swag = "SUCCESS \n\r";
            //putMessage(0, msg_swag,KEY);
//            pc.printf("  Nonce: %d\n", *nonce);
//            pc.printf("  ");
            for(int i = 0; i <8; i++){
//                pc.printf("%#02x", nonce[i]);
//                pc.printf(", ");
                continue;
            }
            msg_swag = "\n";
            //putMessage(0, msg_swag, KEY);
        }
        *nonce+=1;
    }
}

