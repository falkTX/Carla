JAAA -- Quick manual
____________________


Some buttons will light up yellow when you click on them,
and there will be at most one of those active at any time.
This indicates the 'current parameter', which in most
cases can be modified in a number of ways:

- by typing a new value into the text widget, followed by ENTER,
- by using the '<' or '>' buttons to decrement or increment,
- by using the mouse wheel,
- by mouse gestures inside the display area.

Input
-----

Select on of the four inputs.


Frequency and Amplitude
-----------------------

Frequency:

  'Min' and 'Max' set the min and max displayed frequencies.
  If either of these is selected then 
  
     a horizontal Drag Left changes 'Min',
     a horizontal Drag Right changes 'Max'.
  
  'Cent' is the frequency at the middle of the x-axis.
  'Span' is 'Max' - 'Min', changing this value preserves 'Cent'. 
  If either of these is selected then
  
     a horizontal Drag Left changes 'Cent',
     a horizontal Drag Right changes 'Span'.

   'Cent' can also be set by Clicking in the frequency axis scale.
   
Amplitude:

  'Max' is the maximum value on the y-axis.
  'Range' is the range of the y-axis.
  If either of these is selected then
 
     a vertical Drag Left changes 'Max',
     a vertical Drag Right changes 'Range',

So for the last four mouse gestures, a Drag Left will scroll the
display, while a Drag Right will zoom in or out. Maybe I will add
and automatic selection of the axis based on the direction of the
mouse gesture.
 

Analyser
--------

The analyser is based on a windowed FFT. Actually the windowing is performed
by convolution after the FFT, and combined with interpolation. The windowing
and interpolation ensure that displayed peaks will be accurate to 0.25 dB even
if the peak falls between the FFT bins. More accurate measurements can be made
using the markers (see below).

'Bandw' sets the FFT length, and hence the bandwidth of the analyser. Depending
on this value, the size of the display and the frequency range, you may sometimes
see two traces. This happens when the resolution of the analyser is better than the
display, so that one pixel contains more than one analyser value. In that case, the
dark blue trace is the peak value over the frequency range represented by each pixel,
and lighter one is the average value. The first one is correct for discrete frequencies,
and the latter should be used to read noise densities.
There is no mouse gesture to change the bandwidth.

'VidAv' or video average, when switched on, averages the measured energy over time.
This is mainly used to measure noise. The averaging lenght increases over time, to
a maxumum of 1000 iterations. Changing the input or bandwidth resets and restarts
the averaging.

'Freeze' freezes the analyser, but not the display, so you can still scroll and
zoom or use the markers discussed below.


Markers
-------

Markers are used in order to accurately read off values in the display. There can be
up to two markers, set by clicking at the desired frequency inside the display.
When there are two markers, the second one will move with each click, while the first
remains fixed. Measured values for the two markers, and their difference in frequency
and level are displayed in the upper left corner of the display.

'Clear' clears the markers.

When 'Peak' is selected, clicking inside the display will set a marker at the nearest 
peak. The exact frequency and level of the peak are found by interpolation, so the
frequency can be much more accurate than the FFT step, and the level corresponds to
the true peak value regardless of display or analyser resolution.

When 'Noise' is selected, clicking inside the display will set a noise marker.
The noise density (energy per Hz) is calculated and displayed.

