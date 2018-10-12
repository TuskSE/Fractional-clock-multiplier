#include <Metro.h> // Include the Metro library

//pins for input (clock) and output (inbuilt LED)
const int ledPin = 13;
const int switchPin = 17;

//recentPresses[] stores the times of the 6 most recent switch presses
//[0] = most recent, [5] = most distant
// every time we write to this, shuffle the array around
double recentPresses[6] = {0,0,0,0,0,0}; 
const int recentPressesArraySize = 6;

const double ButtonPressTimeout = 5000000; //in microseconds. if the time between two button presses is greater than this, do not use it to set the output clock. 

Metro ClockOutput1 = Metro(1000); //sets up a regular event for clock pulses


double pressTimeTemp = 0; //temporarily stores the time, when the button is pressed
int buttonState = 0; // this will remember if the button was pressed in the previous cycle 




void setup() {
  // put your setup code here, to run once:
  pinMode(ledPin, OUTPUT);
  pinMode(switchPin, INPUT);

  Serial.begin(9600);

  ClockOutput1.reset(); //start the output clock

}

void loop() {
  // put your main code here, to run repeatedly:



  // deal with input
  if(digitalRead(switchPin) == 1 && buttonState == 0){      //............if the switch turns from off to on

      //........insert the time in the array of press times, rotating the array to maintain ordering from most to least recent
      for(int i=(recentPressesArraySize-1); i>0; i--){
        recentPresses[i] = recentPresses[i-1];
        }
      recentPresses[0] =  micros();
      }

      buttonState = digitalRead(switchPin); //save the switch value for reference on the next round
      
      //....print the value of the recent-presses array
      for(int i=0; i<recentPressesArraySize; i++){
        Serial.print(recentPresses[i]);
      }
      Serial.println();

      //If there have been 2 switch pulses recently, use the timing to set the frequence of clock pulse 1
      if(recentPresses[1]>0 && recentPresses[0]-recentPresses[1]<ButtonPressTimeout){
        ClockOutput1.interval( (recentPresses[0]-recentPresses[1]) / 1000);
      }

  // deal with output
  if(ClockOutput1.check()){           //...if it's time to make a clock pulse
        digitalWrite(ledPin,HIGH);    //generate the pulse
        ClockOutput1.reset();         //reset the timer
  }
  else{
    digitalWrite(ledPin,LOW);
  }


}
