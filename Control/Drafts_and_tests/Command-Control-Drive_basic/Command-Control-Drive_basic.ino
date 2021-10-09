#define RXD2 3
#define TXD2 1
//#include <string.h>


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);

  /*  
   *  Serial2 seems to be the one that communicates via USB with the computer from ESP32
   *  Serial(0) is the one communicating via USB with the computer on Arduino Nano
   *  The two modules are connected as follows: (ESP32)Serial <-> Serial1(Arduino Nano)
   */
}


void loop() {
  
  int counter = 1;
  while(1)
  {
    //char thisChar = 'h';
    //Serial.print("Data Recieved: ");
    //Serial.println(Serial.read());
    //Serial.write(counter);
    //Serial.println();
    //Serial2.print(counter);
    //Serial2.write("OH OH \n");
    /*if(Serial.read() > 0)
    {
      //String input = Serial2.read();
      Serial2.print("Received from console: ");
      Serial2.println(Serial.readString());
    }*/

    Serial2.println("Printing from Serial2...");
    Serial.println("Printing from Serial...");
    
    delay(200);
    counter++;
  }
}
