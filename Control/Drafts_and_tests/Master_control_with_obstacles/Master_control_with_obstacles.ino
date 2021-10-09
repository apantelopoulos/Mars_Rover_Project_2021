#include <SPI.h>
#include <WiFi.h>
#include <Vector.h>
#include <WiFiMulti.h>
#include <Arduino_JSON.h>
#define RXD1 4
#define TXD1 2
#define RXD2 16
#define TXD2 17

WiFiMulti WiFiMulti;

//Server connection setup
const uint16_t port = 8080;        // Server port
const char * host = "18.218.20.25"; // IP or DNS
int old_id = 0;                    // Used for Command <-> Drive
//int old_position_x = -1;
//int old_position_y = -1;
int timeout_serial_read = 10;      //Speed up any Serial String read, default is set to 1000 ms
int timeout_serial1_read = 50;
float timeout_client_read = 0.001; //Speed up any Client String read, default is set to 1 s
int was_first = 1, status_change = 0, waitlist_iterator = 0, obstacles_iterator = 0;
float status_from_drive = 0, x_0 = 0, y_0 = 0, obstacle_encountered = 0;

typedef Vector<String> Elements;
typedef Vector<float> Obstacles;
const int ELEMENT_COUNT_MAX = 0;
int storage_array[ELEMENT_COUNT_MAX];

Elements waitlist;
//waitlist.setStorage(storage_array);
Obstacles obstacles_y;
//obstacles_y.setStorage(storage_array);
Obstacles obstacles_x;
//obstacles_x.setStorage(storage_array);


void update_command()
{
  if (waitlist.size() == 0)
    return;

  if (status_from_drive != 1) {
    status_change  = 0;
    return;
  }

  if (status_change == 0 && status_from_drive == 1 && waitlist_iterator < waitlist.size()) //stationary
  {
    //send new command
    Serial1.println(waitlist[waitlist_iterator]);
    waitlist_iterator ++;
    status_change = 1;
  }
  if (waitlist_iterator == waitlist.size() - 1)
    obstacle_encountered = 0;
}


