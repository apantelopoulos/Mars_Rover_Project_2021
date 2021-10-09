#include <Wire.h>
#include <INA219_WE.h>
#include <SPI.h>
#include <SD.h>

INA219_WE ina219; // this is the instantiation of the library for the current sensor

// set up variables using the SD utility library functions:
Sd2Card card;
SdVolume volume;
SdFile root;
float open_loop, closed_loop;
const int chipSelect = 10;
unsigned int rest_timer;
unsigned int rest_timer2;
unsigned int loop_trigger;
unsigned int int_count; // a variables to count the interrupts. Used for program debugging.

float iL, current_ref = 0, error_amps; // Current Control
float ev=0,cv=0,ei=0,oc=0; //internal signals
float Ts=0.0008; //1.25 kHz control frequency. It's better to design the control period as integral multiple of switching period.
float kpv=1,kiv=0.2,kdv=0; // voltage pid.
float u0v,u1v,delta_uv,e0v,e1v,e2v; // u->output; e->error; 0->this time; 1->last time; 2->last last time
float kpi=1,kii=0.2,kdi=0; // current pid.
float u0i,u1i,delta_ui,e0i,e1i,e2i; // Internal values for the current controller
float uv_max=5, uv_min=4.9; //anti-windup limitation
float ui_max=1, ui_min=0; //anti-windup limitation
float pwm_out;
float vb,vref,vpd;
boolean input_switch;
int state_num=0,next_state;
String dataString;
float current_limit;
float PercentageCharge;
float measA,measB,measC,measD;
float Amax = measA + 0.01;
float Bmax = measB + 0.01;
float Cmax = measC + 0.01;
float Dmax = measD + 0.01;
float Amin = measA - 0.01;
float Bmin = measB - 0.01;
float Cmin = measC - 0.01;
float Dmin = measD - 0.01;
float minv,maxv;

bool Overlapping;
void setup() {
  //Some General Setup Stuff

  Wire.begin(); // We need this for the i2c comms for the current sensor
  Wire.setClock(700000); // set the comms speed for i2c
  ina219.init(); // this initiates the current sensor
  Serial.begin(9600); // USB Communications


  //Check for the SD Card
  Serial.println("\nInitializing SD card...");
  if (!SD.begin(chipSelect)) {
    Serial.println("* is a card inserted?");
    while (true) {} //It will stick here FOREVER if no SD is in on boot
  } else {
    Serial.println("Wiring is correct and a card is present.");
  }

  if (SD.exists("BatCycle.csv")) { // Wipe the datalog when starting
    SD.remove("BatCycle.csv");
  }

  
  noInterrupts(); //disable all interrupts
  analogReference(EXTERNAL); // We are using an external analogue reference for the ADC

  //SMPS Pins
 
  pinMode(13, OUTPUT); // Using the LED on Pin D13 to indicate status
  pinMode(2, INPUT_PULLUP); // Pin 2 is the input from the CL/OL switch
  pinMode(6, OUTPUT); // This is the PWM Pin

 
  pinMode(7, OUTPUT); //bat A relay
  pinMode(8, OUTPUT); //bat B relay
  pinMode(5, OUTPUT); //bat C relay
  pinMode(4, OUTPUT); //bat D relay
  
  //Analogue input, the battery voltage (also port B voltage)
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
  pinMode(A6, INPUT);
  pinMode(A7, INPUT);
  //Analogue input, the battery voltage (also port B voltage)


  // TimerA0 initialization for 1kHz control-loop interrupt.
  TCA0.SINGLE.PER = 999; //
  TCA0.SINGLE.CMP1 = 999; //
  TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV16_gc | TCA_SINGLE_ENABLE_bm; //16 prescaler, 1M.
  TCA0.SINGLE.INTCTRL = TCA_SINGLE_CMP1_bm;

  // TimerB0 initialization for PWM output
  TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc | TCB_ENABLE_bm; //62.5kHz

  interrupts();  //enable interrupts.
  analogWrite(6, 120); //just a default state to start with
}
 void loop() {
  if(loop_trigger == 1) { // This loop is triggered, it wont run unless there is an interrupt
     vpd   = analogRead(A6)*(4.096 / 1023.0)*2; //times two because a potential divider is used to measure
     vref = analogRead(A7)*(4.096 / 1023.0)*2;
     iL = ina219.getCurrent_mA(); // sample the inductor current (via the sensor chip)

    digitalWrite(13, HIGH);   // set pin 13. Pin13 shows the time consumed by each control cycle. It's used for debugging.
    SoC();
    // Sample all of the measurements and check which control mode we are in
   
          current_limit = 0.250; // Buck has a higher current limit
          ev = vref - vpd;  //voltage error at this time
          cv=pidv(ev);  //voltage pid
          cv=saturation(cv, current_limit, 0); //current demand saturation
          ei=cv-iL; //current error
          closed_loop=pidi(ei);  //current pid
          closed_loop=saturation(closed_loop,0.99,0.01);  //duty_cycle saturation
          pwm_modulate(closed_loop); //pwm modulation

    digitalWrite(13, LOW);   // reset pin13.
    loop_trigger = 0;
    input_switch = digitalRead(2); //get the OL/CL switch status
  
 
  if(int_count == 1000)
  {
    switch (state_num){
       case 0:{
       if (input_switch == 1) { // if switch, move to charge, this is currently 
          //turn all the relays on
          next_state = 1;
          Serial.println("swutch is on") ;                  
        }if(PercentageCharge == 0)
        {
          next_state = 0;
           Serial.println("0 charge %") ;
        }
       
        else { // otherwise stay put
          next_state = 0;
          Serial.println("swutch is off") ; 
        }
        break;
      }
     case 1:
     {
       Serial.println("state 1");
      
       
       digitalWrite(7,true); 
       digitalWrite(8,true);
       digitalWrite(5,true);
       digitalWrite(4,true);
       measA = analogRead(A0)*4.096/1.03;
       measB = analogRead(A1)*4.096/1.03;
       measC = analogRead(A2)*4.096/1.03;
       measD = analogRead(A3)*4.096/1.03;
       Serial.println(measA);
       Serial.println(measB);
       balanced();
       
       if(Overlapping) // are they balanced?
       {
         next_state = 2;
         Serial.println("Balanced");
         digitalWrite(7,false); 
         digitalWrite(8,false);
         digitalWrite(5,false);
         digitalWrite(4,false);
    
       }
       else
      {
        next_state = 3;
        Serial.println("not Balanced");
      }
       break;
     }
      
  
      case 2:
      { 
        Serial.println("state 2");
        
        Serial.println(rest_timer);
        
       if(15 > rest_timer)
        { 
          next_state = 2;
          vref = 5; 
          rest_timer ++;
        }
        else{
          rest_timer = 0;
          next_state = 1;
        }
        break;        
      }
      case 3:   // in this state we wait for the parallel cells to active balance eachother.
      {
        Serial.println("state 3");
        vref = 0;
       
         if (rest_timer2 < 30) 
         {
            rest_timer2++;
          }
         else
         {
            rest_timer2 = 0;
            next_state = 0;
         }
        break;
      }
     

      default :{ // Should not end up here ....
        Serial.println("Boop");
        current_ref = 0;
        next_state = 0; // So if we are here, we go to error
      
      }
      
    }
    
    dataString = String(state_num) + "," + String(vb) ; //build a datastring for the CSV file
    Serial.println(dataString); // send it to serial as well in case a computer is connected
    File dataFile = SD.open("BatCycle.csv", FILE_WRITE); // open our CSV file
    if (dataFile){ //If we succeeded (usually this fails if the SD card is out)
      dataFile.println(dataString); // print the data
    } else {
      Serial.println("File not open"); //otherwise print an error
    }
    dataFile.close(); // close the file
    int_count = 0; // reset the interrupt count so we dont come back here for 1000ms
  }

    }
 }


