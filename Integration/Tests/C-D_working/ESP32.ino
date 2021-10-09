//#define RXD2 3
//#define TXD2 1
//
//
//
//void setup() {
//  // put your setup code here, to run once:
//  Serial.begin(115200);
//  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
//}
//
//void loop() {
//  // put your main code here, to run repeatedly:
//  int count = 0;
//  while(1){
////  char thisChar = 'h';
//  Serial.print("Data Recieved:");
//  Serial.println(Serial.read());
//  Serial.write(count);
//  Serial2.write(count);
//  delay(200);
//  count++;
//}}


#define RXD2 16
#define TXD2 17

void setup() {
  Serial.begin(115200); //arduino
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2); //computer

}

void loop() {

  int count = 1;
  while(count)
  {
    Serial.print("Sending to Drive and Printing: ");
    Serial.println(count);
    Serial.println("Sent to Drive");
    Serial2.println(count);

    if(Serial2.available())
    {
      Serial.print("Received from Drive: ");
      Serial.println(Serial2.readStringUntil('\n'));
      
    }

    count ++;
    delay (2000);
  }
  
}
