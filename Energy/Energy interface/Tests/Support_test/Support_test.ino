
void setup() 
{
  Serial.begin(9600);
}

void loop() 
{
  int k = 33, t = 90;
  while(1)
  {
//    Serial.print(k);
//    Serial.print(",");
    Serial.println(t);
    if(Serial.available())
    {
      String command = Serial.readStringUntil('\n');
      //Serial.print("Received command: ");// + command);
      //Serial.println(command);
    }
    //Serial.println(t);
    k ++; 
    t ++;
    delay(1000);
  }
}
