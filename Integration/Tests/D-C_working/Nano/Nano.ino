#include <Arduino_JSON.h>


void setup() {
  Serial1.begin(115200, SERIAL_8N1); //esp
  Serial.begin(115200); //computer
}

void loop() {

  int count = 1;
  int position_x, position_y, rover_status;
  String from_control, dir, value;
  JSONVar parsed_from_control;

  
  while(count)  // [testing purposes] it does not have to be in a while loop
  {
    Serial.println("Sending to Control...");
    Serial1.print(position_x);
    Serial1.print(",");
    Serial1.print(position_y);
    Serial1.print(",");
    Serial1.println(rover_status);
    Serial.println("Sent to Control");

    //discuss if other relevant values/parameters shoud be sent
    //mainly need to agree with command
    
    
    if(Serial1.available())
    {
      from_control = Serial1.readStringUntil('\n');
      Serial.print("Received from Control: ");
      Serial.println(from_control);
      
    }

    parsed_from_control = JSON.parse(from_control);

    dir = parsed_from_server["direction"];
    value = parsed_from_server["value"];
    //use dir and value to your liking

    count ++;
    delay (500);
  }

}


    
