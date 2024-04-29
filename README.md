Lock in filter
==============

Lock in amplifiers are often required to read a small signal
with a given specific 
frequency (of known phase) in a noisy signal.
An FFT or a notch filter will not resolve the signal well if the noise
signal is swamping the small signal.
First the d.c. term is removed using a custom Z transform
double pole and zero filter. This may apply a slight gain to the 
signal (in this case around a gain of 1.1).

The signal is then rectified by multiplying by 1 is
the sine is in +ve phase and -1 if in -ve phase.

This is then integrated and then passed to a standard single pole low pass filter.
The resultant signal represents the small signal level that was burried in noise.

RPC 29APR2024
=========================================================================
