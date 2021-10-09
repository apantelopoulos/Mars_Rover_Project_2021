#define RXD2 3
#define TXD2 1

void setup() {
  Serial.begin(115200); //arduino
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2); //computer

}

void loop() {

  int count = 1;
  while(count)
  {
    Serial2.print("Sending to Drive and Printing: ");
    Serial2.println(count);
    Serial2.println("Sent to Drive");
    Serial.println(count);

    if(Serial.available())
    {
      Serial2.print("Received from Drive: ");
      Serial2.println(Serial.readStringUntil('\n'));
      
    }

    count ++;
    delay (2000);
  }
  
}
