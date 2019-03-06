#ifndef Coms.h
#define Coms.h

// Internal Comms

// Message
typedef struct {
    int id;          /* Who is sending msg*/
    int    currState;    /* AD result of measured voltage */
    int    orState;      /* AD result of measured current */
    uint32_t counter;    /* A counter value               */
} msg_t;

Mail<msg_t, 16> msg_box;

void Receiver(){
    while (true) {
        osEvent evt = msg_box.get();
        if (evt.status == osEventMail) {
            msg_t *msg = (msg_t*)evt.value.p;
            pc.printf("\nHi!, My name is %d\n\r", msg->id);
            
            msg_box.free(msg);
        }
    }
}

void putMessage(int id, int voltage, int current, uint32_t counter) {
    msg_t *msg = msg_box.alloc();
    msg->id = id;
    msg->currState = voltage;
    msg->orState = current;
    msg->counter = counter;
    msg_box.put(msg);
}


// Serial Comms

void setKey(string tempKey){
    uint64_t newKey;
    istringstream iss(tempKey);
    iss >> newKey;

    NewKey_Mutex.trylock();
    KEY = newKey;
    NewKey_Mutex.unlock();

}

Queue<void, 8> inCharQ;
void serialISR(){
    uint8_t newChar = pc.getc();
    inCharQ.put((void*)newChar);
}

void decodeSerialInput(){
    string command;
    pc.attach(&serialISR);
    while(1) {
        while(1) {
        osEvent newEvent = inCharQ.get();
        uint8_t newChar = (uint8_t)newEvent.value.p;
        newChar = (unsigned char) newChar;
        if(newChar != '\r')
            command += newChar;
        else break;
    }
    
    char instr = command[0];
    switch (instr){
        case 'r' :
        case 'R' :{
            string revs = command.substr(2);
            revs = atof(revs.c_str());
//            Rotate(revs);
            break;}

        case 'v' : 
        case 'V' : {
            string speed = command.substr(1);
            speed = atof(speed.c_str());
//            MaxSpeed(speed);
            break;}

        case 'k' :
        case 'K' : {
            pc.printf("Finished typing");
            string tempKey = command.substr(1);
            setKey(tempKey);
            break;
            }

        case 't' :
        case 'T' :{
            string tune = command.substr(1);
//            Sing(tune);
            break;
            }
        default : int a = 5; 
//       throw("The fuck man?")
    }
    command = "";
    }
    
}




#endif
