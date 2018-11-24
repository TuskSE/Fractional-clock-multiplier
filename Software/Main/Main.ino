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

//sets the range for the Length and Division knobs
const int Divisions_min = 1;
const int Divisions_max = 16;
const int Length_min = 1;
const int Length_max = 16;
//To read Knob value: reading = map(analogRead(CtrPin_Length),0,1023,Length_min,Length_max);

Metro ClockOutput1 = Metro(1000); //sets up a regular event for clock pulses
Metro ClockOutput2 = Metro(1000); //sets up a regular event for clock pulses
Metro ClockOutput3 = Metro(1000); //sets up a regular event for clock pulses

double pressTimeTemp = 0; //temporarily stores the time, when the button is pressed
int clockInState = 0;
int clockInPrevState = 0; // this will remember if the clock input was "on" in the previous cycle 

//multiplication/division factors. These will eventually be controlled by pots.
//float ClicksPerCycle = 8;
float MultDivFactor2 = 2;
float MultDivFactor3 = 0.5;

//-------------------------------------------------------------------------------------------------------------------

//The PulsePredictor class is responsible for 
// + logging input pulses    + predicting when future pulses will arise
// + knowing whether it is giving good predictions or not
// + relaying pulse events to other bits of code
// + relaying, e.g., cycle times to other bits of code

const int recentPulseTimesArraySize = 10;
const int predictedFuturePulseTimesArraySize = 16;

class PulsePredictor {
  unsigned long int recentPulseTimes[recentPulseTimesArraySize];    //remembers the absolute time of the previous 10 pulses
  unsigned long int predictedFuturePulseTimes[16]; //predicts the absolute time of the next 16 pulses
  bool TrustworthyPulsePredictions; // records whether the incoming clock is predictable
  bool PulseRecieved; //notes when a pulse has been recieved
  bool TransmitPulse; //notes when a pulse should be transmitted to downstream code segments
  bool PulsesShouldBeTransmitted; //notes whether pulses ought to be transmitted

  public:
  PulsePredictor (); //sets default initialization values  
  void InputPulse (); //engaging this tells the Pulse Predictor that a Pulse has arrived
  unsigned long int TimeOfNthPulse (int n); //returns the estimated time of future pulses
  unsigned long int PulseInterval (int n1, int n2); //returns the expected time between future pulses
  bool IsThereAPulse ();
  void RecalculatePredictions ();
} InputPulsePredictor;

//sets the default initialization of the PulsePredictor
PulsePredictor::PulsePredictor () {   
  TrustworthyPulsePredictions = false;
  PulseRecieved = false;
  TransmitPulse = false;
  PulsesShouldBeTransmitted = true;
}

void PulsePredictor::InputPulse (){
     //Log the time of the input pulse
     
     //........insert the time in the array of press times, rotating the array to maintain ordering from most to least recent
      for(int i=(recentPulseTimesArraySize-1); i>0; i--){
        recentPulseTimes[i] = recentPulseTimes[i-1];
        }
      recentPulseTimes[0] =  millis();

      //Flag the fact that a pulse has been recieved
      PulseRecieved = true;

      //Flag the the pulse should be transmitted
      if (PulsesShouldBeTransmitted == true){
        TransmitPulse = true;
      }
}


void PulsePredictor::RecalculatePredictions (){
  //Measure the average of the time between the last two pulses. In future, this can be more sophisticated.
  unsigned long int PulseInterval = (recentPulseTimes[0]-recentPulseTimes[2])/2;

  //Use this to predict the times of future pulses
  for(int i=0; i<predictedFuturePulseTimesArraySize; i++){
    predictedFuturePulseTimes[i] = recentPulseTimes[0]+(i+1)*PulseInterval;
  }
}

bool PulsePredictor::IsThereAPulse() {
  if ( TransmitPulse == true ) {
    TransmitPulse = false; //assume the pulse has now been transmitted - job done!
    return true;
  } else {
    return false;
  }
  
}

//----------------------------------------------------------------------------------------------------------------------

// the TrigOutManager is called on to generate output pulses, and handles e.g. the timing of said pulses.

class TrigOutManager {
  unsigned long TrigLengthMicros, CurrentPulseEndtimeMicros; //Length of trigger pulse in microseconds
  bool OutputPulseOccuring; //if True, we are in the middle of outputting a pulse

  public:
  TrigOutManager (); //initializes trigger length. Currently this is hardwired into the function.
  void StartPulse();
  bool ShouldWeBeOutputting();
} TrigOutManager_Thru;


//initialize variables
TrigOutManager::TrigOutManager (){
  TrigLengthMicros = 1000;
}


void TrigOutManager::StartPulse(){
  OutputPulseOccuring = true;
  CurrentPulseEndtimeMicros = micros() + TrigLengthMicros;

  //If the micros() timer is close to rollover, just skip the pulse. Quick and dirty!
  if(CurrentPulseEndtimeMicros > 429496700 ){
    OutputPulseOccuring = false;
  }
}

bool TrigOutManager::ShouldWeBeOutputting(){
  if(OutputPulseOccuring){
    if (micros()>CurrentPulseEndtimeMicros){
      OutputPulseOccuring = false;
      return false;
    } else {
      return true;
    }
  } else {
    return false;
  }
}




//----------------------------------------------------------------------------------------------------------------------

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

//----------------------------------------------------------------------------------------------------------------------

void loop() {
  // deal with input
  clockInState = digitalRead(InPin_Trig);
  
  if(clockInState == 0 && clockInPrevState == 1){      //............if the Trigger In voltage switches from low to high
    InputPulsePredictor.InputPulse();
  }
    clockInPrevState = clockInState;

 

    
  // deal with output
  
  if(InputPulsePredictor.IsThereAPulse()==true){
    TrigOutManager_Thru.StartPulse();
  }

  if(TrigOutManager_Thru.ShouldWeBeOutputting()==true){
    digitalWrite(OutPin_Thru,HIGH);
  } else {
    digitalWrite(OutPin_Thru,LOW);
  }
  
  

  //Serial.println(InPin_Trig);
  //Serial.println(InputPulsePredictor.IsThereAPulse());
}
