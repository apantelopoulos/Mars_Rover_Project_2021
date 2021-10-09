
//other global declarations and definitons here

void setup() 
{
  Serial1.begin(115200, SERIAL_8N1); //used to communicate with Control
  Serial.begin(115200); //used to communicate with computer
}

void loop() 
{

  //receive data from Control
  if(Serial1.available())
  {
    String from_control = Serial1.readStringUntil('\r');
    //use '\n' as an escape sequence is errors occur
    Serial.print("Drive module received: ");
    Serial.println(from_control);
    delay(100); //breathing time for the processor, can be removed if necessary
  }
  

  //send data to Control
  //declare a variable (preferably String for versatility)
  //to use as packet which contains all the relevant data
  //(speed, direction etc.) to Control
  String outbound_data;
  //outbound_data = whatever is to be sent
  Serial.println();
  Serial.println("Sending status to Control...");
  Serial1.println(outbound_data);
  delay(100); //breathing time, can be removed if necessary
  
}




    
