#include <HardwareSerial.h>

HardwareSerial DriveSerial(2);

void setup() {
    DriveSerial.begin(115200, SERIAL_8N1, 16, 17);
}

void loop() {
    while (DriveSerial.available()) {
        uint8_t byteFromSerial = DriveSerial.read();
        // Do something
    }
    
    //Write something like that
    DriveSerial.write("TESTING");
}
