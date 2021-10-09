#include <SPI.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <Arduino_JSON.h>
#define RXD1 4
#define TXD1 2
#define RXD2 16
#define TXD2 17

WiFiMulti WiFiMulti;

//Server connection setup
const uint16_t port = 8080;        // Server port
const char * host = "3.129.44.87"; // IP or DNS
int old_id = 0;                    // Used for Command <-> Drive
//int old_position_x = -1;
//int old_position_y = -1;
int timeout_serial_read = 15;      //Speed up any Serial String read, default is set to 1000 ms
int timeout_serial1_read = 50;  
float timeout_client_read = 0.001; //Speed up any Client String read, default is set to 1 s
int was_first = 1;

void setup()
{

  // initialise board connections

  Serial.begin(115200); //Computer
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2); //Arduino
  Serial1.begin(115200, SERIAL_8N1, RXD1, TXD1); //FPGA
  Serial1.setTimeout(timeout_serial_read); //Speed up reading from FPGA
  Serial2.setTimeout(timeout_serial_read); //Speed up reading from Arduino
  delay(10);

  /*
     Serial is the one that communicates via USB with the computer from ESP32
     Serial2 is the one communicating via Rx/Tx with the processor on Arduino Nano
     The two modules are connected as follows: (ESP32)Serial2 <-> Serial1(Arduino Nano)
  */

  // initialise WiFi connection
  WiFiMulti.addAP("VM9990689", "Tj8vfkdn9xff");

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

  int command_id, fpga_data, is_green = 0, is_purple = 0, can_send = 0;
  String from_server, from_drive, from_fpga, to_server = "", to_drive, to_fpga, dir, value; 
  String fpga_nothing = "999999,999999", from_fpga_truncated_green, from_fpga_truncated_purple, colour, green = "1,", purple = "0,", nothing = "0,";
  JSONVar parsed_from_server;


  if (!client.connect(host, port))    //Try to connect to server and send result
  {
    Serial.print(F("Cannot connect to server: "));
    Serial.println(F("Waiting 3 seconds before retrying..."));
    delay(3000);
    //MIGHT BE WORTH TELLING THE FPGA TO STOP AS WELL
    return; //restart the loop
  }


  while (true)
  {
    //Send whatever is needed to server via client.print()
    //Usually, this data will come from Serial2.read(), which
    //is the Drive module

    if (!client.connected())    //Checks for server connection
    {
      Serial.println(F("Connection to server lost"));
      Serial.println(F("Waiting 3 seconds before retrying..."));
      delay(3000);
      //MIGHT BE WORTH TELLING THE FPGA TO STOP AS WELL
      return; //restart the loop
    }
    can_send = 0;

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

    Serial.print(F("Command id: "));
    Serial.println(parsed_from_server["id"]);
    Serial.println("Command direction: " + dir);
    Serial.println("Command value: " + value);
    delay(10); //optional


   
    //Send data to Drive
    if (command_id != old_id)
    {
      old_id = command_id;
      to_drive = from_server;
      Serial2.println(to_drive);
      //  Serial.print(F("Sent to Drive: "));
      //  Serial.println(to_drive);
        delay(10); //optional
    }

//    if(command_id != old_id && was_first){
//      was_first = 0;
//      old_id = command_id;
//    }


    //Receive data from FPGA
    Serial1.println('s');
    if (Serial1.available())
    {
      from_fpga = Serial1.readStringUntil('k');
      Serial.print(F("Control module received from FPGA: "));
      Serial.println(from_fpga);
      fpga_data = 1;
      colour = from_fpga.substring(0,3);
      //from_fpga_truncated = from_fpga.substring(4);

      if(colour == "GGG")
      {
        is_green ++;
        if(is_green < 3)
        {
          from_fpga_truncated_green = green + from_fpga.substring(4);
          can_send = 1;
        }
      }

      if(colour == "PPP")
      {
        is_purple ++;
        if(is_purple < 3)
        {
          from_fpga_truncated_purple = purple + from_fpga.substring(4);
          can_send = 1;
        }
      }
    }
    else
    {
      Serial.println(F("Nothing from FPGA"));
      fpga_data = 0;
    }

    delay(10);
    //Receive data from Drive and send everything to Command
    if (Serial2.available() > 0)
    {
      from_drive = Serial2.readStringUntil('\n');
      Serial.print(F("Received from Drive: "));
      Serial.println(from_drive);
      
      from_drive.replace("\n", ",");
      from_drive.replace("\r", ",");
      //from_fpga.replace("k", ""); //maybe this doesn't do anything

      if(fpga_data && colour == "GGG")
        to_server = from_drive + from_fpga_truncated_green;
      else if(fpga_data && colour == "PPP")
        to_server = from_drive + from_fpga_truncated_purple;
      else if(!fpga_data)
        to_server = from_drive + nothing + fpga_nothing;

      delay(10);
      Serial.print(F("Control -> Command: "));
      Serial.println(to_server);
      client.print(to_server);
      Serial.println(F(""));
    }
    else
    {
      client.print(' ');
      to_server = "";
      Serial.print(F("Control -> Command: "));
      Serial.println(to_server);
      //this is to make sure the server always receives something so that it can send back commands
      //part of the Drive if statement because if the rover doesn't move, the obstacles won't either
    }

    //Serial.println();
    delay(500);

    //delay(500); //optional

  } //while(true)


  //Could have a reset command from server to close the connection, 
  //although that would mean no reconnection in a real life scenario
  /*This last part is left for further development
    since client disconnection might be a nice
    feature to have in the final demo

    Serial.println("Closing connection.");
    client.stop();

    Serial.println("Waiting 5 seconds before restarting...");
    delay(5000);
  */

}
