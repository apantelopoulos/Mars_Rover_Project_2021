#define RXD2 3
#define TXD2 1



void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
}

void loop() {
 
  int count = 0;
  
  while(1){
    //char thisChar = 'h';
    Serial.print("Data Recieved:");
    Serial.println(Serial.read());
    Serial.write(count);
    Serial2.write(count);
    delay(200);
    count++;
  }


}
