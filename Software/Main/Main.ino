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
int ControlValue_Divisions = 1;
int ControlValue_Length =1;

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
  signed long int PredictionError[4]; //for the previous 10 pulses, record the difference between the actual pulse time and what was predicted
  bool TrustworthyPulsePredictions; // records whether the incoming clock is predictable
  bool PulseRecieved; //notes when a pulse has been recieved
  bool TransmitPulse; //notes when a pulse should be transmitted to downstream code segments
  bool PulsesShouldBeTransmitted; //notes whether pulses ought to be transmitted

  public:
  PulsePredictor (); //sets default initialization values  
  void InputPulse (); //engaging this tells the Pulse Predictor that a Pulse has arrived
  unsigned long int TimeOfNthPulse (int n); //returns the estimated time of future pulses
  unsigned long int PulseInterval (int n1, int n2); //returns the expected time between future pulses
  unsigned long int TimeOflastPulsePlusN(int); //returns the time of a pulse in the past. n=0 => last pulse. n=1 is the pulse before that. etc
  bool IsThereAPulse ();
  void RecalculatePredictions ();
  void CheckReliability ();
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

      //check this time against what the pulse predictor was expecting, and record
      
        for(int i=3; i>0; i--){
         PredictionError[i] = PredictionError[i-1];
        }
      PredictionError[0] = recentPulseTimes[0]-predictedFuturePulseTimes[0];

      CheckReliability (); // check how reliable the last few predictions have been
      RecalculatePredictions (); //recalculate estimates of upcoming pulse times
}


