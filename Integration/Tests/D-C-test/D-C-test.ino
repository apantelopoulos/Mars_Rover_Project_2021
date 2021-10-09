

void setup() {
  Serial1.begin(115200, SERIAL_8N1); //esp
  Serial.begin(115200); //computer
}

void loop() {

  int count = 1;
  while(count)
  {
    Serial.print("Sending to Control and Printing: ");
    Serial.println(count);
    Serial.println("Sent to Control");
    Serial1.println(count);

    if(Serial1.available())
    {
      Serial.print("Received from Control: ");
      Serial.println(Serial1.readStringUntil('\n'));
      
    }

    count ++;
    delay (2000);
  }

}
