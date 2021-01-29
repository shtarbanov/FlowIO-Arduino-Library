/*This is the hardware testng firmware used for ensuring that the device works as expected.
 * Open your serial monitor to read the instructions for how to run each test, and the 
 * outcomes and results. You need to have access to the UserSW button for this test. 
 * Follow the instructions on the Serial Monitor.
 */

 //TODO: Add a battery test. If battery is connected, voltage will drop on the vbat pin when valves are opened.
 //TODO: Add a blockage test. Open both side valves and the pressure should not increase past a max value.
#include <FlowIO.h>
#define btnPin 7
#define batteryPin A6


FlowIO flowio;
int mode = 0;
int prevMode = -1; //making this different so we enter the state machine on first start.

bool buttonState = 0;         // current state of the button
bool prevButtonState = 0;     // previous state of the button
float p0 = 0;
float pinf=0;
float pvac=0;

void setup(){
  flowio = FlowIO(GENERAL); //This must be done the very first item to minimize the click on startup (Bug #44 on Github)  
  //NOTE: You cannot do the object initialization together with the declaration, because it will cause the 
  //hardware to not work fine, even though it will compile fine.
  Serial.begin(115200);
  while(!Serial) delay(10);
  pinMode(btnPin,INPUT_PULLUP); 
  Serial.println("\n### --FlowIO Self Test Initialized - ###\n");
  flowio.blueLED(HIGH);
}

void loop() {
  if(mode != prevMode){ //Only execute this code if the mode has changed.
    switch(mode){
      case 0: //we come here when we first start the system
        flowio.blueLED(1);
        sensorTest();
        Serial.println("------\nPress UserSW to begin 1: 'Valve Click test'");
        Serial.println(" - each valve will turn on and off sequentially. You should count 14 clicks.");
        break;
      case 1:
        flowio.blueLED(1);
        delay(500); //delay to allow enough time to release button
        valveClickTest();
        Serial.println("Valve Click Test complete.");
        Serial.println("------\nPress UserSW to begin 2: 'Inflation Pump Test'");
        Serial.println(" - Run inflation pump for 100ms. Internal pressure should increase.");
        break;
      case 2:
        flowio.blueLED(1);
        delay(500); //delay to allow enough time to release button
        inflationPumpTest();
        Serial.println("Inflation Pump Test complete.");
        Serial.println("------\nPress UserSW button to start 3: 'Vacuum Pump Test'");
        Serial.println(" - Run vacuum pump for 100ms. Internal pressure should decrease");
        break;
      case 3:
        flowio.blueLED(1);
        delay(500); //delay to allow enough time to release button
        vacuumPumpTest();
        Serial.println("Vacuum Pump Test complete.");
        Serial.println("------\nPress UserSW button to start 4: 'Battery Connection Test'");
        Serial.println(" - Compares battery voltage with valves off and on and looks for drop");
        break;
    }
    prevMode = mode;
  }
  buttonState = digitalRead(btnPin);
  if(buttonState != prevButtonState){ //if buttonstate has changed.
    if(buttonState == LOW)  //and if it is now pressed.
      mode += 1;
      if(mode>4) mode=0;
      delay(50); //debounce
  }
  prevButtonState = buttonState;  
}
void batteryConnectionTest(){
  flowio.pixel(1,1,1);
  Serial.println("Battery Connection Test in Progress...");
  pinMode(batteryPin,INPUT);
  int vbat0 = analogRead(batteryPin);
  flowio.openInletValve();
  flowio.openOutletValve();
  delay(100);
  int vbat1 = analogRead(batteryPin);
  flowio.closeInletValve();
  flowio.closeOutletValve();
  if(vbat1<vbat){
    Serial.println("Success");
  }
  else{
    Serial.println("Fail");
  }
}
void valveClickTest(){
  flowio.pixel(1,1,1);
  Serial.print("Valve Click Test in Progress....");
  Serial.print("in..");
  flowio.openInletValve();
  delay(500);
  flowio.closeInletValve();
  delay(1000);
  Serial.print("out..");
  flowio.openOutletValve();
  delay(500);
  flowio.closeOutletValve();
  delay(1000);
  Serial.print("p1..");
  flowio.setPorts(0b00000001);
  delay(500);
  flowio.setPorts(0b00000000);
  delay(1000);
  Serial.print("p2..");
  flowio.setPorts(0b00000010);
  delay(500);
  flowio.setPorts(0b00000000);
  delay(1000);
  Serial.print("p3..");
  flowio.setPorts(0b00000100);
  delay(500);
  flowio.setPorts(0b00000000);
  delay(1000);
  Serial.print("p4..");
  flowio.setPorts(0b00001000);
  delay(500);
  flowio.setPorts(0b00000000);
  delay(1000);
  Serial.println("p5.");
  flowio.setPorts(0b00010000);
  delay(500);
  flowio.setPorts(0b00000000);
  delay(1000);  
  flowio.pixel(0,0,0);
}
void sensorTest(){
  flowio.pixel(1,1,1);
  //To get the correct pressure reading, we will open and close two ports first
  flowio.openPorts(0b00000011);
  delay(100);
  flowio.stopAction(0xFF);
  Serial.println("Sensor Test in Progress....");
  flowio.activateSensor();
  if(flowio.readError()){
    Serial.print("Detecting Sensor......FAILED. #$%@# Error ");
    Serial.println(flowio.readError());
    flowio.pixel(1,0,0);
  }
  else{
    Serial.print("Detecting Sensor......Success! :) ");
    Serial.print("P = ");
    p0=flowio.getPressure(PSI);
    Serial.println(p0);
    flowio.pixel(0,1,0);
  }
}
void inflationPumpTest(){
  flowio.pixel(1,1,1);
  Serial.print("Inflation Pump Test in Progress....");
  flowio.openInletValve();
  flowio.startPump(1);
  delay(100);
  Serial.print("P = ");
  pinf=flowio.getPressure(PSI);
  Serial.print(pinf);
  if(pinf<=(p0+0.4)){
    Serial.println("...FAILED"); //pressure should be at least 0.4psi higher.
    flowio.pixel(1,0,0);
  }
  else{
    Serial.println("...Success! :)");
    flowio.pixel(0,1,0);
  }
  flowio.stopPump(1);
  //release the pressure and close the valves:
  flowio.openOutletValve();
  delay(300);
  flowio.closeOutletValve();
  flowio.closeInletValve();
}
void vacuumPumpTest(){
  flowio.pixel(1,1,1);
  Serial.print("Vacuum Pump Test in Progress....");
  flowio.openOutletValve();
  flowio.startPump(2);
  delay(100);
  Serial.print("P = ");
  pvac=flowio.getPressure(PSI);
  Serial.print(pvac);
  if(pvac>=(p0-0.4)){
    Serial.println("...FAILED"); //pressure should be at least 0.4psi lower.
    flowio.pixel(1,0,0);
  }
  else{
    Serial.println("...Success! :)");
    flowio.pixel(0,1,0);
  }
  flowio.stopPump(2);
  //release the pressure and close the valves
  flowio.openInletValve();
  delay(300);
  flowio.closeOutletValve();
  flowio.closeInletValve();
}