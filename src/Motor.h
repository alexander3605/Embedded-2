#ifndef Motor_h
#define Motor_h

//Mapping from sequential drive states to motor phase outputs
/*
State   L1  L2  L3
0       H   -   L
1       -   H   L
2       L   H   -
3       L   -   H
4       -   L   H
5       H   L   -
6       -   -   -
7       -   -   -
*/

//Photointerrupter input pins
#define I1pin D3
#define I2pin D6
#define I3pin D5

//Incremental encoder input pins
#define CHApin   D12
#define CHBpin   D11

//Motor Drive output pins   //Mask in output byte
#define L1Lpin D1           //0x01
#define L1Hpin A3           //0x02
#define L2Lpin D0           //0x04
#define L2Hpin A6          //0x08
#define L3Lpin D10           //0x10
#define L3Hpin D2          //0x20

#define PWMpin D9
PwmOut pwm_out(PWMpin);

//Motor current sense
#define MCSPpin   A1
#define MCSNpin   A0

//Drive state to output table
const int8_t driveTable[] = {0x12,0x18,0x09,0x21,0x24,0x06,0x00,0x00};
int Tr = 0;
int Ts = 0; 
//Mapping from interrupter inputs to sequential rotor states. 0x00 and 0x07 are not valid
const int8_t stateMap[] = {0x07,0x05,0x03,0x04,0x01,0x00,0x02,0x07};  
//const int8_t stateMap[] = {0x07,0x01,0x03,0x02,0x05,0x00,0x04,0x07}; //Alternative if phase order of input or drive is reversed

//Phase lead to make motor spin
int8_t lead = 2;  //2 for forwards, -2 for backwards

//Status LED
DigitalOut led1(LED1);

//Photointerrupter inputs
InterruptIn I1(I1pin);
InterruptIn I2(I2pin);
InterruptIn I3(I3pin);

//Motor Drive outputs
DigitalOut L1L(L1Lpin);
DigitalOut L1H(L1Hpin);
DigitalOut L2L(L2Lpin);
DigitalOut L2H(L2Hpin);
DigitalOut L3L(L3Lpin);
DigitalOut L3H(L3Hpin);


//Encoder inputs
InterruptIn CHA(CHApin);
InterruptIn CHB(CHBpin);
int encCount = 0;
int encState = 0;
int badEdges = 0;
//Encoder interrupts functions
void encISR0() {
    if (encState == 3) encCount++;
    else badEdges++;
    encState = 0;
    }
void encISR1() {
    if (encState == 0) encCount++;
    else badEdges++;
    encState = 1;
    }
void encISR2() {
    if (encState == 1) encCount++;
    else badEdges++;
    encState = 2;
    }
void encISR3() {
    if (encState == 2) encCount++;
    else badEdges++;
    encState = 3;
    }

//Set a given drive state
void motorOut(int8_t driveState){
    
    //Lookup the output byte from the drive state.
    int8_t driveOut = driveTable[driveState & 0x07];
      
    //Turn off first
    if (~driveOut & 0x01) L1L = 0;
    if (~driveOut & 0x02) L1H = 1;
    if (~driveOut & 0x04) L2L = 0;
    if (~driveOut & 0x08) L2H = 1;
    if (~driveOut & 0x10) L3L = 0;
    if (~driveOut & 0x20) L3H = 1;
    
    //Then turn on
    if (driveOut & 0x01) L1L = 1;
    if (driveOut & 0x02) L1H = 0;
    if (driveOut & 0x04) L2L = 1;
    if (driveOut & 0x08) L2H = 0;
    if (driveOut & 0x10) L3L = 1;
    if (driveOut & 0x20) L3H = 0;
    }
    
    //Convert photointerrupter inputs to a rotor state
inline int8_t readRotorState(){
    return stateMap[I1 + 2*I2 + 4*I3];
    }

//Basic synchronisation routine    
int8_t motorHome() {
    //Put the motor in drive state 0 and wait for it to stabilise
    motorOut(0);
    wait(2.0);
    
    //Get the rotor state
    return readRotorState();
}


// Motor control stuff

void motorCtrlTick(){
    motorCtrlT.signal_set(0x1);
}

/// print number of revolutions, called every sec
int old_revsCount = 0;
void printRevs(){
    putMessage(revsCount - old_revsCount, " revs/sec\n\r", 0);
    putMessage(Ts, " Ts\n\r", 0);
    old_revsCount = revsCount;
}

void motorCtrlFn(){
    int oldPos = 0;
    Ticker motorCtrlTicker; 
    motorCtrlTicker.attach_us(&motorCtrlTick,50000);
    Ticker statusPrintTicker;
    statusPrintTicker.attach_us(&printRevs,1000000);
    int motorCount = 0;
    while(1){
        motorCtrlT.signal_wait(0x1);
        core_util_critical_section_enter();
        int position = revsCount;
        core_util_critical_section_exit();
        velocity = 20* (position - oldPos);
        oldPos=position;
//        motorCount++;
//        if(motorCount == 10){
//             putMessage((int)(velocity)," revs/sec\n\r",1);
//             motorCount =0;
//        }
    }
}

// BUG TO FIX --> pwm_out goes back to max value after this function
void set_velocity(int target_speed){
    if(!target_speed) {
        pwm_out.write(1);
        return;}
    if(target_speed > 0) lead = 2;
    else lead = -2;
     
    Ticker statusPrintTicker;
    photoISR();
    int Kps = 5; //Experimentally-defined constant
    int Kis = 2; //Experimentally-defined constant
    float es_integral = 0;
    float es;
    int sign;
    float timeStep = 1; //used for integral scaling
    while (velocity != target_speed){
        sign = 1;
        // TODO: add wait for timeStep
        // -- insert code --
        es = target_speed - abs(velocity);
        if(es < 0) sign = -1;
        es_integral += es * timeStep;
        Ts = (Kps * es  +  Kis * es_integral) * sign;
         
        if(Ts > 0){
            lead = 2;
            pwm_out.write( (float)(std::min(Ts, 100)/100) );
        } else {
            lead = -2;
            pwm_out.write( (float)(std::min(abs(Ts), 100)/100) );
        }
    }
}


void Rotate(float revs){
    // check rotation direction
    if (revs < 0) lead = -2;
    else lead = 2;
    revs = abs(revs);
    photoISR(); //start
    
    core_util_critical_section_enter();
    int targetRevs = revs + revsCount;
    core_util_critical_section_exit();
    int vel =0;
    int Kpr = 16.6;
    int Kdr = 24;
    int currRevs = 0;
    bool targetMet = false;
    
    while(!targetMet){
    core_util_critical_section_enter();
    vel = velocity;
    currRevs = revsCount;
    core_util_critical_section_exit();
    
    Tr = Kpr * (targetRevs - currRevs) - Kdr * vel; 
    
        pwm_out.write( (float)(std::min(Tr, 100)/100) );
    
        targetMet = abs(targetRevs) <= abs(currRevs);
        }   
        putMessage(currRevs," --> currRevs\n\r",0);
        
    }


#endif