void PulsePredictor::CheckReliability (){
  int sum = 0;
  for (int i=0; i<4; i++){
    sum = sum + PredictionError[i];
  }

  if (sum < 10) { TrustworthyPulsePredictions = true; } else
  { TrustworthyPulsePredictions = true; }
  
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

unsigned long int PulsePredictor::TimeOfNthPulse (int n){
  if (TrustworthyPulsePredictions){
      return predictedFuturePulseTimes[n-1];
  } else {
    return 0; //this will be the signal that there are no reliable predictions
  }
}


unsigned long int PulsePredictor::TimeOflastPulsePlusN (int n){
  if (TrustworthyPulsePredictions){
      return recentPulseTimes[n];
  } else {
    return 0; //this will be the signal that there are no reliable predictions
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
} TrigOutManager_Thru, TrigOutManager_Cycle, TrigOutManager_Main;


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


//------------------------------------------------------------------------------------------------------------
// This will handle clock division/multiplication. Its aim is to spread N output pulses evenly in the space between M 
// input pulses. The end of an input cycle will trigger the end of the output cycle, to ensure sync is maintained.
// To space out pulses correctly, it must recieve information about how long the cycle is expected to last. 

class DividerMultiplier {
  int InputCycleLength,OutputCycleLength,PositionInInputCycle,PositionInOutputCycle;
  unsigned long int ExpectedCycleLength, CycleStartTime, ExpectedCycleEndTime, ExpectedIntervalTime, PulseTimes[16];
  bool CycleOutPulseStart, MainOutPulseStart, ThruPulseStart, TransmitPulses;
  
  public:
  DividerMultiplier ();
  void CycleReset();
  bool ShouldWeOutputCyclePulse();
  bool ShouldWeOutputMainPulse();
  bool ShouldWeOutputThruPulse();
  void InputPulse();
  void SetInOutCycleLengths(int, int);
  void CalculateOutputTimesAtCycleStart();
  void CalculateOutputTimesMidCycle();
  void UpdateKnobValues(int, int, int);
} DividerMultiplierMain;


DividerMultiplier::DividerMultiplier(){   //initalize values
  TransmitPulses = false;
  CycleOutPulseStart = false;
  MainOutPulseStart = false;
  ThruPulseStart = false;
  InputCycleLength = 2;
  OutputCycleLength = 3;
  PositionInInputCycle = 0;
  PositionInOutputCycle = 0; 
  CycleStartTime = 0;
}

void DividerMultiplier::SetInOutCycleLengths(int in, int out){
  InputCycleLength = in;
  OutputCycleLength = out;
}

void DividerMultiplier::CalculateOutputTimesAtCycleStart() {
 //do not call this until we have enough information to do the calculations!
 ExpectedCycleEndTime = InputPulsePredictor.TimeOfNthPulse(InputCycleLength); 

 if(CycleStartTime !=0 && ExpectedCycleEndTime !=0){
  TransmitPulses = true;
  ExpectedCycleLength = ExpectedCycleEndTime - CycleStartTime;
  ExpectedIntervalTime = ExpectedCycleLength/OutputCycleLength;
  
Serial.println(" ");
Serial.print(CycleStartTime);
Serial.print("  to   ");
Serial.println(ExpectedCycleEndTime);

Serial.println(ExpectedIntervalTime);

  
  PulseTimes[0] = CycleStartTime;
      Serial.print(PulseTimes[0]);
      Serial.print(" ");
  
    for (int i=1; i < OutputCycleLength; i++){
      PulseTimes[i] = PulseTimes[i-1]+ExpectedIntervalTime;
      Serial.print(PulseTimes[i]);
      Serial.print(" ");
    }
  }  
}

void DividerMultiplier::CalculateOutputTimesMidCycle(){
  //both the input cycle time and output cycle time may have changed

  //first, we need to extrapolate back to when the start of the current input cycle would have been, then calculate the cycle time from that point
  CycleStartTime = InputPulsePredictor.TimeOflastPulsePlusN(PositionInInputCycle);
  ExpectedCycleEndTime = InputPulsePredictor.TimeOfNthPulse(InputCycleLength-PositionInInputCycle);

  if(CycleStartTime !=0 && ExpectedCycleEndTime !=0){
  ExpectedCycleLength = ExpectedCycleEndTime - CycleStartTime;

  //calculate the output times
    ExpectedIntervalTime = ExpectedCycleLength/OutputCycleLength;
    PulseTimes[0] = CycleStartTime;
    PulseTimes[OutputCycleLength-1] = CycleStartTime + ExpectedCycleLength;
    for (int i=(OutputCycleLength-1); i > 0; i--){   //calculate from the end of the cycle backwards until we get to the pulse due next
        PulseTimes[i-1] = PulseTimes[i]-ExpectedIntervalTime;
        if (PulseTimes[i-1]<millis()) { //PulseTimes[i] must be the next pulse due
          PositionInOutputCycle = i;
          break;
        }
    }
  } 
}


// registers an input pulse. Advances the input pulse clock.  
//if we reached the end of the cycle, loop back to the start, emit both pulses, and calculate the times for the next set of pulses
void DividerMultiplier::InputPulse() {
  PositionInInputCycle = PositionInInputCycle + 1;
  
  ThruPulseStart=true; // pass to thru output

  if (PositionInInputCycle >= InputCycleLength){    
      PositionInInputCycle = 0;
      PositionInOutputCycle = 0;
      CycleOutPulseStart = true;
      MainOutPulseStart = true;
      CycleStartTime = millis();
      CalculateOutputTimesAtCycleStart();
  }
      
}


void DividerMultiplier::CycleReset() {
  //This will mark the next input pulse as the start of the cycle
    PositionInInputCycle=InputCycleLength-1;
}


bool DividerMultiplier::ShouldWeOutputCyclePulse(){
  if (TransmitPulses==true){
    if(CycleOutPulseStart){
      CycleOutPulseStart = false; //registers that the instruction has been read
      return true;
      } else {
        return false;
      } 
    } else {
      return false;
    } 
}


bool DividerMultiplier::ShouldWeOutputMainPulse(){
  if (millis() > PulseTimes[PositionInOutputCycle]){
    if (PositionInOutputCycle<OutputCycleLength){      //The cycle only loops back to the start when triggered by the input signal
      PositionInOutputCycle = PositionInOutputCycle + 1;  
      return true;       Serial.println(PositionInOutputCycle);

    } else
      return false; 
  } else {
      return false;
  }
}

  bool DividerMultiplier::ShouldWeOutputThruPulse(){    //Note: pulses from "main" will ALSO be passed to the thru output in divider mode. That's not handled by this function.
    if(ThruPulseStart){
      ThruPulseStart = false; //registers that the instruction has been read
      return true;
      } else {
        return false;
      } 
}

//Adjust cycle paramaters without screwing up our place in the cycle
void DividerMultiplier::UpdateKnobValues(int InCycleLengthKnobPlusCV, int OutCycleLengthKnobPlusCV, int ShuffleKnobPlusCV){
    if (InCycleLengthKnobPlusCV != InputCycleLength){
      InputCycleLength = InCycleLengthKnobPlusCV;
      PositionInInputCycle = PositionInInputCycle % InputCycleLength; }
  // to do: we need to recalculate the cycle length, and accordingly adjust the output times, and figure out where we are in the output sequence
      
    if (OutCycleLengthKnobPlusCV != OutputCycleLength){
      //calculate the new set of times for the output cycle, and figure out where we are in the sequence
    } 
  }


//------------------------------------------------------------------------------------------------------------------------
//JitterSmoother is designed to filter out jitter in potentiometer voltages (and CV) which don't correspond to changes in input voltage. 
//This very simple implementation doesn't work especially well, and risks ignoring slow CV changes (eg from a slow LFO). 

class JitterSmoother { 
  int OldValue, NewValue;
  const unsigned long int SampleInterval = 60;
  
  public:
  JitterSmoother ();
  int SmoothChanges(int);
  static unsigned long int TimeOfLastRead;
} JitterSmootherL, JitterSmootherD, JitterSmootherCV;


JitterSmoother::JitterSmoother(){   //initalize values
 OldValue = 0;
 NewValue = 0;
}

int JitterSmoother::SmoothChanges(int Input){
if ((millis() - TimeOfLastRead) > SampleInterval){
  TimeOfLastRead = millis();
  if ( (Input - OldValue) > 50 ){
  NewValue = Input;
} else if ( (Input - OldValue) < -50 ){
  NewValue = Input;
}  else{
    NewValue = OldValue;
  }
OldValue = NewValue;
}
return NewValue;  
}

unsigned long int JitterSmoother::TimeOfLastRead = 0;  


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
  
  analogReadRes(12);

  Serial.begin(9600);

}


//----------------------------------------------------------------------------------------------------------------------

void loop() {
  // deal with input
  clockInState = digitalRead(InPin_Trig);
  
  if(clockInState == 0 && clockInPrevState == 1){      //............if the Trigger In voltage switches from low to high
    InputPulsePredictor.InputPulse();
    DividerMultiplierMain.InputPulse();
  }
    clockInPrevState = clockInState;


  //update control values
  //we repeat the reading twice, because if we take the first reading, it will still have some residual influence from the previous reading because the capacitor hasn't had time to charge/discharge. 
  ControlValue_Length = map(JitterSmootherL.SmoothChanges(analogRead(CtrPin_Length)),0,4096,Length_min,Length_max);
  ControlValue_Length = map(JitterSmootherL.SmoothChanges(analogRead(CtrPin_Length)),0,4096,Length_min,Length_max);
  ControlValue_Divisions = map(JitterSmootherD.SmoothChanges(analogRead(CtrPin_Divisions)),0,4096,Divisions_min,Divisions_max);
  ControlValue_Divisions = map(JitterSmootherD.SmoothChanges(analogRead(CtrPin_Divisions)),0,4096,Divisions_min,Divisions_max);

  DividerMultiplierMain.UpdateKnobValues(ControlValue_Length, ControlValue_Divisions, 0);

  // deal with starting output pulses
  if(DividerMultiplierMain.ShouldWeOutputThruPulse()==true){
    TrigOutManager_Thru.StartPulse();
  }

  if(DividerMultiplierMain.ShouldWeOutputCyclePulse()==true){
    TrigOutManager_Cycle.StartPulse();
  }

    if(DividerMultiplierMain.ShouldWeOutputMainPulse()==true){
    TrigOutManager_Main.StartPulse();
    TrigOutManager_Thru.StartPulse();
  }
 

  //deal with actually setting voltage on the outputs
  if(TrigOutManager_Thru.ShouldWeBeOutputting()==true){
    digitalWrite(OutPin_Thru,HIGH);
  } else {
    digitalWrite(OutPin_Thru,LOW);
  }

  if(TrigOutManager_Main.ShouldWeBeOutputting()==true){
    digitalWrite(OutPin_MainOut,HIGH);
  } else {
    digitalWrite(OutPin_MainOut,LOW);
  }

  if(TrigOutManager_Cycle.ShouldWeBeOutputting()==true){
    digitalWrite(OutPin_Cycle,HIGH);
  } else {
    digitalWrite(OutPin_Cycle,LOW);
  }

  
}
