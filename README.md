# Fractional Clock Multiplier (wip)
8hp algorithmic rhythm-generation module for Eurorack, powered by the Teensy LC

Currently two main modes:
* Fractional Multiplication and division of steady or slowly-varying clock inputs.
  * No requirement for clock input to be multiple ppqn, as the clock input is predictive. I'd like to update this so it can deal with swung input also
* Generation of euclidean rhythms ( see [Toussaint 2005](http://cgm.cs.mcgill.ca/~godfried/publications/banff.pdf) ) from arbitrary (regular or non-regular) clock input 

Output can be shifted either by beat-quantized or subquantized amounts, and optionally recombined with the input. This can be used to generate swung clock output for example, or controlled via CV to generate complex compound rhythms. 

Selectable trigger/gate output modes. 

Assignable CV control over all paramaters, with attenuverter. 

Current schematic, panel mockup and PCB design:

<img src="https://github.com/TuskSE/Fractional-clock-multiplier/blob/master/Kicad%20schematic/Fractional%20clock%20divider%20PCB%202/Schematic.png" width="800" />

<img src="https://github.com/TuskSE/Fractional-clock-multiplier/blob/master/Panel%20mockup.png" width="150" /> <img src="https://github.com/TuskSE/Fractional-clock-multiplier/blob/master/Kicad%20schematic/Fractional%20clock%20divider%20PCB%202/topPCB.png" width="150" /> <img src="https://github.com/TuskSE/Fractional-clock-multiplier/blob/master/Kicad%20schematic/Fractional%20clock%20divider%20PCB%202/bottomPCB.png" width="150" /> 

<img src="https://github.com/TuskSE/Fractional-clock-multiplier/blob/master/Kicad%20schematic/Fractional%20clock%20divider%20PCB%202/render.png" width="400" />
