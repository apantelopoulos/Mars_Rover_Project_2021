

void sampling(){
  
  current_mA = ina219.getCurrent_mA(); // sample the inductor current (via the sensor chip)
  iL = current_mA/1000.0;
  
}

float saturation( float sat_input, float uplim, float lowlim){ // Saturatio function
  if (sat_input > uplim) sat_input=uplim;
  else if (sat_input < lowlim ) sat_input=lowlim;
  else;
  return sat_input;
}

void pwm_modulate(float pwm_input){ // PWM function
  analogWrite(6,(int)(255-pwm_input*255)); 
}