void avoid_obstacle()
{

  //working with mm only!

  //x_0 = 0; //rover actual x pos
  //y_0 = 0;//rover actual y pos

  String temp;
  float degree, degree_right, difference, temp_fwd;

  if (obstacle_encountered == 1)
  {
    //obstacle_encountered = 0;
    return;
  }


  for (int i = 0; i < obstacles_y.size(); i ++)
  {
    if (abs(obstacles_y[i]) - abs(y_0) < 300) //mm
    {
      if (abs(obstacles_x[i]) - abs(x_0) <= 22) //mm
      {
        obstacle_encountered = 1;

        temp  = "{\"id\" : 6969, \"direction\" : \"stop\", \"value\" : \"\"}";
        //std::vector
        Serial1.println(temp);


        //calculate turn degree
        degree = atan(220 / (obstacles_y[i] - y_0));
        degree = degree * 180 / 3.141592;
        String degree_s = String(degree);
        temp  = "{\"id\" : 6969, \"direction\" : \"left\", \"value\" : \"" + degree_s + "\"}";
        waitlist.push_back(temp);

        difference = abs(obstacles_y[i]) - abs(y_0);
        temp_fwd = sqrt(220 * 220 + difference * difference);
        String temp_fwd_s = String(temp_fwd);
        temp = "{\"id\" : 6969, \"direction\" : \"forward\", \"value\" : \"" + temp_fwd_s + "\"}";
        waitlist.push_back(temp);

        degree_right = 2 * degree;
        String degree_right_s = String(degree_right);
        temp = "{\"id\" : 6969, \"direction\" : \"right\", \"value\" : \"" + degree_right_s + "\"}";
        waitlist.push_back(temp);

        temp = "{\"id\" : 6969, \"direction\" : \"forward\", \"value\" : \"" + temp_fwd_s + "\"}";
        waitlist.push_back(temp);

        temp  = "{\"id\" : 6969, \"direction\" : \"left\", \"value\" : \"" + degree_s + "\"}";
        waitlist.push_back(temp);
      }
    }
  }
}


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
  //if there are memory problems, maybe use char * instead of String
  String from_server, from_drive, from_fpga, to_server = "", to_drive, to_fpga, dir, value;
  String fpga_nothing = "999999,999999", from_fpga_truncated_green, from_fpga_truncated_purple, colour, green = "1,", purple = "2,", nothing = "0,";
  String fpga_col, fpga_dist_y, fpga_dist_x, drive_0, drive_1, drive_2, drive_3, drive_4, drive_5, drive_6;
  float fpga_dist_y_float, fpga_dist_x_float, drive_0_float, drive_1_float, drive_2_float, drive_3_float, drive_4_float, drive_5_float, drive_6_float;
  float rov_mid_x, rov_mid_y, rov_angle, cam_to_obs_fwd, cam_to_obs_side, mid_to_cam, mid_to_obs_fwd, mid_to_obs_side, mid_to_obs_abs, obs_angle_from_rov, obs_angle_total, obs_x_from_mid, obs_y_from_mid, obs_x, obs_y;
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


    /*
      if(command_id != old_id && was_first){
      was_first = 0;
      old_id = command_id;
      }
    */


    //Receive data from FPGA
    Serial1.println('s');
    if (Serial1.available())
    {
      fpga_col = Serial1.readStringUntil(',');
      fpga_dist_y = Serial1.readStringUntil(',');
      fpga_dist_x = Serial1.readStringUntil('k');
      Serial.println("Control module received from FPGA: " + fpga_col + fpga_dist_y + ',' + fpga_dist_x);
      //Serial.println(from_fpga);
      fpga_data = 1;
      fpga_dist_y_float = fpga_dist_y.toFloat();
      fpga_dist_x_float = fpga_dist_x.toFloat();
      //colour = from_fpga.substring(0,3);
      //from_fpga_truncated = from_fpga.substring(4);

      if (fpga_col == "GGG")
      {
        if (!is_green)
        {
          obstacles_y[obstacles_iterator] = fpga_dist_y_float;
          obstacles_x[obstacles_iterator] = fpga_dist_x_float;
          obstacles_iterator ++;
        }
        is_green ++;
        //obstacle_encountered = 1;
        if (is_green < 3)
        {
          from_fpga_truncated_green = green + fpga_dist_y + ',' + fpga_dist_x;
          can_send = 1;
        }
      }

      if (fpga_col == "PPP")
      {
        if (!is_purple)
        {
          obstacles_y[obstacles_iterator] = fpga_dist_y_float;
          obstacles_x[obstacles_iterator] = fpga_dist_x_float;
          obstacles_iterator ++;
        }
        is_purple ++;
        //obstacle_encountered = 1;
        if (is_purple < 3)
        {
          from_fpga_truncated_purple = purple + fpga_dist_y + ',' + fpga_dist_x;
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


    //Send data to Drive
    if (command_id != old_id) // && !was_first)
    {
      if (obstacle_encountered == 1 || waitlist_iterator >= waitlist.size() - 1)
      {
        update_command();
        avoid_obstacle();
      }
      else //NOT SURE IF JUST ELSE OR ELSE IF
      {
        old_id = command_id;
        to_drive = from_server;
        Serial2.println(to_drive);
        //  Serial.print(F("Sent to Drive: "));
        //  Serial.println(to_drive);
        delay(10); //optional
      }
    }


    //Receive data from Drive
    if (Serial2.available() > 0)
    {
      drive_0 = Serial2.readStringUntil(',');
      drive_1 = Serial2.readStringUntil(',');
      drive_2 = Serial2.readStringUntil(',');
      drive_3 = Serial2.readStringUntil(',');
      drive_4 = Serial2.readStringUntil(',');
      drive_5 = Serial2.readStringUntil(',');
      drive_6 = Serial2.readStringUntil('\n');
      //drive_7 = Serial2.readStringUntil('\n');
      Serial.println("Received from Drive: " + drive_0 + ',' + drive_1 + ',' + drive_2 + ',' + drive_3 + ',' + drive_4 + ',' + drive_5 + ',' + drive_6);
      //Serial.println(from_drive);

      drive_6.replace("\n", ",");
      drive_6.replace("\r", ",");
      from_drive = drive_0 + ',' + drive_1 + ',' + drive_2 + ',' + drive_3 + ',' + drive_4 + ',' + drive_5 + ',' + drive_6;

      drive_0_float = drive_0.toFloat();
      drive_1_float = drive_1.toFloat();
      drive_2_float = drive_2.toFloat();
      drive_3_float = drive_3.toFloat();
      drive_4_float = drive_4.toFloat();
      drive_5_float = drive_5.toFloat();
      drive_6_float = drive_6.toFloat();
      status_from_drive = drive_6_float;
      x_0 = drive_3_float; //???
      y_0 = drive_4_float; //???
      //drive_7_float = drive_7.toFloat();
    }


    //convert FPGA readings for server use
    rov_mid_x = drive_3_float; //mm
    rov_mid_y = drive_4_float; //mm
    rov_angle = drive_6_float; //mm //of rover from its original orientation (during last time it was reset)

    //100, 1, 2, 3, 4, 5, 6, GGG, 7, 8

    cam_to_obs_fwd = fpga_dist_y_float * 10;   //mm
    cam_to_obs_side = fpga_dist_x_float * 10;  //mm

    mid_to_cam = 75; //in   //rov_mid_x, rov_mid_y, rov_angle, cam_to_obs_fwd, cam_to_obs_side, mid_to_cam, mid_to_obs_fwd, mid_to_obs_side, mid_to_obs_abs, obs_angle_from_rov, obs_angle_total, obs_x_from_mid, obs_y_fom_mid, obs_x, obs_y;

    mid_to_obs_fwd = mid_to_cam + cam_to_obs_fwd;
    mid_to_obs_side = cam_to_obs_side;

    mid_to_obs_abs = sqrt(sq(mid_to_obs_fwd) + sq(mid_to_obs_side));

    obs_angle_from_rov = atan(mid_to_obs_side / mid_to_obs_fwd);
    obs_angle_total = rov_angle + obs_angle_from_rov;

    obs_x_from_mid = mid_to_obs_abs * sin(obs_angle_total);
    obs_y_from_mid = mid_to_obs_abs * cos(obs_angle_total);

    obs_x = rov_mid_x + obs_x_from_mid;// do we detect cam_to_obs distances as distance to the obstacle's centre?
    obs_y = rov_mid_y + obs_y_from_mid;






    //Send all data to Command
    if (fpga_data && colour == "GGG")
      to_server = from_drive + green + String(obs_x) + ',' + String(obs_y);
    else if (fpga_data && colour == "PPP")
      to_server = from_drive + purple + String(obs_x) + ',' + String(obs_y);
    else if (!fpga_data)
      to_server = from_drive + nothing + fpga_nothing;

    delay(10);
    Serial.print(F("Control -> Command: "));
    Serial.println(to_server);
    client.print(to_server);
    Serial.println(F(""));




    /*
      else
      {
      client.print(' ');
      to_server = "";
      Serial.print(F("Control -> Command: "));
      Serial.println(to_server);
      //this is to make sure the server always receives something so that it can send back commands
      //part of the Drive if statement because if the rover doesn't move, the obstacles won't either
      }*/

    delay(250);
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
