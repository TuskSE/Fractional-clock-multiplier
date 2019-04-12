#include <Metro.h> // Include the Metro library
#include <Encoder.h>

//Define all the pins
const int SwPin_DivEuc = 0;
const int SwPin_OutModes = 1;
const int SwPin_CVup = 2;
const int SwPin_CVdown = 3;
const int SwPin_Encoder = 6;
const int CtrPin_CVamt = 21;
const int CtrPin_Divisions = 20;
const int CtrPin_Length = 19;
const int OutPin_Cycle = 7;
const int OutPin_MainOut = 8;
const int OutPin_Thru = 9;
const int InPin_Trig = 18; //Low input voltage = HIGH reading on pin.
const int InPin_CV = 17;
const int LEDPin_Shuffle = 16;
const int LEDPin_internal = 13;
const int EncPinA = 4;
const int EncPinB = 5;

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
int ControlVoltage_in = 0;
int ControlValue_Divisions, ControlValue_Divisions_Scaled = 1;
int ControlValue_Length, ControlValue_Length_Scaled =1;

double pressTimeTemp = 0; //temporarily stores the time, when the button is pressed
int clockInState = 0;
int clockInPrevState = 0; // this will remember if the clock input was "on" in the previous cycle 

//multiplication/division factors. These will eventually be controlled by pots.
//float ClicksPerCycle = 8;
float MultDivFactor2 = 2;
float MultDivFactor3 = 0.5;

Encoder EncKnob(EncPinB, EncPinA);
int EncoderValTemp = 0;
int EncoderShiftInstruction = 0; 
float EncoderShuffleInstruction = 0.0;
const int EncoderCountsPerClick = 4; //property of the encoder used: how many digital output counts corresponding to one click (indent) felt by the user.
//const int EncoderClicksPerRevolution = 20; //property of the encoder used 
int EncoderShuffleNoOfClicks = 20; //The number of clicks of encoder rotation which makes the shuffle amount return to it's starting value

bool CVcontrolCycleLength, CVcontrolDivisions, CVcontrolShuffle, CVcontrolShift;

bool EuclideanMode_temp = true; //if 0, runs as clock divider. If 1, runs as euclidean pattern generator. 

int Temp;
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
  signed long int PredictionError[4]; //for the previous N pulses, record the difference between the actual pulse time and what was predicted
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
  { TrustworthyPulsePredictions = false; }
  
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

// the TrigOutManager is called on to generate output pulses, and handles e.g. giving said pulses a finite length

class TrigOutManager {
  unsigned long TrigLengthMicros, CurrentPulseEndtimeMicros; //Length of trigger pulse in microseconds
  const unsigned long GateModeTimeoutLengthMicros = 20000; //this is the time after initiation that a pulse can be killed by an external function.
  //^ this is neccessary because otherwise, in clock divider mode, through pulses and main pulses which ought to be exactly coincident are actually staggered a little, and if the through pulse comes slightly later - we don't want it to kill the main pulse in that case.
  bool OutputPulseOccuring; //if True, we are in the middle of outputting a pulse
  bool RolloverFlag; //if True, we are in the middle of a pulse which will straddle the point when micros() rolls over

  public:
  TrigOutManager (); //initializes trigger length. Currently this is hardwired into the function.
  void StartPulse();
  void EndPulse();
  bool ShouldWeBeOutputting(bool);
} TrigOutManager_Thru, TrigOutManager_Cycle, TrigOutManager_Main;


//initialize variables
TrigOutManager::TrigOutManager (){
  TrigLengthMicros = 1000;
  //GateModeTimeoutLengthMicros = 20000;
}


void TrigOutManager::StartPulse(){
  OutputPulseOccuring = true;
  CurrentPulseEndtimeMicros = micros() + TrigLengthMicros;
  
  //If the micros() timer is close to rollover, then the value of CurrentPulseEndtimeMicros will have rolled over, as both are unsigned long.
  //Work out if such a rollover has occured:
  if (CurrentPulseEndtimeMicros < TrigLengthMicros){  
    RolloverFlag = true;  } else {  
      RolloverFlag = false; }
}

void TrigOutManager::EndPulse(){
    //We allow a pulse to be cancelled only after a certain amount of time since the pulse started. this is neccessary because otherwise, in clock divider mode, through pulses and main pulses which ought to be exactly coincident are actually staggered a little, and if the through pulse comes slightly later - we don't want it to kill the main pulse in that case.
    if (micros()>(CurrentPulseEndtimeMicros+GateModeTimeoutLengthMicros)){
           OutputPulseOccuring = false;
    }
  //To do: deal with rollover!
}


bool TrigOutManager::ShouldWeBeOutputting(bool GateMode){
  if(OutputPulseOccuring){
  //if gate mode is on, we only stop a pulse when specifically instructed
  //otherwise, we check against the time the current pulse is supposed to end

      if(GateMode==true){
          return true;
        }else{ // check against the time the current pulse is supposed to end
               if(RolloverFlag == false){
                 if (micros()>CurrentPulseEndtimeMicros){
                    OutputPulseOccuring = false;
                   return false;
                  } else {
                    return true;
                 }
               }else{ // RolleverFlag == true
                 if ( (micros()>CurrentPulseEndtimeMicros) && (micros() < 100*TrigLengthMicros) ){
                   OutputPulseOccuring = false;
                   return false;
                 } else {
                   return true;
                 }
              }
        }
  } else { //no output pulse occuring
    return false;
  }
  
}


