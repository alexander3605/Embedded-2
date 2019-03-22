#ifndef Coms_h
#define Coms_h

// Internal Comms

// Message
typedef struct {
    int num;          /* Who is sending msg*/
//    int    currState;    /* AD result of measured voltage */
//    int    orState;      /* AD result of measured current */
//    uint32_t counter;    /* A counter value               */
    char* body;
    uint64_t keyP;
} msg_t;

Mail<msg_t, 256> msg_box;

void Receiver(){
    while (true) {
        osEvent evt = msg_box.get();
        if (evt.status == osEventMail) {
            msg_t *msg = (msg_t*)evt.value.p;
//            pc.printf("\nHi!, My name is \n\r");
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
        case 'a' : 
        case 'A' : {
            putMessage(0, "--- IN A\n\r", 0);
            string input= command.substr(1);
            float torque = atof(input.c_str());
            if (torque >= 0 ) {
                lead = 2;
                pwm_out = torque;}
            else {
                lead = -2;
                pwm_out = -torque;}
            break;}
        
        case 'r' :
        case 'R' :{
            putMessage(0, "--- IN R\n\r", 0);
            string input = command.substr(1);
            float newrevs = atof(input.c_str());
            revs = newrevs;
            putMessage(0, "---Revs set---\n\r", 0);
            break;}

        case 'v' : 
        case 'V' : {
            pc.printf("Finished typing");
            string input = command.substr(1);
            float speed = atof(input.c_str());
            glob_target_speed = speed;
            pc.printf("--Velocity set--");
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
    }
    command = "";
    }
    
}




#endif

