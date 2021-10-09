#include <Wire.h>
#include <INA219_WE.h>
#include <SPI.h>
#include <SD.h>

INA219_WE ina219; // this is the instantiation of the library for the current sensor

// set up variables using the SD utility library functions:
Sd2Card card;
SdVolume volume;
SdFile root;

const int chipSelect = 10;
unsigned int rest_timer,rest_timer2;
unsigned int loop_trigger;
unsigned int int_count; // a variables to count the interrupts. Used for program debugging.
float u0i, u1i, delta_ui, e0i, e1i, e2i; // Internal values for the current controller
float ui_max = 1, ui_min = 0; //anti-windup limitation
float kpi = 0.02512, kii = 39.4, kdi = 0; // current pid.
float Ts = 0.001; //1 kHz control frequency.
float current_measure, current_ref = 0, error_amps; // Current Control
float pwm_out;
float V_B;
boolean input_switch;
int state_num=0,next_state;
String dataString;
float measA,measB,measC,measD;

float Power;
float maxPower = 0;
float maxCrrentRef;
float S;
float ActivateMPPT = 0;
bool Overlapping;
float Amax = measA + 0.01;
float Bmax = measB + 0.01;
float Cmax = measC + 0.01;
float Dmax = measD + 0.01;
float Amin = measA - 0.01;
float Bmin = measB - 0.01;
float Cmin = measC - 0.01;
float Dmin = measD - 0.01;
float minv,maxv,PercentageCharge;
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
 
  if (loop_trigger == 1){ // FAST LOOP (1kHZ)
      state_num = next_state; //state transition
      V_B = analogRead(A6)*4.096/1.03; //check the battery voltage (1.03 is a correction for measurement error, you need to check this works for you)
      SoC();
      current_measure = (ina219.getCurrent_mA()); // sample the inductor current (via the sensor chip)
      error_amps = (current_ref - current_measure) / 1000; //PID error calculation
      pwm_out = pidi(error_amps); //Perform the PID controller calculation
      pwm_out = saturation(pwm_out, 0.99, 0.01); //duty_cycle saturation
      analogWrite(6, (int)(255 - pwm_out * 255)); // write it out (inverting for the Buck here)
      int_count++; //count how many interrupts since this was last reset to zero
      
      loop_trigger = 0; //reset the trigger and move on with life
  
  }
  
  if (int_count == 1000) { // SLOW LOOP (1Hz)
   Serial.println(PercentageCharge) ; //this provides the charge percentage to the server every second
    int_count = 0;
    input_switch = digitalRead(2); //get the OL/CL switch status 
    switch (state_num) { // STATE MACHINE (see diagram)
     
      case 0:
      { 
        Serial.println("state 0");
        current_ref = 0;
        if (input_switch == 1) { // if switch, move to charge, this is currently 
          //turn all the relays on
          next_state = 1;
          Serial.println("swutch is on") ;                  
        }if(PercentageCharge == 0)
        {
          next_state = 0;
           Serial.println("0 charge %") ;
        }
        if(PercentageCharge > 99)
        {
          next_state = 0;
           Serial.println("full charge!") ;
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
       current_ref = 0;
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
       Serial.println(measC);
       Serial.println(measD);
       balanced();
       if((measA>3200)&(measB>3200)&(measC>3200)&(measD>3200))
       {
        next_state = 0; //finished
       }
       if((measA>3200)||(measB>3200)||(measC>3200)||(measD>3200))
       {
        next_state = 3; 
       }
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
      { //MPPT stage
        Serial.println("state 2");
        Serial.println(current_measure);
        Serial.println(rest_timer);
        if (rest_timer <= 3 ) 
        {
          MPPT();
          Serial.println("MPPT NOW");
          Serial.println(maxPower /1000000);
          rest_timer++;
        } 
        if (3 < rest_timer <15) 
        {
          
          current_ref = maxCrrentRef; 
          rest_timer++;
          next_state = 2;
        } 
       if(15 < rest_timer)
        { 
          next_state = 1;
          rest_timer = 0;
         
        }
        break;        
      }
      case 3:
      {
        Serial.println("state 3");
        current_ref = maxCrrentRef;; 
        FindTheSmallest();
        MPPT();
       if (rest_timer2 < 20) 
       {
        if(S == measA)
        {
         digitalWrite(7,false); 
         digitalWrite(8,true);
         digitalWrite(5,true);
         digitalWrite(4,true);  
         rest_timer2++;
                  
        }
        if(S == measB)
        {
         digitalWrite(7,true); 
         digitalWrite(8,false);
         digitalWrite(5,true);
         digitalWrite(4,true);
         rest_timer2++;          
        }
          if(S == measC)
          {
           digitalWrite(7,true); 
           digitalWrite(8,true);  
           digitalWrite(5,false);
           digitalWrite(4,true);          
          }
          if(S == measD)
          {
           digitalWrite(7,true); 
           digitalWrite(8,true); 
           digitalWrite(5,true);
           digitalWrite(4,false);          
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
    
    dataString = String(state_num) + "," + String(V_B) + "," + String(current_ref) + "," + String(current_measure); //build a datastring for the CSV file
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
// Timer A CMP1 interrupt. Every 1000us the program enters this interrupt. This is the fast 1kHz loop
ISR(TCA0_CMP1_vect) {
  loop_trigger = 1; //trigger the loop when we are back in normal flow
  TCA0.SINGLE.INTFLAGS |= TCA_SINGLE_CMP1_bm; //clear interrupt flag
}

float saturation( float sat_input, float uplim, float lowlim) { // Saturation function
  if (sat_input > uplim) sat_input = uplim;
  else if (sat_input < lowlim ) sat_input = lowlim;
  else;
  return sat_input;
}

float pidi(float pid_input) { // discrete PID function
  float e_integration;
  e0i = pid_input;
  e_integration = e0i;

  //anti-windup
  if (u1i >= ui_max) {
    e_integration = 0;
  } else if (u1i <= ui_min) {
    e_integration = 0;
  }

  delta_ui = kpi * (e0i - e1i) + kii * Ts * e_integration + kdi / Ts * (e0i - 2 * e1i + e2i); //incremental PID programming avoids integrations.
  u0i = u1i + delta_ui;  //this time's control output

  //output limitation
  saturation(u0i, ui_max, ui_min);

  u1i = u0i; //update last time's control output
  e2i = e1i; //update last last time's error
  e1i = e0i; // update last time's error
  return u0i;
}
void FindTheSmallest()
{
 S = min(min(measA, measB), min(measC, measD));
}

void MPPT()
{

  
    for (int i = 0 ; i < 250; i = i+10) // here we create an array of the powers produced at all duty cycle
    {
      current_ref = i;
      Power = V_B * current_measure;
      if (Power > maxPower)
      {
        maxPower = Power;
        maxCrrentRef = i;       
      }
    
  }
}
void SoC()
{
  minv = 2600;
  PercentageCharge = ((V_B-minv)/600)*100; // 600 is the difference between 3200 and 2600
}
void balanced()
{
  
  Overlapping = (Amax > Bmin || Bmax > Amin) && (Amax > Cmin || Cmax > Amin) && (Amax > Dmin || Dmax > Amin)                && (Bmax > Cmin || Cmax > Bmin) && (Bmax > Dmin || Dmax > Bmin) && (Dmax > Cmin || Cmax > Dmin);

}
