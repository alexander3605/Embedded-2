#ifndef Coms_h
#define Coms_h

// Internal Comms

// Message
typedef struct {
    int num; 
    char* body;
    uint64_t keyP;
} msg_t;

Mail<msg_t, 256> msg_box;

void Receiver(){
    while (true) {
        osEvent evt = msg_box.get();
        if (evt.status == osEventMail) {
            msg_t *msg = (msg_t*)evt.value.p;
            char* texttt = msg->body;
            int num = msg->num;
            uint64_t keyP = msg->keyP;
            pc.printf("%d %s %llu\n", num, texttt, keyP);
            
            msg_box.free(msg);
        }
    }
}


void putMessage(int n, char* s, uint64_t keyP){
    pc.printf(" "); // DO NOT REMOVE - it solves a problem 
    pc.printf(" "); // DO NOT REMOVE - it solves a problem
    msg_t *msg = msg_box.alloc();
    msg->num = n;
    msg->body = s;
    msg->keyP = keyP;
    msg_box.put(msg);   
}


// Serial Comms

void setKey(string tempKey){
    uint64_t newKey;
    istringstream iss(tempKey);
    iss >> newKey;

    NewKey_Mutex.trylock();
    *key = (uint64_t)newKey;
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
            string input = command.substr(1);
            float newrevs = atof(input.c_str());
            revs = newrevs;
            break;}

        case 'v' : 
        case 'V' : {
            string input = command.substr(1);
            float speed = atof(input.c_str());
            glob_target_speed = speed;
            break;}

        case 'k' :
        case 'K' : {
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
    }
    command = "";
    }
    
}




#endif

