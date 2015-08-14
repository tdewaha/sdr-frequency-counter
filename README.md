# sdr-frequency-counter
Software defined radio based frequency counter for finding the strongest signal within
a frequency range.

Build: 

    gcc counter.c kiss_fft.c -lrtlsdr -lm -o counter

Usage: 

    ./counter 140 150

Result:

    Found Elonics E4000 tuner
    Exact sample rate is: 2000000.052982 Hz
    Scanning from 140.000 MHz to 150.000 MHz
    BW: 2.0 MHz   RBW: 2.00 kHz

    Signal: 144.344006 @ -45.8 dB



Uses:
KissFFT for fast fourier transform (http://kissfft.sourceforge.net/)
librtlsdr to handle the dvb-t dongle (https://github.com/steve-m/librtlsdr)


