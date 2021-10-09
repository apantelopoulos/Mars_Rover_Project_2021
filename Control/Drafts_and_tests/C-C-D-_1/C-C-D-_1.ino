#include <SPI.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <Arduino_JSON.h>
#define RXD2 16
#define TXD2 17


WiFiMulti WiFiMulti;

//Server connection setup
const uint16_t port = 8080;
const char * host = "3.129.44.87"; // ip or dns
int old_id = 0;
//int old_position_x = -1;
//int old_position_y = -1;
int timeout_serial_read = 10;      //Speed up any Serial String read, default is set to 1000 ms
float timeout_client_read = 0.001; //Speed up any Client String read, default is set to 1 s

void setup()
{

  // initialise board connections

  Serial.begin(115200); //Computer
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2); //Arduino
  Serial2.setTimeout(timeout_serial_read); //Speed up reading from Arduino
  delay(10);

  /*
     Serial is the one that communicates via USB with the computer from ESP32
     Serial2 is the one communicating via Rx/Tx with the processor on Arduino Nano
     The two modules are connected as follows: (ESP32)Serial2 <-> Serial1(Arduino Nano)
  */

  // initialise WiFi connection
  WiFiMulti.addAP("AAAAAAA", "BBBBBBBB");

  Serial.println(F(""));
  Serial.println(F(""));
  Serial.print(F("Waiting for WiFi... "));

  while (WiFiMulti.run() != WL_CONNECTED)
  {
    Serial.print(F("."));
    delay(100);
  }

  Serial.println(F(""));
  Serial.println(F("WiFi connected"));
  Serial.print(F("IP address: "));
  Serial.println(WiFi.localIP());

  Serial.print(F("Connecting to "));
  Serial.println(host);

  delay(50);
}



void loop ()
{

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  client.setTimeout(timeout_client_read);   //Speed up reading from server

  int command_id;
  String from_server, from_drive, to_server, to_drive, dir, value;
  JSONVar parsed_from_server;


  if (!client.connect(host, port))    //Try to connect to server and send result
  {
    Serial.print(F("Cannot connect to server: "));
    Serial.println(F("Waiting 3 seconds before retrying..."));
    delay(3000);
    return; //restart the loop
  }


  while (true)
  {
    //Send whatever is needed to server via client.print()
    //Usually, this data will come from Serial2.read(), which
    //is the Drive module


    //SUPPORT IS NEEDED FOR THE SPI INTERFACE WITH THE FPGA


    //uncomment this line to send a basic document request to the server
    //client.print("GET /index.html HTTP/1.1\n\n");

    if (!client.connected())    //Checks for server connection
    {
      Serial.println(F("Connection to server lost"));
      Serial.println(F("Waiting 3 seconds before retrying..."));
      delay(3000);
      return; //restart the loop
    }


    //Receive data from Command
    if (client.available() > 0)
    {
      Serial.println(F("Received from server: "));
      from_server = client.readStringUntil('\n');
      Serial.println(from_server);
    }
    else
    {
      Serial.println(F("No commands from server"));
    }


    //Decode data from Command
    parsed_from_server = JSON.parse(from_server);
    command_id = parsed_from_server["id"];
    dir = parsed_from_server["direction"];
    value = parsed_from_server["value"];

    Serial.print("Command id: ");
    Serial.println(parsed_from_server["id"]);
    Serial.println("Command direction: " + dir);
    Serial.println("Command value: " + value);
    //delay(50); //optional


    //Send data to Drive
    if (old_id != command_id)
    {
      old_id = command_id;
      to_drive = from_server;
      Serial2.println(to_drive);
      //  Serial.print(F("Sent to Drive: "));
      //  Serial.println(to_drive);
      //  delay(50); //optional
    }


    //Receive data from Drive and send to Command
    if (Serial2.available() > 0)
    {
      from_drive = Serial2.readStringUntil('\n');
      from_drive.replace("\n", "");

      to_server = "555,0,0,2";    
      //REPLACE THIS WITH to_server = from_drive OR JUST SEND from_drive

      Serial.print(F("Drive -> Command: "));
      Serial.println(to_server);
      client.print(to_server);
      Serial.println(F(""));
    }
    else
    {
      client.print(' ');
      //this is to make sure the server always receives something so that it can send back commands
    }

    delay(500); //optional

  } //while(true)



  /*This last part is left for further development
    since client disconnection might be a nice
    feature to have in the final demo

    Serial.println("Closing connection.");
    client.stop();

    Serial.println("Waiting 5 seconds before restarting...");
    delay(5000);
  */


}
