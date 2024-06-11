# Golden lion VCV Rack collection
A collection of modules for VCV rack made for my "Individual Project" course.
The main goal for this project was to learn about DSP by implementing my own modules for vcv rack.
Please keep in mind that this project is far from finished. Many modules are implemented to the minimal requirements. 
The VCO module currently still has problems with aliasing. A lot of code is still in its first form and needs 
refactoring/updating.

# In this collection
![](/docs/ALL.png)
## VCO
The voltage controlled oscillator is the sound source of the synthesiser. It creates different wave forms within the 
audio frequency spectrum. This oscillator is able to output a sine, square, triangle and saw wave. It has 
modulation inputs to control the frequency. Both a standard 1 volt per octave to control the base frequency and a 
frequency modulation input are present.
The VCO also implements sync which means that another VCO signal can be used to reset the phase of the first VCO's signal.

## VCA
The voltage controlled amplifier controls the amplitude of a wave. This is a very simple module that has an input for 
the wave that is being controlled and an input to modulate the amplitude using modulation sources. It then outputs the 
modified wave.

## VCF
The voltage controlled filter is be the most intricate of the proposed modules. A filter is used to remove unwanted 
harmonics. A filter often supports one or more of the following types of filtering: low pass, high pass and band pass. 
A low pass filter removes harmonics above the cutoff frequency, a high pass filter removes the harmonics below 
the cutoff and a badpass will only allow the harmonics within a range around the cutoff frequency to pass through.
In hardware this can be done relatively easily. Since in a digital synthesizer the only input the filter has is the 
input voltage at a given point in time mathematical functions are needed to compute which voltage to send out.

The GL-VCF provides both IIR and FIR filtering, the filtering method can be switched in the context menu under `Filter settings`->`mode`.
The IIR filter is a cascading butterworth filter using 3 biquad filters and passing the output from one on to the next.
The IIR filter also implements resonance in the form of a biquad peak boost filter. This filter is applied after the 3
biquad filters. Currently the filter becomes unstable and crashes after a couple seconds when gain or bandwidth are 0.
The FIR filter is a basic FIR filter using the window-sinc method. Specifically, the hamming window function is used.

## LFO
The low frequency oscillator is the second modulation source. This oscillator outputs waves that can have frequencies 
below the audio frequency spectrum however, often the output frequency can go up to 1024 HZ which is inside the audio 
range. This can be used to create all sorts of effects like amplitude modulation.

## ADSR
The ADSR (attack, decay, sustain, release) envelope generator is the first of 2 modulation sources. At the most basic 
level it takes a gate input and outputs voltage follwing a predetermined function. The function consists of 3 phases 
being the attack, decay, release phase each of which will have a seperate input (likely a knob/slider) to determine 
their length. Additionally, an input to determine the voltage at which the decay phase will level out is needed, this is 
the sustain input. As long as the gate is open, the output voltage will remain at this level. Modulation inputs can be 
added to change the length of the 3 phases and the level of sustain.

## Noise
This module currently only generates a white noise signal. Red noise has not yet been implemented.

## Spring modulation
This module provides a modulation source based on a simulation of damped springs. Given a mass, liquid viscosity and a
spring constant. This will simulate the oscillation of the spring when starting at 10v. The damping only influences the
ossillations amplitude and not its frequency.

## HATS
A module to create hihats.
Currently not implemented.

## KICKS
A module to synthesize kick drums.
Currently not implemented.

## SNARES
A module to synthesize snare drums.
Currently not implemented.


# Installing
In order to install the modules the VCV Rack SDK is needed.
Installing:
```fish
export RACK_DIR=<RACK_SDK_DIR>
make install
```
