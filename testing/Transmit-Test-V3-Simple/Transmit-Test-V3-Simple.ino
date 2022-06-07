/*
 * 
 */

#include <SparkFun_Swarm_Satellite_Arduino_Library.h> //Click here to get the library:  http://librarymanager/All#SparkFun_Swarm_Satellite

const uint8_t ledPin = 29;
uint32_t swarmStatusUpdateDelay = 10000;
uint32_t nextSwarmStatusUpdate = 0;
uint32_t swarmHoldTime = 3600;
uint16_t loopUpdateDelay = 1000;

SWARM_M138 mySwarm;

void setLed(bool state = false){
    digitalWrite(ledPin, state);
}

void printSwarmStatus(){
    Swarm_M138_Receive_Test_t *rxTest = new Swarm_M138_Receive_Test_t; // Allocate memory for the information
    mySwarm.getReceiveTest(rxTest);

    uint16_t count;
    mySwarm.getUnsentMessageCount(&count);
    
    if (rxTest->background) // Check if rxTest contains only the background RSSI
    {
        Serial.print("RSSI: ");
        Serial.print(rxTest->rssi_background);
    }
    else{
       Serial.print("RSSI SAT: ");
        Serial.print(rxTest->rssi_sat);
    }

    Serial.print("\tMessages in queue: ");
    Serial.println(count);
    
    delete rxTest; // Free the memory
}

void queueSwarm(uint32_t holdTime = 3600){
    setLed(true);
    delay(500);
    const char myMessage[] = "Swarm testing continued!";
    Serial.print("Queuing message: ");
    Serial.println(myMessage);

    while(true){
        uint64_t id;
        Swarm_M138_Error_e err = mySwarm.transmitTextHold(myMessage, &id, holdTime);
  
        // Check if the message was queued successfully
        if (err == SWARM_M138_SUCCESS)
        {
          Serial.print(F("Message queued! Hold time: "));
          Serial.print(holdTime);
          Serial.print("\tID: ");
          serialPrintUint64_t(id);
          Serial.println();
          setLed(false);
          break;
        }
        else
        {
          Serial.print(F("Swarm communication error: "));
          Serial.print((int)err);
          Serial.print(F(" : "));
          Serial.print(mySwarm.modemErrorString(err)); // Convert the error into printable text
          if (err == SWARM_M138_ERROR_ERR) // If we received a command error (ERR), print it
          {
            Serial.print(F(" : "));
            Serial.print(mySwarm.commandError); 
            Serial.print(F(" : "));
            Serial.println(mySwarm.commandErrorString((const char *)mySwarm.commandError)); 
          }
          else
            Serial.println();
        }
        delay(5000);
    }
}

void setup()
{
  pinMode(ledPin, OUTPUT);
  setLed(true);
  
  Serial.begin(115200);
  delay(5000);

  Serial.println("Swarm testing V3");

  //mySwarm.enableDebugging(); // Uncomment this line to enable debug messages on Serial
  Serial1.setTX(0);
  Serial1.setRX(1);

  Serial.println("Connecting to Swarm modem...");
  while(mySwarm.begin(Serial1) == false){
      Serial.println(F("Could not communicate with the modem..."));
      delay(5000);
  }
  Serial.println("Connected to Swarm modem");

  Serial.println("Setup complete!");
  setLed(false);
  mySwarm.deleteAllTxMessages();
}

void loop()
{
    uint16_t swarmMsgCount;
    mySwarm.getUnsentMessageCount(&swarmMsgCount);

    if(swarmMsgCount == 0){
        queueSwarm(swarmHoldTime);
    }

    if(millis() > nextSwarmStatusUpdate){
        printSwarmStatus();
        nextSwarmStatusUpdate = millis() + swarmStatusUpdateDelay;
    }

    delay(loopUpdateDelay);
}

void serialPrintUint64_t(uint64_t theNum)
{
  // Convert uint64_t to string
  // Based on printLLNumber by robtillaart
  // https://forum.arduino.cc/index.php?topic=143584.msg1519824#msg1519824
  
  char rev[21]; // Char array to hold to theNum (reversed order)
  char fwd[21]; // Char array to hold to theNum (correct order)
  unsigned int i = 0;
  if (theNum == 0ULL) // if theNum is zero, set fwd to "0"
  {
    fwd[0] = '0';
    fwd[1] = 0; // mark the end with a NULL
  }
  else
  {
    while (theNum > 0)
    {
      rev[i++] = (theNum % 10) + '0'; // divide by 10, convert the remainder to char
      theNum /= 10; // divide by 10
    }
    unsigned int j = 0;
    while (i > 0)
    {
      fwd[j++] = rev[--i]; // reverse the order
      fwd[j] = 0; // mark the end with a NULL
    }
  }

  Serial.print(fwd);
}
