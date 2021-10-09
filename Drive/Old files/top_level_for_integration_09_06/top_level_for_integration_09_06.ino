/*
   Program written by Yue Zhu (yue.zhu18@imperial.ac.uk) in July 2020.
   pin6 is PWM output at 62.5kHz.
   duty-cycle saturation is set as 2% - 98%
   Control frequency is set as 1.25kHz.
*/

#include <Wire.h>
#include <INA219_WE.h>
#include "SPI.h"
#include <Arduino_JSON.h>

// these pins may be different on different boards
// this is for the uno
#define PIN_SS        10
#define PIN_MISO      12
#define PIN_MOSI      11
#define PIN_SCK       13

#define PIN_MOUSECAM_RESET     8
#define PIN_MOUSECAM_CS        7

#define ADNS3080_PIXELS_X                 30
#define ADNS3080_PIXELS_Y                 30

#define ADNS3080_PRODUCT_ID            0x00
#define ADNS3080_REVISION_ID           0x01
#define ADNS3080_MOTION                0x02
#define ADNS3080_DELTA_X               0x03
#define ADNS3080_DELTA_Y               0x04
#define ADNS3080_SQUAL                 0x05
#define ADNS3080_PIXEL_SUM             0x06
#define ADNS3080_MAXIMUM_PIXEL         0x07
#define ADNS3080_CONFIGURATION_BITS    0x0a
#define ADNS3080_EXTENDED_CONFIG       0x0b
#define ADNS3080_DATA_OUT_LOWER        0x0c
#define ADNS3080_DATA_OUT_UPPER        0x0d
#define ADNS3080_SHUTTER_LOWER         0x0e
#define ADNS3080_SHUTTER_UPPER         0x0f
#define ADNS3080_FRAME_PERIOD_LOWER    0x10
#define ADNS3080_FRAME_PERIOD_UPPER    0x11
#define ADNS3080_MOTION_CLEAR          0x12
#define ADNS3080_FRAME_CAPTURE         0x13
#define ADNS3080_SROM_ENABLE           0x14
#define ADNS3080_FRAME_PERIOD_MAX_BOUND_LOWER      0x19
#define ADNS3080_FRAME_PERIOD_MAX_BOUND_UPPER      0x1a
#define ADNS3080_FRAME_PERIOD_MIN_BOUND_LOWER      0x1b
#define ADNS3080_FRAME_PERIOD_MIN_BOUND_UPPER      0x1c
#define ADNS3080_SHUTTER_MAX_BOUND_LOWER           0x1e
#define ADNS3080_SHUTTER_MAX_BOUND_UPPER           0x1e
#define ADNS3080_SROM_ID               0x1f
#define ADNS3080_OBSERVATION           0x3d
#define ADNS3080_INVERSE_PRODUCT_ID    0x3f
#define ADNS3080_PIXEL_BURST           0x40
#define ADNS3080_MOTION_BURST          0x50
#define ADNS3080_SROM_LOAD             0x60

#define ADNS3080_PRODUCT_ID_VAL        0x17


INA219_WE ina219; // this is the instantiation of the library for the current sensor

int count = 0;

//these need to be global (found out the hard way; kept resetting dist_to_move and init_x, init_y every loop iteration, after instruction received
String current_status = "stationary";
String prev_status = "stationary";
int current_status_encoded = 1;

float init_x = 0; // hm - check logic against below to be sure
float init_y = 0;

float dist_to_move = 0;
float dist_from_init = 0;

float angle_to_turn = 0;
float angle_from_init = 0;

float radius = 148; // measured from middle of sensor LED to middle of rover - radius of rotation; tweak for more accurate angle performance


float dutyref = 0.5; //added this line to manually control speed from code, instead of knob

float open_loop; // Duty Cycles // removed closed_loop
float iL, current_mA; // Measurement Variables // removed vpd,vb (CL) and vref,dutyref (knob)

float oc = 0; //internal signals //removed ev=0, cv=0, ei=0 (CL)
float Ts = 0.0008; //1.25 kHz control frequency. It's better to design the control period as integral multiple of switching period.
float current_limit = 3.0; // changed to 3 from 1 (set to 3 anyway, for OL Buck)

unsigned int loopTrigger;
unsigned int com_count = 0; // a variables to count the interrupts. Used for program debugging.


