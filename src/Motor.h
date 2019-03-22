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
float Tr = 0;
float Ts = 0; 
float glob_target_speed;

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
    float TempTr, TempTs, TempVel;
    Ticker motorCtrlTicker; 
    motorCtrlTicker.attach_us(&motorCtrlTick,50000);
    Ticker statusPrintTicker;
    //statusPrintTicker.attach_us(&printRevs,1000000); // --> UNCOMMENT TO PRINT ROTATION SPEED
    int motorCount = 0;
    int position;
    float T;
    while(1){
        motorCtrlT.signal_wait(0x1);
        
        core_util_critical_section_enter();
        TempTs = Ts;
        TempTr = Tr;
        position = revsCount;
        vel = 20* (position - oldPos);
        TempVel = vel;
        core_util_critical_section_exit();
        
        oldPos=position;
        if(TempVel >= 0){
            T = std::min(TempTr,TempTs);
            pwm_out.write(T/100);
            }
        else{
            T = std::max(TempTr, TempTs);
            pwm_out.write(T/100);
            }
    }
}

// BUG TO FIX --> pwm_out goes back to max value after this function
void set_velocity(){
   
    float velocity;
    Ticker statusPrintTicker;
    photoISR();
    float Kps = 5.4; //Experimentally-defined constant
    float Kis = 1.8; //Experimentally-defined constant
    float es_integral = 0;
    float es;
    float sign;
    float timeStep = 0.1; //used for integral scaling
    float tempTs;

    while(1){  
        core_util_critical_section_enter();
        velocity = vel;
        core_util_critical_section_exit();
        
        if (!vel){
            core_util_critical_section_enter();
           Ts=0; 
           Tr=0;
             core_util_critical_section_exit();
            }
        sign = 1;
        // add wait for timeStep to count integral unit
        wait(timeStep);
        es = glob_target_speed - abs(velocity);
        if(es < 0) sign = -1;
        es_integral += es * timeStep;
       
        tempTs = (Kps * es  +  Kis * es_integral) * sign;
        core_util_critical_section_enter();
        Ts = tempTs;
         core_util_critical_section_exit(); 
        
    }
    
}


void Rotate(){
    // check rotation direction
    float tempRevs;
    core_util_critical_section_enter();
    tempRevs = revs;
    core_util_critical_section_exit();
    if (revs < 0) lead = -2;
    else lead = 2;
    tempRevs = abs(revs);
    photoISR(); //start
    int targetRevs;
    core_util_critical_section_enter();
    targetRevs = tempRevs + revsCount;
    core_util_critical_section_exit();
    
    int velocity;
    float Kpr = 17.2;
    float Kdr = 26.6;
    int currRevs = 0;
    bool targetMet = false;
    float tempTr;

    while(1){
        core_util_critical_section_enter();
        velocity = vel;
        currRevs = revsCount;
        core_util_critical_section_exit();
        
        tempTr = Kpr * (targetRevs - (float)currRevs) - Kdr * (float)vel; 
        
        core_util_critical_section_enter();
        Tr = tempTr;
        core_util_critical_section_exit();
    }   
        
    }


#endif