//------------------------------------------------------------------------------------------------------------
// This will handle clock division/multiplication. Its aim is to spread N output pulses evenly in the space between M 
// input pulses. The end of an input cycle will trigger the end of the output cycle, to ensure sync is maintained.
// To space out pulses correctly, it must recieve information about how long the cycle is expected to last. 

class DividerMultiplier {
  int InputCycleLength,OutputCycleLength,PositionInInputCycle,PositionInOutputCycle,NewPositionTemp, Debug_PositionInOutputCycleOld;
  unsigned long int ExpectedCycleLength, CycleStartTime, ExpectedCycleEndTime, ExpectedIntervalTime, PulseTimes[16], ShuffleTime;
  bool CycleOutPulseStart, MainOutPulseStart, ThruPulseStart, TransmitPulses;
  float fractionalShuffle;  
  
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
  void UpdateKnobValues(int, int);
  void ShiftPositionInInputCycle(int);
  void UpdateFractionalShuffleTime(float);
  void PrintPositionInOutputCycle();
  int ReportPositionInCycle();
  void SetPositionInCycle(int);
  
} DividerMultiplierMain;


DividerMultiplier::DividerMultiplier(){   //initalize values
  TransmitPulses = false;  // this is a master switch to turn output on/off
  CycleOutPulseStart = false;   //PulseStart flags are there for the trigger manager to pick up, and signal that an output pulse should be starting
  MainOutPulseStart = false;
  ThruPulseStart = false;
  InputCycleLength = 2;
  OutputCycleLength = 3;
  PositionInInputCycle = 0;  //Counter to keep track of our position in the cycle. Runs from 0 to N-1
  PositionInOutputCycle = 0; 
  CycleStartTime = 0;
  NewPositionTemp = 0;
  fractionalShuffle = 0.0;   //The fraction of an output beat by which the output should be delayed. Expected value between 0 and 1. 
  Debug_PositionInOutputCycleOld = 16;  
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
  
  PulseTimes[0] = CycleStartTime;
  
  //apply shuffle
  PulseTimes[0] = PulseTimes[0] + (fractionalShuffle*ExpectedIntervalTime);

  //calculate the remaining output pulse times
    for (int i=1; i < OutputCycleLength; i++){
      PulseTimes[i] = PulseTimes[i-1]+ExpectedIntervalTime;
    }
  }

}

