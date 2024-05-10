int switch_pwm_A = 0;
int switch_pwm_B = 0;

void switchPinInit(){
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(PWMA, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
  pinMode(PWMB, OUTPUT);

  ledcSetup(channel_A, freq, ANALOG_WRITE_BITS);
  ledcAttachPin(PWMA, channel_A);

  ledcSetup(channel_B, freq, ANALOG_WRITE_BITS);
  ledcAttachPin(PWMB, channel_B);

  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, LOW);
}


void switchEmergencyStop(){
  switch_pwm_A = 0;
  switch_pwm_B = 0;
  
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, LOW);

  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, LOW);
}


void switchPortCtrlA(float pwmInputA){
  int pwmIntA = round(pwmInputA);
  if(abs(pwmIntA) < 1e-6){
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, LOW);
    return;
  }

  if(pwmIntA > 0){
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, HIGH);
    ledcWrite(channel_A, pwmIntA);
  }
  else{
    digitalWrite(AIN1, HIGH);
    digitalWrite(AIN2, LOW);
    ledcWrite(channel_A,-pwmIntA);
  }
}


void switchPortCtrlB(float pwmInputB){
  int pwmIntB = round(pwmInputB);
  if(abs(pwmIntB) < 1e-6){
    digitalWrite(BIN1, LOW);
    digitalWrite(BIN2, LOW);
    return;
  }

  if(pwmIntB > 0){
    digitalWrite(BIN1, LOW);
    digitalWrite(BIN2, HIGH);
    ledcWrite(channel_B, pwmIntB);
  }
  else{
    digitalWrite(BIN1, HIGH);
    digitalWrite(BIN2, LOW);
    ledcWrite(channel_B,-pwmIntB);
  }
}


void switchCtrl(int pwmIntA, int pwmIntB) {
    switch_pwm_A = pwmIntA;
    switch_pwm_B = pwmIntB;
    switchPortCtrlA(switch_pwm_A);
    switchPortCtrlB(switch_pwm_B);
}


void lightCtrl(int pwmIn) {
  switch_pwm_A = pwmIn;
  switchPortCtrlA(-abs(switch_pwm_A));
}