// Timer A CMP1 interrupt. Every 800us the program enters this interrupt. 
// This, clears the incoming interrupt flag and triggers the main loop.

ISR(TCA0_CMP1_vect){
  TCA0.SINGLE.INTFLAGS |= TCA_SINGLE_CMP1_bm; //clear interrupt flag
  loop_trigger = 1;
}

// This subroutine processes all of the analogue samples, creating the required values for the main loop



float saturation( float sat_input, float uplim, float lowlim){ // Saturatio function
  if (sat_input > uplim) sat_input=uplim;
  else if (sat_input < lowlim ) sat_input=lowlim;
  else;
  return sat_input;
}

void pwm_modulate(float pwm_input){ // PWM function
  analogWrite(6,(int)(255-pwm_input*255)); 
}

// This is a PID controller for the voltage

float pidv( float pid_input){
  float e_integration;
  e0v = pid_input;
  e_integration = e0v;
 
  //anti-windup, if last-time pid output reaches the limitation, this time there won't be any intergrations.
  if(u1v >= uv_max) {
    e_integration = 0;
  } else if (u1v <= uv_min) {
    e_integration = 0;
  }

  delta_uv = kpv*(e0v-e1v) + kiv*Ts*e_integration + kdv/Ts*(e0v-2*e1v+e2v); //incremental PID programming avoids integrations.there is another PID program called positional PID.
  u0v = u1v + delta_uv;  //this time's control output

  //output limitation
  saturation(u0v,uv_max,uv_min);
  
  u1v = u0v; //update last time's control output
  e2v = e1v; //update last last time's error
  e1v = e0v; // update last time's error
  return u0v;
}

// This is a PID controller for the current

float pidi(float pid_input){
  float e_integration;
  e0i = pid_input;
  e_integration=e0i;
  
  //anti-windup
  if(u1i >= ui_max){
    e_integration = 0;
  } else if (u1i <= ui_min) {
    e_integration = 0;
  }
  
  delta_ui = kpi*(e0i-e1i) + kii*Ts*e_integration + kdi/Ts*(e0i-2*e1i+e2i); //incremental PID programming avoids integrations.
  u0i = u1i + delta_ui;  //this time's control output

  //output limitation
  saturation(u0i,ui_max,ui_min);
  
  u1i = u0i; //update last time's control output
  e2i = e1i; //update last last time's error
  e1i = e0i; // update last time's error
  return u0i;
}
void SoC()
{
  minv = 2600;
  PercentageCharge = ((vb-minv)/600)*100; // 600 is the difference between 3200 and 2600
}
void balanced()
{
  
  Overlapping = (Amax > Bmin || Bmax > Amin) && (Amax > Cmin || Cmax > Amin) && (Amax > Dmin || Dmax > Amin)                && (Bmax > Cmin || Cmax > Bmin) && (Bmax > Dmin || Dmax > Bmin) && (Dmax > Cmin || Cmax > Dmin);

}


/*end of the program.*/
