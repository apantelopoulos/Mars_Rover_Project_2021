void setup() {
  // put your setup code here, to run once:
  Serial1.begin(115200, SERIAL_8N1);
  Serial.begin(115200);
}

void loop() {
  // put your main code here, to run repeatedly:
   if(Serial1.available()){
  char data_rcvd = Serial1.read();
  Serial.print("Driving module received");
  Serial.println(data_rcvd);
  }
}




    
