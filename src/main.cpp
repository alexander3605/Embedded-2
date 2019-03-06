#include "mbed.h"
#include <stdint.h>
#include <cstdio>
#include <inttypes.h>
#include <string>
#include <sstream>

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
    
    //Poll the rotor state and set the motor outputs accordingly to spin the motor
    while (1) {


        
        //pc.printf("Llave: %d\n\r",KEY);


    }
}