void DividerMultiplier::CalculateOutputTimesMidCycle(){
  //both the input cycle time and output cycle time may have changed, as well as the fractional shuffle

  //first, we need to extrapolate back to when the start of the current input cycle would have been, then calculate the cycle time from that point
  CycleStartTime = InputPulsePredictor.TimeOflastPulsePlusN(PositionInInputCycle);
  ExpectedCycleEndTime = InputPulsePredictor.TimeOfNthPulse(InputCycleLength-PositionInInputCycle);

  if(CycleStartTime !=0 && ExpectedCycleEndTime !=0){
  //Serial.print("Engaged. Output cycle length =  ");
  //Serial.print(OutputCycleLength);
  ExpectedCycleLength = ExpectedCycleEndTime - CycleStartTime;

  //calculate the output times, adjusting for shuffle
    ExpectedIntervalTime = ExpectedCycleLength/OutputCycleLength;
    PulseTimes[0] = CycleStartTime + (fractionalShuffle*ExpectedIntervalTime);  //NB: PulseTimes[0] always falls after the Cycle Start time    
    for (int i=1; i<OutputCycleLength; i++){
      PulseTimes[i] = PulseTimes[i-1] + ExpectedIntervalTime;
    }

    //Now work out which pulse is due next
    for (int i=0; i<OutputCycleLength; i++){
      if(PulseTimes[i]>millis()){
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

    //If the Main trigger out gate mode is engaged, we need to end any gate-length pulse that might be occuring
   //TrigOutManager_Main.EndPulse();
   ThruPulseStart=true; // pass to thru output

  if (PositionInInputCycle >= InputCycleLength){    
      PositionInInputCycle = 0;
      PositionInOutputCycle = 0;
      CycleOutPulseStart = true;
      if(fractionalShuffle == 0.0){MainOutPulseStart = true;}
      CycleStartTime = millis();
      CalculateOutputTimesAtCycleStart();
  } else {
 //   //If the Main trigger out gate mode is engaged, we need to end any gate-length pulse that might be occuring
 //   TrigOutManager_Main.EndPulse();

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
  if (PositionInOutputCycle<OutputCycleLength){ //if this is not true, we already output the last pulse of the cycle and must wait until the recycle is reset
    if (millis() > PulseTimes[PositionInOutputCycle]){ //Check the current time against the time we know the upcoming pulse should start
      PositionInOutputCycle = PositionInOutputCycle + 1;  
      return true;
    } else {  return false; } 
    } else {return false; }
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
void DividerMultiplier::UpdateKnobValues(int InCycleLengthKnobPlusCV, int OutCycleLengthKnobPlusCV){
    if (InCycleLengthKnobPlusCV != InputCycleLength){
      InputCycleLength = InCycleLengthKnobPlusCV;
      DividerMultiplier::CalculateOutputTimesMidCycle();
      PositionInInputCycle = PositionInInputCycle % InputCycleLength;
      }
            
    if (OutCycleLengthKnobPlusCV != OutputCycleLength){
      //calculate the new set of times for the output cycle, and figure out where we are in the sequence
      OutputCycleLength = OutCycleLengthKnobPlusCV;
      DividerMultiplier::CalculateOutputTimesMidCycle();
    } 
  }

//change our position in the input cycle
void DividerMultiplier::ShiftPositionInInputCycle(int AmoutToShiftBy){
  if (AmoutToShiftBy != 0){
    NewPositionTemp = PositionInInputCycle + AmoutToShiftBy;
  
    //code elswhere expect "position in cycle" to be within a range of 0 .... N-1, where N is cycle length.
    //if we've ended up with a position outside this range, find the equivalent value within the range
    
    if (NewPositionTemp >= InputCycleLength){
      NewPositionTemp = NewPositionTemp + 1; //shift to a frame in which we count from 1 ... N, rather than 0 .... N-1, where N is cycle length
      NewPositionTemp = NewPositionTemp % InputCycleLength;
      NewPositionTemp = NewPositionTemp - 1; //shift back
    }
  
    while (NewPositionTemp < 0){
      NewPositionTemp = NewPositionTemp + InputCycleLength;
    }
  
    PositionInInputCycle = NewPositionTemp;
    DividerMultiplier::CalculateOutputTimesMidCycle();  // update output times to reflect where we now are in the input cycle
  }
}

void DividerMultiplier::UpdateFractionalShuffleTime(float AmountToChangeBy){
  if (AmountToChangeBy != 0){
    fractionalShuffle = fractionalShuffle + AmountToChangeBy;
  
    //force result to a value between 0 and 1;
      while (fractionalShuffle < 0 ){
        fractionalShuffle  = fractionalShuffle + 1.0;
      }
    
      while (fractionalShuffle >= 1.0){
        fractionalShuffle = fractionalShuffle - 1.0;
      }
 
    //Set brightness of the shuffle indicator LED to reflect the amount of shuffle
    analogWrite(LEDPin_Shuffle, fractionalShuffle*40);
    
    //update output times accordingly
    DividerMultiplier::CalculateOutputTimesMidCycle();

  }
}

void DividerMultiplier::PrintPositionInOutputCycle(){
  if(Debug_PositionInOutputCycleOld != PositionInOutputCycle){
      Serial.println(PositionInOutputCycle);

      if(PulseTimes[PositionInOutputCycle]>millis()){
        Serial.println("something is wrong!");
      }
  }
  Debug_PositionInOutputCycleOld = PositionInOutputCycle;
}

int DividerMultiplier::ReportPositionInCycle(){
  return PositionInInputCycle;
}

void DividerMultiplier::SetPositionInCycle(int NewPosition){
  PositionInInputCycle = NewPosition;
}


//------------------------------------------------------------------------------------------------------------------------
// EuclieanCalculator will handle euclidean rhythm generation.
//i.e. it will calculate on which beats of the input cycle should an output be triggered
//It will work by spreading m input pulses evenly along a circle, and generating a polygon of k vertices (k = number of hits) inside it. These values will "snap" to the next input pulse.
//"shuffle" will rotate the k-vertex polygon fractionally, such that they eventually snap to different points of the m-vertex polygon
//shift will rotate the k-vertex polygon by one (inter-m-vertex angle)
//to change the zero point of the cycle, we will rely on a (yet to be implemented) cycle reset input


class EuclideanCalculator {
  int InputCycleQuantization, NumberOfHits, mTemp, PositionInInputCycle, InputCycleResetPosition, AmountOfShift;
  bool HitLocationsInCycle [16], MainOutPulseStart, ThruPulseStart, CycleOutPulseStart, TransmitPulses, SnapToClosestPoint;
  float QuantizationPointSpacing, PolygonPointSpacing, QuantizationPointsOnCircle[16], PolygonVertexes[16], fractionalShiftOfPolygon;

  public:
  EuclideanCalculator ();
  void RecalculateRhythm();
  void UpdateKnobValues(int, int, float, float);
  void InputPulse();
  bool ShouldWeOutputMainPulse();
  bool ShouldWeOutputCyclePulse();
  bool ShouldWeOutputThruPulse();
  int ReportPositionInCycle();
  void SetPositionInCycle(int);
  
} EuclideanCalculatorMain;

EuclideanCalculator::EuclideanCalculator(){   //initalize values. See DividerMultiplier class for description of some variables. 
  InputCycleQuantization = 12;
  NumberOfHits = 5;
  TransmitPulses = true;
  CycleOutPulseStart = false;
  MainOutPulseStart = false;
  ThruPulseStart = false;
  SnapToClosestPoint = false;
  PositionInInputCycle = 0;
  fractionalShiftOfPolygon = 0;
  AmountOfShift = 0;
  InputCycleResetPosition = 0; //the position in the input cycle at which we transmit the "cycle" pulse, and which we start at when the sequence restarts. this allows us to manage phase shifts. 
}

int EuclideanCalculator::ReportPositionInCycle(){
  return PositionInInputCycle;
}

void EuclideanCalculator::SetPositionInCycle(int NewPosition){
  PositionInInputCycle = NewPosition;
}

void EuclideanCalculator::UpdateKnobValues(int InCycleQyantizationKnobPlusCV, int NumberOfHitsKnobPlusCV, float ShuffleKnobPlusCV, float ShiftInstruction){

//if any inputs have changed, we need to do something....
  if( (InCycleQyantizationKnobPlusCV != InputCycleQuantization) || (NumberOfHitsKnobPlusCV != NumberOfHits) || (ShuffleKnobPlusCV != 0) || (ShiftInstruction !=0)){

      if(ShuffleKnobPlusCV != 0){
        //we need to make sure the value fits between  0 and <1, and handle LED output
        //this is duplicated in the DividerMultiplier code - would be tider to rewrite this as one function, at some point
        fractionalShiftOfPolygon = fractionalShiftOfPolygon + ShuffleKnobPlusCV;

        while(fractionalShiftOfPolygon < 0.0){
          fractionalShiftOfPolygon = fractionalShiftOfPolygon + 1;
        }
        while(fractionalShiftOfPolygon >= 1.0){
           fractionalShiftOfPolygon = fractionalShiftOfPolygon - 1;
        }
        //Set brightness of the shuffle indicator LED to reflect the amount of shuffle
        analogWrite(LEDPin_Shuffle, fractionalShiftOfPolygon*40);
      }

      if(ShiftInstruction != 0){
        // shfit position in cycle
        AmountOfShift = AmountOfShift + ShiftInstruction;
      }
      
      //adjust shift value to be within the cycle length, just for tidyness, and more logical results when we switch the cycle length
      while (AmountOfShift >= InputCycleQuantization){
        AmountOfShift = AmountOfShift - InputCycleQuantization;
      }

      while (AmountOfShift < 0){
        AmountOfShift = AmountOfShift + InputCycleQuantization;
      }

      // Set length and hits
      InputCycleQuantization = InCycleQyantizationKnobPlusCV;
      NumberOfHits = NumberOfHitsKnobPlusCV;
      EuclideanCalculator::RecalculateRhythm();
      if (InCycleQyantizationKnobPlusCV != InputCycleQuantization){
        PositionInInputCycle = PositionInInputCycle % InputCycleQuantization;  //ensures that we don't screw up position in the input cycle when we change input cycle length
        }
  }

}


void EuclideanCalculator::InputPulse() {
  //registers an input pulse, advances the position in input cycle. If we have reached the end of the cycle, loop back to the start. 
  //if we are at the "InputCycleResetPosition", emit cycle pulse

  PositionInInputCycle = PositionInInputCycle + 1;

  //Pass a thru pulse
  ThruPulseStart=true;

  //If we are in main-out gate mode, end any gate which is occuring
  //TrigOutManager_Main.EndPulse();
  
    if (PositionInInputCycle >= InputCycleQuantization){    
      PositionInInputCycle = 0;
      CycleOutPulseStart = true;
  }

  //now we know where we are in the cycle, figure out if we need to transmit a pulse
  if (HitLocationsInCycle[PositionInInputCycle]==true){
    MainOutPulseStart = true;
    }
  
}


void EuclideanCalculator::RecalculateRhythm(){
  //Spread quantization points along a circle of circumfernece 1; first quantization at 0
  
  QuantizationPointSpacing = (1.00)/(float)InputCycleQuantization;
  QuantizationPointsOnCircle[0]=0.0;
  for(int i=1; i<InputCycleQuantization; i++){
    QuantizationPointsOnCircle[i] = QuantizationPointsOnCircle[i-1] + QuantizationPointSpacing;
  }
  QuantizationPointsOnCircle[InputCycleQuantization] = 1;
  for(int i = InputCycleQuantization+1; i<16; i++){    
    QuantizationPointsOnCircle[i] = 0;     //set unused values to 0
  }

  //Draw polygon corresponding to ideal (unquantized) hit locations
    PolygonPointSpacing = (1.00)/(float)NumberOfHits;

   //We seed the first vertex of the polygon, accounting for the shuffle and shift amounts
   PolygonVertexes[0] = 0.0 + QuantizationPointSpacing*fractionalShiftOfPolygon + QuantizationPointSpacing*AmountOfShift;
   //NB: QuantizationPointSpacing*AmountOfShift may have a value of up to one, meaning that when we construct the full polygon, we may have points with values up to 2.0, which we will need to map back on to the circle 0 to 1.0

   for(int i=1; i<NumberOfHits; i++){
      PolygonVertexes[i] = PolygonVertexes[i-1] + PolygonPointSpacing;
    }
   for(int i=NumberOfHits; i<16; i++){
      PolygonVertexes[i] = 0;
    }

    //ensure all points lie within 0 to 1.0
   for(int i=0; i<NumberOfHits; i++){
    if (PolygonVertexes[i]>=1){
        PolygonVertexes[i] = PolygonVertexes[i] - 1.0;

    }
   }
    
  //reset HitLocationsInCycle
  for(int i=0; i<16; i++){
    HitLocationsInCycle[i] = false;
  }

  //For each polygon point, find the nearest quantization point clockwise around the circle, and mark as a hit location
  for(int k=0; k<NumberOfHits; k++){
    mTemp =0;
    
    for (mTemp=0; mTemp<=InputCycleQuantization; mTemp++){
      if (QuantizationPointsOnCircle[mTemp] >= PolygonVertexes[k]){break;};
    }

    //Deal with the case where we have snapped to the quantization point of the *next* cycle (at 1.0), which is equivalent to the point at 0.0
    if(mTemp == InputCycleQuantization){
      mTemp = 0;
    }

      if(SnapToClosestPoint){
          //alternative mode only: for each polygon point, snap to the quantizaiton point *anticlockwise* around the circle, if that is closer than the clockwise one we have identified
          if(mTemp>0){
            if( (QuantizationPointsOnCircle[mTemp]-PolygonVertexes[k]) > (PolygonVertexes[k]-QuantizationPointsOnCircle[(mTemp-1)])){
              mTemp = mTemp-1;
            } 
          }else{
            if( 0.0-PolygonVertexes[k] > ((1+PolygonVertexes[k])-QuantizationPointsOnCircle[InputCycleQuantization-1]) ){
              mTemp = InputCycleQuantization-1;
            }
          }
      }
    HitLocationsInCycle[mTemp] = true;
  } 
}

  bool EuclideanCalculator::ShouldWeOutputMainPulse() {
    if(MainOutPulseStart){
      MainOutPulseStart = false; //registers that the instruction has been read
      return true;
    } else {
      return false;
    }
  }

  bool EuclideanCalculator::ShouldWeOutputCyclePulse(){
    if(CycleOutPulseStart){
      CycleOutPulseStart = false; //registers that the instruction has been read
      return true;
    } else {
      return false;
    }
  }

  bool EuclideanCalculator::ShouldWeOutputThruPulse(){
    if(ThruPulseStart){
      ThruPulseStart = false; //registers that the instruction has been read
      return true;
    } else {
      return false;
    }
  }


  //testing:
  //for(int m=0; m<InputCycleQuantization; m++){
    //Serial.print(HitLocationsInCycle[m]);
    //Serial.print(" ");
  //}
  //Serial.println(" "); 
  //}
  



//------------------------------------------------------------------------------------------------------------------------
//JitterSmoother is designed to filter out jitter in potentiometer voltages (and CV) which don't correspond to changes in input voltage. 
//This very simple implementation doesn't work especially well, and risks ignoring slow CV changes (eg from a slow LFO). 

class JitterSmoother { 
  int OldValue, NewValue;
  unsigned long int SampleInterval;
  int Threshold;
  unsigned long int TimeOfLastRead;
  
  public:
  JitterSmoother ();
  void SetSampleIntervalandThreshold(unsigned long int,int);
  int SmoothChanges(int);
} JitterSmootherL, JitterSmootherD, JitterSmootherCV, JitterSmootherAttenuverter;


JitterSmoother::JitterSmoother(){   //initalize values
 OldValue = 1;
 NewValue = 1;
 SampleInterval = 60;
 Threshold = 50;
 TimeOfLastRead = 1;
}

int JitterSmoother::SmoothChanges(int Input){
  if ( (millis() - TimeOfLastRead) > SampleInterval){

    //needed the first time we call the function 
    if (TimeOfLastRead == 1){
      NewValue = Input;
      TimeOfLastRead = millis();
      OldValue = Input;
      return NewValue;  
    }

    //On all the following occasions...
    TimeOfLastRead = millis();  //update for future reference
     
    if ( (Input - OldValue) > Threshold ){  
    NewValue = Input; 
  } else if ( (Input - OldValue) < (-Threshold) ){
    NewValue = Input;
  }  else{
    NewValue = OldValue;
  }

  OldValue = NewValue;
  }
return NewValue;  
}

void JitterSmoother::SetSampleIntervalandThreshold(unsigned long int NewSampleInt, int NewThreshold){
  SampleInterval = NewSampleInt;
  Threshold = NewThreshold;
}

//---------------------------------------------------------------
//ModeController handles switching between different modes of operation
class ModeController {
  bool EuclideanMode, DivMultMode, MainOutputGateOutMode;
  bool TempSwitchState;

  public:
  ModeController();
  void ReadModeSwitch();
  void ReadGateSwitch();
  bool AreWeInEuclideanMode();
  bool AreWeInDividerMultiplierMode();
  bool AreWeInMainOutputGateMode();
} ModeControllerMaster;

ModeController::ModeController(){ //initialize values
  EuclideanMode = true;
  DivMultMode = false;
  MainOutputGateOutMode = false;
}

void ModeController::ReadModeSwitch(){
  TempSwitchState = digitalRead(SwPin_DivEuc);
  if(DivMultMode != TempSwitchState){ // if switch statoe doesn't correspond to the mode we're in currently
    if(TempSwitchState == true){
      DivMultMode = true;
      EuclideanMode = false;
      DividerMultiplierMain.SetPositionInCycle(EuclideanCalculatorMain.ReportPositionInCycle());  
      DividerMultiplierMain.CalculateOutputTimesMidCycle();
    }else if(TempSwitchState == false){
      DivMultMode = false;
      EuclideanMode = true;
      EuclideanCalculatorMain.SetPositionInCycle(DividerMultiplierMain.ReportPositionInCycle());    
    }
  }
}


void ModeController::ReadGateSwitch(){
  TempSwitchState = digitalRead(SwPin_OutModes);
  if(TempSwitchState != MainOutputGateOutMode){ // if switch statoe doesn't correspond to the mode we're in currently
    if(TempSwitchState == true){
      MainOutputGateOutMode = true;
    } else {
      MainOutputGateOutMode = false;
    }
  }
}

bool ModeController::AreWeInEuclideanMode(){
  if(EuclideanMode == true){
    return true;
  } else {
    return false;
  }
}

bool ModeController::AreWeInDividerMultiplierMode(){
  if(DivMultMode == true){
    return true;
  } else {
    return false;
  }
}

bool ModeController::AreWeInMainOutputGateMode(){
  if(MainOutputGateOutMode == true){
    return true;
  } else {
    return false;
  }
}

//----------------------------------------------------------------------------------------------------------------------

//the CV assigner will control the routing of CV signals. Currently, everything is controlled by a 3-way switch and this scheme is a bit overkill. but I might want something more sophisticated in future.
class CVassigner {
  bool SwPin_CVup_State;
  bool SwPin_CVdown_State;
//  bool CVcontrolCycleLength, CVcontrolDivisions, CVcontrolShuffle, CVcontrolShift;   <-- I made these to global for now
  float ControlValue_Attenuverter, ControlValue_CVin, ShuffleModifierOld, ShuffleModifierNew, ShuffleModifierChange;
  int CVzeroReading, ShiftModifierOld, ShiftModifierNew, ShiftModifierChange; //the analogRead that results when the control votlage = 0;
  const int CVscalingFactor = 80; //scales the output of CVassigner::readCV to give a result <2000
  const int ShiftCVScalingFactor = 200; //scales the effect CVassigner::ReturnShiftModifier
  const int ShuffleCVScalingFactor = 80; //scales the effect CVassigner::ReturnShuffleModifier


  public:
  CVassigner ();
  void UpdateCVrouting();
  int readCV();
  int ReturnShiftModifier();
  float ReturnFractionalShuffleModifier();
  
} CVassignerMaster;

CVassigner::CVassigner(){  //default starting state is for CV to control divisions
  SwPin_CVup_State = false;
  SwPin_CVdown_State = false;
  CVcontrolCycleLength = false;
  CVcontrolDivisions = true;
  CVcontrolShuffle = false;
  CVcontrolShift = false;
  ControlValue_Attenuverter = 0;
  ControlValue_CVin = 0;
  CVzeroReading = 1710; //this is measured empirically and then hard-coded, for now.
  ShiftModifierOld = 0;
  ShiftModifierNew = 0;
  ShiftModifierChange = 0;
  ShuffleModifierOld = 0.0;
  ShuffleModifierNew = 0.0;
  ShuffleModifierChange = 0.0;
}

void CVassigner::UpdateCVrouting(){
  if ( SwPin_CVup_State != digitalRead(SwPin_CVup) || SwPin_CVdown_State != digitalRead(SwPin_CVdown)  ){   //if the CV switch has changed position
    SwPin_CVup_State = digitalRead(SwPin_CVup);
    SwPin_CVdown_State = digitalRead(SwPin_CVdown);
    if( (SwPin_CVup_State == true) && (SwPin_CVdown_State == false) ){
      CVcontrolCycleLength = true;
      CVcontrolDivisions = false;
      CVcontrolShuffle = false;
      CVcontrolShift = false;
    }else if( (SwPin_CVup_State == true) && (SwPin_CVdown_State == true) ){  //switch in middle position. This might be false/false, depending on the switch, I guess
      CVcontrolCycleLength = false;
      CVcontrolDivisions = true;
      CVcontrolShuffle = false;
      CVcontrolShift = false;
    }else if( (SwPin_CVup_State == false) && (SwPin_CVdown_State == true) ){
      CVcontrolCycleLength = false;
      CVcontrolDivisions = false;
      CVcontrolShuffle = false;
      CVcontrolShift = true;
    }else{
      //Serial.println("ERROR: the CV assign switch is both up and down!");
    }
  }
}

//read CV will return an value between +- ~4000, scaled by the attenuverter (CV amt) knob. 
int CVassigner::readCV(){
  ControlValue_Attenuverter = map(analogRead(CtrPin_CVamt),0,4096,+100,-100);
  ControlValue_Attenuverter = map(JitterSmootherAttenuverter.SmoothChanges(analogRead(CtrPin_CVamt)),0,4096,+100,-100);  //read twice to give time for the ADC capacitor to equilibrate, avoiding crosstalk from other analog reads 
  //aside: do we need a voltage smoother on the above?
  ControlValue_CVin = analogRead(InPin_CV)-CVzeroReading;
  ControlValue_CVin = JitterSmootherCV.SmoothChanges(analogRead(InPin_CV))-CVzeroReading; //read twice to give time for the ADC capacitor to equilibrate, avoiding crosstalk from other analog reads 
  
  return (int)(ControlValue_Attenuverter*ControlValue_CVin/CVscalingFactor);

}


int CVassigner::ReturnShiftModifier(){
  //the main fuction controlling shift takes an integer input, which is the amount to shift by: void DividerMultiplier::ShiftPositionInInputCycle(int AmoutToShiftBy)
  //we need to keep track of an overall shift related to CV, and send a signal to that fuction only when control voltage changes cause that shift to change
  
  if ( CVcontrolShift == false ) {
    if (ShiftModifierNew != 0){  //if CV control over shift just got disabled, we want to make sure we reverse any changes which CV control exerted previously
      ShiftModifierOld = ShiftModifierNew;
      ShiftModifierNew = 0;
      return -(ShiftModifierNew-ShiftModifierOld);
    } else {
      return 0; 
    }
  } 

    ShiftModifierOld = ShiftModifierNew; //rememeber shift modifier from last function call
    ShiftModifierNew = (int)(CVassigner::readCV()/ShiftCVScalingFactor);
    ShiftModifierChange = ShiftModifierNew-ShiftModifierOld;
    return -(ShiftModifierChange);

    
}


float CVassigner::ReturnFractionalShuffleModifier(){
  if ( CVcontrolShuffle == false ) {
    if (ShuffleModifierNew != 0){ 
      //if CV control over shuffle just got disabled, we want to make sure we reverse any changes which CV control exerted previously
      ShuffleModifierOld = ShuffleModifierNew;
      ShuffleModifierNew = 0;
      return (ShuffleModifierNew-ShuffleModifierOld);
    } else{
      return 0.0; 
    }
  } else { 
    ShuffleModifierOld = ShuffleModifierNew; //rememeber shift modifier from last function call
    ShuffleModifierNew = (float)(CVassigner::readCV()/ShuffleCVScalingFactor)/(float)EncoderShuffleNoOfClicks; //This gives the output the same quantization as the encoder
    ShuffleModifierChange = ShuffleModifierNew - ShuffleModifierOld;
 
    return ShuffleModifierChange;
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

  EncKnob.write(0);

  JitterSmootherCV.SetSampleIntervalandThreshold(60,200); //tunes the smoothing algorith for the control voltage
}


//----------------------------------------------------------------------------------------------------------------------

void loop() {

  ModeControllerMaster.ReadModeSwitch();
  ModeControllerMaster.ReadGateSwitch();
    
  // deal with input
  clockInState = digitalRead(InPin_Trig);
  if(clockInState == 0 && clockInPrevState == 1){      //............if the Trigger In voltage switches from low to high
    
    //pass to the pulse predictor
    InputPulsePredictor.InputPulse();

    //pass to whatever's controlling the current output mode
    if( ModeControllerMaster.AreWeInEuclideanMode() ){
      EuclideanCalculatorMain.InputPulse();
    }else{
      DividerMultiplierMain.InputPulse();
    }
    
  }
    clockInPrevState = clockInState;


  //update knob control values
  //we repeat the reading twice, because if we take the first reading, it will still have some residual influence from the previous reading because the capacitor hasn't had time to charge/discharge. 
  ControlValue_Length = analogRead(CtrPin_Length);
  ControlValue_Length = JitterSmootherL.SmoothChanges(analogRead(CtrPin_Length));
  ControlValue_Divisions = analogRead(CtrPin_Divisions);
  ControlValue_Divisions = JitterSmootherD.SmoothChanges(analogRead(CtrPin_Divisions));
  //Might need to re-introduce jitter smoothing at some point


  //add control voltage to knob value as appriate
  //Prevent the voltage from pushing the control value beyond the normal range of the knob. 
  if (CVcontrolCycleLength == true){
    ControlValue_Length = ControlValue_Length + CVassignerMaster.readCV();
    if(ControlValue_Length < 0){ControlValue_Length = 0;}
    if(ControlValue_Length > 4096){ControlValue_Length = 4096;}
  }


  if(CVcontrolDivisions == true){
    ControlValue_Divisions = ControlValue_Divisions + CVassignerMaster.readCV();
    if(ControlValue_Divisions < 0){ControlValue_Divisions = 0;}
    if(ControlValue_Divisions > 4096){ControlValue_Divisions = 4096;}
  }

  //scale control values to be within specified range
  ControlValue_Length_Scaled = map(ControlValue_Length,0,4096,Length_min,Length_max);
  ControlValue_Divisions_Scaled = map(ControlValue_Divisions,0,4096,Divisions_min,Divisions_max);

  //check if the encoder has been turned since the last cycle
  EncoderValTemp = -EncKnob.read();

  //If the encoder has been turned, update values accordingly
  if (EncoderValTemp !=  0){
    if((EncoderValTemp % EncoderCountsPerClick) == 0){  //if this is not true, the knob has not reached an indent yet and must be mid-turn - do nothing     
      if(digitalRead(SwPin_Encoder)==0){
         //if the encoder switch was *not* pushed, we update a shift paramater. This will be fed to the Shift function after any control voltage is added
         EncoderShiftInstruction = (int)EncoderValTemp/EncoderCountsPerClick;
      } else if(digitalRead(SwPin_Encoder)==1){
         //if the encoder switch *was* pushed, update a shuffle paramater 
         EncoderShuffleInstruction = (float)(-EncoderValTemp/EncoderCountsPerClick)/(float)(EncoderShuffleNoOfClicks);
      }
      EncKnob.write(0); //Now that we have read the accumulated value from the encoder, reset encoder reading to zero
    }
  }


  //if(EncoderShiftInstruction!=0){Serial.println(EncoderShiftInstruction);}

  //Apply knob changes and Shift/Shuffle to pattern, according to encoder changes AND control voltage changes as appropriate 
  if ( ModeControllerMaster.AreWeInEuclideanMode() ) {
     EuclideanCalculatorMain.UpdateKnobValues(ControlValue_Length_Scaled, ControlValue_Divisions_Scaled, EncoderShuffleInstruction + CVassignerMaster.ReturnFractionalShuffleModifier(), -EncoderShiftInstruction + CVassignerMaster.ReturnShiftModifier());
  } else {
     DividerMultiplierMain.UpdateKnobValues(ControlValue_Length_Scaled, ControlValue_Divisions_Scaled);
     DividerMultiplierMain.ShiftPositionInInputCycle(EncoderShiftInstruction + CVassignerMaster.ReturnShiftModifier());
     DividerMultiplierMain.UpdateFractionalShuffleTime(EncoderShuffleInstruction + CVassignerMaster.ReturnFractionalShuffleModifier());
  }

  EncoderShiftInstruction = 0; 
  EncoderShuffleInstruction = 0.0;
  
  
  //check routing of CV signals
  CVassignerMaster.UpdateCVrouting();


  // deal with starting output pulses
  //for gate output mode, we need to tell the trigger manager to finish emitting pulse(gate) only if a Thru trigger is signalled without an accompanying main trigger
  if ( ModeControllerMaster.AreWeInEuclideanMode() )
  {
    // ask euclidean generator whether we should output a pulse
    if(EuclideanCalculatorMain.ShouldWeOutputThruPulse()==true){
        TrigOutManager_Thru.StartPulse();
        TrigOutManager_Main.EndPulse();
      }
    
      if(EuclideanCalculatorMain.ShouldWeOutputCyclePulse()==true){
        TrigOutManager_Cycle.StartPulse();
      }
    
      if(EuclideanCalculatorMain.ShouldWeOutputMainPulse()==true){
        TrigOutManager_Thru.StartPulse();
        TrigOutManager_Main.StartPulse();
      }
  } else  {
    // ask divider/multiplier whether we should output a pulse
      if(DividerMultiplierMain.ShouldWeOutputThruPulse()==true){
        TrigOutManager_Thru.StartPulse();
        TrigOutManager_Main.EndPulse();
        TrigOutManager_Main.EndPulse();
      }
    
      if(DividerMultiplierMain.ShouldWeOutputCyclePulse()==true){
        TrigOutManager_Cycle.StartPulse();
      }
    
      if(DividerMultiplierMain.ShouldWeOutputMainPulse()==true){
        TrigOutManager_Thru.StartPulse();
        TrigOutManager_Main.EndPulse();
        TrigOutManager_Main.StartPulse();
      }
  }

  //deal with actually setting voltage on the outputs
  if(TrigOutManager_Thru.ShouldWeBeOutputting(false)==true){
    digitalWrite(OutPin_Thru,HIGH);
  } else {
    digitalWrite(OutPin_Thru,LOW);
  }

  if(TrigOutManager_Main.ShouldWeBeOutputting(ModeControllerMaster.AreWeInMainOutputGateMode())){
    digitalWrite(OutPin_MainOut,HIGH);
  } else {
    digitalWrite(OutPin_MainOut,LOW);
  }

  if(TrigOutManager_Cycle.ShouldWeBeOutputting(false)==true){
    digitalWrite(OutPin_Cycle,HIGH);
  } else {
    digitalWrite(OutPin_Cycle,LOW);
  }
  
}
