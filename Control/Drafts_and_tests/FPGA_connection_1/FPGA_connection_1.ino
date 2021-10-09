#define RXD1 4
#define TXD1 2

//CHANGES TO HARDWARE SERIAL NEEDED
//SET UP A MEETING FOR DETAILS
// RX1 BECOMES 4
// TX1 BECOMES 2

int timeout_serial_read = 50;      //Speed up any Serial String read, default is set to 1000 ms

void setup() 
{
  Serial.begin(115200); //Computer
  Serial1.begin(115200, SERIAL_8N1, RXD1, TXD1);
  Serial1.setTimeout(timeout_serial_read); //Speed up reading from FPGA
}

void loop() 
{
  if(Serial1.available())
  {
    String from_fpga = Serial1.readStringUntil('\n');
    Serial.print("Control module received: ");
    Serial.println(from_fpga);
  }
  else
    Serial.println("Nothing from FPGA");
  
  char outbound_data = 't';
  //Could potentially use a character to tell the FPGA
  //to stop processing obstacles while obstacle avoidance 
  //is currently happening.

  //This idea could make or break, since there may be other obstacles
  //that show up during the first obstacle avoidance maneuver and 
  //they would not be processed as obstacles, so the rover could
  //drive into them.
  
  Serial.println("Sending data to FPGA...");
  Serial1.println(outbound_data);
  Serial.println();
  delay(1000); //breathing time, can be removed if necessary
  
}