//************************** Motor Constants **************************//
int DIRRstate = LOW;              //initializing direction states
int DIRLstate = HIGH;

int DIRL = 20;                    //defining left direction pin
int DIRR = 21;                    //defining right direction pin

int pwmr = 5;                     //pin to control right wheel speed using pwm
int pwml = 9;                     //pin to control left wheel speed using pwm
//*******************************************************************//

//************************** optical sensor vars **************************//
int total_x = 0;
int total_y = 0;

int prev_total_x = -1;
int prev_total_y = -1;

int total_x1 = 0;
int total_y1 = 0;

int x = 0;
int y = 0;

int a = 0;
int b = 0;

int distance_x = 0;
int distance_y = 0;

volatile byte movementflag = 0;
volatile int xydat[2];

int tdistance = 0;
//*******************************************************************//

// for some reason this needed to be moved from functions tab; needs to appear before loop
struct MD
{
  byte motion;
  char dx, dy;
  byte squal;
  word shutter;
  byte max_pix;
};

void setup() {

  //************************** Motor Pins Defining **************************//
  pinMode(DIRR, OUTPUT);
  pinMode(DIRL, OUTPUT);
  pinMode(pwmr, OUTPUT);
  pinMode(pwml, OUTPUT);
  digitalWrite(pwmr, LOW);       //changed from initial HIGH bc set this after instructions decoded
  digitalWrite(pwml, LOW);
  //*******************************************************************//

  //Basic pin setups

  noInterrupts(); //disable all interrupts
  pinMode(13, OUTPUT);  //Pin13 is used to time the loops of the controller
  analogReference(EXTERNAL); // We are using an external analogue reference for the ADC

  // TimerA0 initialization for control-loop interrupt.

  TCA0.SINGLE.PER = 999; //
  TCA0.SINGLE.CMP1 = 999; //
  TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV16_gc | TCA_SINGLE_ENABLE_bm; //16 prescaler, 1M.
  TCA0.SINGLE.INTCTRL = TCA_SINGLE_CMP1_bm;

  // TimerB0 initialization for PWM output

  pinMode(6, OUTPUT);
  TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc | TCB_ENABLE_bm; //62.5kHz
  analogWrite(6, 120);

  interrupts();  //enable interrupts.
  Wire.begin(); // We need this for the i2c comms for the current sensor
  ina219.init(); // this initiates the current sensor
  Wire.setClock(700000); // set the comms speed for i2c


  //************************** optical sensor setup **************************//
  pinMode(PIN_SS, OUTPUT);
  pinMode(PIN_MISO, INPUT);
  pinMode(PIN_MOSI, OUTPUT);
  pinMode(PIN_SCK, OUTPUT);

  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV32);
  SPI.setDataMode(SPI_MODE3);
  SPI.setBitOrder(MSBFIRST);

  Serial.begin(38400);

  if (mousecam_init() == -1)
  {
    Serial.println("Mouse cam failed to init");
    while (1);
  }
  //*******************************************************************//


  //************************** Tx, RX setup **************************//
  Serial1.begin(9600, SERIAL_8N1); // to ESP (TX1, RX1) // change baud rate to 38400 to match sensor?
  Serial.begin(9600); // to computer (TX0, RX0)
  //***********************************************************//
}

byte frame[ADNS3080_PIXELS_X * ADNS3080_PIXELS_Y]; // for some reason this appears after setup in optical sensor code


