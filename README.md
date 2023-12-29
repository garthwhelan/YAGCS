WIP - Yet Another GCode Sender

An alternative gcode sender for use with GRBL and FluidNC that follows my personal preferences more strongly than alternatives I have tried. Please take a look at gSender or UniversalGcodeSender if you have mistakely stumbled here first, they are each good. Written with Qt.

Features:
* Display obj of part in work coordinates (sanity check)

Building and running:

>qmake

>make

>./YAGCS

![alt text](https://github.com/garthwhelan/YAGCS/blob/main/example_pic.png?raw=true)

TODO:

* Connect to GRBL button
* Improved path preview
** Support for G2/G3
** Color coded by e.g. tools? feeds?
* Faster path rendering (Probably a way to do this without 3D lines)
* Better camera orbit in 3D preview
* Probing support
* Simulation using tool information from Fusion 360 comments
* Live display machine bounds and tool location on preview
* Live display path (already/not yet) taken seperately
* General fixes
