#include <Metro.h> // Include the Metro library

//Define all the pins
const int SwPin_DivEuc = 0;
const int SwPin_OutModes = 1;
const int SwPin_CVup = 2;
const int SwPin_CVdown = 3;
const int SwPin_Encoder = 6;
const int CtrPin_CVamt = 21;
const int CtrPin_Divisions = 20;
const int CtrPin_Length = 19;
const int EncPinA = 4;
const int EncPinB = 5;
const int OutPin_Cycle = 7;
const int OutPin_MainOut = 8;
const int OutPin_Thru = 9;
const int InPin_Trig = 18; //Low input voltage = HIGH reading on pin.
const int InPin_CV = 17;
const int LEDPin_Shuffle = 16;
const int LEDPin_internal = 13;

//recentPresses[] stores the times of the 6 most recent switch presses
//[0] = most recent, [5] = most distant
// every time we write to this, shuffle the array around
double recentClockTimes[6] = {0,0,0,0,0,0}; 
const int recentClockTimesArraySize = 6;

const double ButtonPressTimeout = 5000000; //in microseconds. if the time between two button presses is greater than this, do not use it to set the output clock. 

Metro ClockOutput1 = Metro(1000); //sets up a regular event for clock pulses
Metro ClockOutput2 = Metro(1000); //sets up a regular event for clock pulses
Metro ClockOutput3 = Metro(1000); //sets up a regular event for clock pulses


double pressTimeTemp = 0; //temporarily stores the time, when the button is pressed
int clockInPrevState = 0; // this will remember if the clock input was "on" in the previous cycle 

//multiplication/division factors. These will eventually be controlled by pots.
//float ClicksPerCycle = 8;
float MultDivFactor2 = 2;
float MultDivFactor3 = 0.5;



void setup() {
  // put your setup code here, to run once:
  
  //Configure pins
  pinMode(SwPin_DivEuc,INPUT_PULLUP);
  pinMode(SwPin_OutModes,INPUT_PULLUP);
  pinMode(SwPin_CVup,INPUT_PULLUP);
  pinMode(SwPin_CVdown,INPUT_PULLUP);
  pinMode(SwPin_Encoder,INPUT);

  pinMode(EncPinA,INPUT_PULLUP);
  pinMode(EncPinB,INPUT_PULLUP);
  
  pinMode(CtrPin_CVamt,INPUT);
  pinMode(CtrPin_Divisions,INPUT);
  pinMode(CtrPin_Length,INPUT);

  pinMode(OutPin_Cycle,OUTPUT);
  pinMode(OutPin_MainOut,OUTPUT);
  pinMode(OutPin_Thru,OUTPUT);
  pinMode(LEDPin_Shuffle,OUTPUT);
  pinMode(LEDPin_internal,OUTPUT);

  pinMode(InPin_Trig,INPUT_PULLUP);
  pinMode(InPin_CV,INPUT);

  Serial.begin(9600);

  ClockOutput1.reset(); //start the output clock
  ClockOutput2.reset(); //start the output clock
  ClockOutput3.reset(); //start the output clock

}

void loop() {
  // put your main code here, to run repeatedly:

  // deal with input
  if(digitalRead(InPin_Trig) == 0 && clockInPrevState == 1){      //............if the Trigger In voltage switches from low to high

      //........insert the time in the array of press times, rotating the array to maintain ordering from most to least recent
      for(int i=(recentClockTimesArraySize-1); i>0; i--){
        recentClockTimes[i] = recentClockTimes[i-1];
        }
      recentClockTimes[0] =  micros();
      }

      clockInPrevState = digitalRead(InPin_Trig); //save the switch value for reference on the next round
      
      //....print the value of the recent-presses array
      for(int i=0; i<recentClockTimesArraySize; i++){
        Serial.print(recentClockTimes[i]);
      }
      Serial.println();

      //If there have been 2 switch pulses recently, use the timing to set the frequence of clock pulse 1
      if(recentClockTimes[1]>0 && recentClockTimes[0]-recentClockTimes[1]<ButtonPressTimeout){
        ClockOutput1.interval( (recentClockTimes[0]-recentClockTimes[1]) / 1000);
        ClockOutput2.interval( (recentClockTimes[0]-recentClockTimes[1])/ (MultDivFactor2 * 1000));
        ClockOutput3.interval( (recentClockTimes[0]-recentClockTimes[1])/ (MultDivFactor3 * 1000));
        //ClockOutput1.reset();
        //ClockOutput2.reset();
        //ClockOutput3.reset();   
      }


  // deal with output
  
  if(ClockOutput1.check()){           //...if it's time to make a clock pulse
        digitalWrite(OutPin_Thru,HIGH);    //generate the pulse
        ClockOutput1.reset();         //reset the timer
  }
  else{
    digitalWrite(OutPin_Thru,LOW);
  }
  
  if(ClockOutput2.check()){           //...if it's time to make a clock pulse
        digitalWrite(OutPin_Cycle,HIGH);    //generate the pulse
        ClockOutput2.reset();         //reset the timer

        }
  else{
    digitalWrite(OutPin_Cycle,LOW);
  }

    if(ClockOutput3.check()){           //...if it's time to make a clock pulse
        digitalWrite(OutPin_MainOut,HIGH);    //generate the pulse
        ClockOutput3.reset();         //reset the timer
         ClockOutput2.reset(); 
        ClockOutput1.reset(); 
  }
  else{
    digitalWrite(OutPin_MainOut,LOW);
  }


}