void loop() {

  unsigned long currentMillis = millis();

  //************************** start SMPS (Buck OL) loop **************************//
  if (loopTrigger) { // This loop is triggered, it wont run unless there is an interrupt
    //digitalWrite(13, HIGH);   // set pin 13. Pin13 shows the time consumed by each control cycle. It's used for debugging.
    sampling();
    // Open Loop Buck
    oc = iL - current_limit; // Calculate the difference between current measurement and current limit
    if ( oc > 0) {
      open_loop = open_loop - 0.001; // We are above the current limit so less duty cycle
    } else {
      open_loop = open_loop + 0.001; // We are below the current limit so more duty cycle
    }
    open_loop = saturation(open_loop, dutyref, 0.02); // saturate the duty cycle at the reference or a min of 0.02
    pwm_modulate(open_loop); // and send it out

  }
  //digitalWrite(13, LOW);   // reset pin13.
  loopTrigger = 0;
  //*******************************************************************************//


  //************************** start optical sensor loop **************************//

  //removed ascii art code - inlcuding #if 0, #else, #endif at end; can find in top_level_og

  // concerning below: 'bar graph' is asterisks, section enabled bc under 'else', whereas prev section under #if 0, which is always false?
  // if enabled this section produces a bar graph of the surface quality that can be used to focus the camera
  // also drawn is the average pixel value 0-63 and the shutter speed and the motion dx,dy.

  int val = mousecam_read_reg(ADNS3080_PIXEL_SUM);
  MD md;
  mousecam_read_motion(&md);
  //commented out diagnostic serial prints below, to disable delays
  /*
    for(int i=0; i<md.squal/4; i++)
    Serial.print('*');
    Serial.print(' ');
    Serial.print((val*100)/351);
    Serial.print(' ');
    Serial.print(md.shutter); Serial.print(" (");
    Serial.print((int)md.dx); Serial.print(',');
    Serial.print((int)md.dy); Serial.println(')');

    // Serial.println(md.max_pix);
    delay(100);
  */

  distance_x = md.dx; //convTwosComp(md.dx);
  distance_y = md.dy; //convTwosComp(md.dy);

  total_x1 = (total_x1 + distance_x);
  total_y1 = (total_y1 + distance_y);

  total_x = 10 * total_x1 / 157; //Conversion from counts per inch to mm (400 counts per inch)
  total_y = 10 * total_y1 / 157; //Conversion from counts per inch to mm (400 counts per inch)

  //commented out diagnostic serial prints below, to disable delays
  
    Serial.print('\n');

    Serial.println("Distance_x = " + String(total_x));

    Serial.println("Distance_y = " + String(total_y));
    Serial.print('\n');

  //  delay(100);
  
  //*******************************************************************//



  //************************** TX/RX to/from Control **************************//

  //rmmbr delays have to be removed for Buck to be fast enough!!

  //would it make a difference if these were global? ah - maybe should reset command so that instructions only received once
  String from_control;
  String command = "none"; //initialize in case UART somehow doesn't yield value
  String magnitude_string;
  float magnitude = 0; //initialize in case UART somehow doesn't yield value

  JSONVar parsed_from_control;

  if (Serial1.available())
  {
    from_control = Serial1.readStringUntil('\n');

    Serial.print("Received from Control: ");
    Serial.println(from_control);


  }

  parsed_from_control = JSON.parse(from_control);

  command = parsed_from_control["direction"];
  magnitude_string = parsed_from_control["value"];
  magnitude = magnitude_string.toFloat();

  delay (20);


  // but requires that instruction is only sent ONCE - otherwise init_pos updated every loop iteration
  if (command == "forward" || command == "back" || command == "left" || command == "right") {
    init_x = total_x;
    init_y = total_y;
    dist_to_move = magnitude;
    angle_to_turn = magnitude; // will only use one of these two anyway; put in same if-statement to avoid repeating code

    if (command == "forward") {
      current_status = "going forward";
    } else if (command == "back") {
      current_status = "going back";
    } else if (command == "left") {
      current_status = "turning left";
    } else if (command == "right") {
      current_status = "turning right";
    }


  } else if (command == "recentre") { // maybe change "dir" to "command"? Should this stop the rover? Should the current command abort?
    total_x = 0;
    total_y = 0; //will these work?? What about orientation? Which direction is x and which y?
  }

  dist_from_init = sqrt(sq(total_x - init_x) + sq(total_y - init_y)); // maybe add dutyref-dependent factor to account for overhsoot during delay??
  radius = 148; // measured from middle of sensor LED to middle of rover - radius of rotation; tweak for more accurate angle performance
  angle_from_init = 2 * degrees( asin(0.5 * dist_from_init / radius) ); //simple trig BUT only works for <90deg; more than that has to be multiple commands


  //but better to keep motor speed low to minimize overhsoot by time conditionals re-evaluated in next loop iteration
  //ifs here control direction of motors depending on instruction
  //notice second condition is a compensation for overshoots (but e.g. still remain in "going back" even though compensate overshoot with a forward motion - 'disguised' from command
  //compensation only kicks in if overshoot is significant e.g. 4mm - for instance when going with high speed
  //experimentally saw that reducing tolerance too much caused oscillation (use absolute rather than fractional because depends on speed and loopt time creating constant overshoot)
  if ( (current_status == "going forward" && dist_from_init < dist_to_move - 5 * dutyref) || (current_status == "going back" && dist_from_init > dist_to_move) ) {
    DIRRstate = HIGH;
    DIRLstate = LOW;
  } else if ( (current_status == "going back" && dist_from_init < dist_to_move - 5 * dutyref) || (current_status == "going forward" && dist_from_init > dist_to_move) ) {
    DIRRstate = LOW;
    DIRLstate = HIGH;
  } else if ( (current_status == "turning left" && angle_from_init < angle_to_turn - 4 * dutyref) || (current_status == "turning right" && angle_from_init > angle_to_turn) ) {
    DIRRstate = HIGH;
    DIRLstate = HIGH;
  } else if ( (current_status == "turning right" && angle_from_init < angle_to_turn - 4 * dutyref) || (current_status == "turning left" && angle_from_init > angle_to_turn) ) {
    DIRRstate = LOW;
    DIRLstate = LOW;
  } else {
    current_status = "stationary";
    //the issue is, once enters stationary state, cannot do compensation
    //so compensation only happens when delays are such that overshoot occurs while still in e.g. "going back"?
  }

  digitalWrite(DIRR, DIRRstate);
  digitalWrite(DIRL, DIRLstate);

  //makes sense to put it separate to above ifs bc here changes pwm, not dir_state
  //also if-statement above ensures goes into "stationary" if dist_from_init > dist_to_move etc.
  if (current_status == "going forward" || current_status == "going back" || current_status == "turning left" || current_status == "turning right") {
    digitalWrite(pwmr, HIGH);
    digitalWrite(pwml, HIGH);
  } else { //just to ensure covers undefined as well as stationary?
    digitalWrite(pwmr, LOW);
    digitalWrite(pwml, LOW);
  }


  //Serial.print(count);
  //Serial.print(",");
  // Serial.print("current_status: " + current_status);
  //  Serial.print(",");
  //  Serial.print("angle_to_turn: " + String(angle_to_turn) );
  //  Serial.print(",");
  //  Serial.print("angle_from_init: " + String(angle_from_init) );
  //  Serial.print(",");
  //  Serial.println("dist_from_init: " + String(dist_from_init) );
  //  delay(20);
  //

  //add re-centre command that resets origin?

  //encoding current_status to send to command
  if (current_status == "going forward") {
    current_status_encoded = 2;
  } else if (command == "back") {
    current_status_encoded = 3;
  } else if (command == "left") {
    current_status_encoded = 5;
  } else if (command == "right") {
    current_status_encoded = 4;
  } else if (current_status == "stationary") {
    current_status_encoded = 1;
  }


  if (count < 100) {
    count++;
  } else {
    count = 0; //this makes count 501 and prevents being stuck at 500, which would keep sending instruction
  } 

  //Serial.println(count);

  //|| (current_status == "stationary")
  if ( (prev_total_x != total_x) || (prev_total_y != total_y) || (prev_status != current_status) || (count == 10) ) {
    //add re-centre command that resets origin?
    Serial1.print("100,");
    Serial1.print(String(-total_x));
    Serial1.print(","); // is comma necessary for parsing on ESP?
    Serial1.print(String(total_y));
    Serial1.print(",");
    Serial1.println(String(current_status_encoded));
    //Serial1.print(",");
    //Serial1.print("-1000,");
    //Serial1.println("-1000");

    prev_total_x = total_x;
    prev_total_y = total_y;
    prev_status = current_status;

    //Serial.print('\n');

    //Serial.println("Distance_x = " + String(total_x));

    //Serial.println("Distance_y = " + String(total_y));
    //Serial.print('\n');

    delay(100);
  }

  //*******************************************************************//

}

// Timer A CMP1 interrupt. Every 800us the program enters this interrupt.
// This, clears the incoming interrupt flag and triggers the main loop.

ISR(TCA0_CMP1_vect) {
  TCA0.SINGLE.INTFLAGS |= TCA_SINGLE_CMP1_bm; //clear interrupt flag
  loopTrigger = 1;
}
