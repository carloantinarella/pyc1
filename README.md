# PyC - Python and C communication example

This project is a simple example of a program consisting of two intercommunicating processes: a python process and a C process. The project itself is not very useful except that for learning purposes.

The C process (source code in pyc1.c), when started, launches two threads: the first one periodically reads a certain amount of raw bytes from /dev/urandom and copies them into a portion of previously initialized shared memory; the second thread opens a UNIX datagram socket and then keeps listening for messages from the python process.

The python process makes use of the Kivy library for GUI handling. After opening (in readonly mode) the shared memory area created by the C process, it transforms the raw bytes there present in pixels by dividing the byte array in groups of three (R, G, B), and builds a texture that is a graphical representation of the pixels' color. Pixel replication algorithm (yes, is very slow!) is used in order to have a better rendering and a better understanding of the functionality. That way the texture displays a matrix of random-colored big pixels (is very easy to change single pixel dimensions just by changing a variable in the python code). At startup, the python process connects to the UNIX datagram socket previously created by the C process, and sends a numeric value to the C process that represents the number of grid-samples read from /dev/urandom over which an average should be performed (the average is performed by the C process, and turns out to be fast). The number can be chosen during runtime by the user just by using the slider in the right part of the GUI.
The bigger the number of samples to average over, and the closer to grey color (128, 128, 128) look the pixels.
A "SAVE png" button allows to export the texture in png format.

Both processes terminate when an attempt to close the GUI is made. The python process sends the special value "999" to the C process and this calls the exit() system call.

## Why two processes?

The goal of this example is to demonstrate how easy can be to take the good part of two programming worlds:

* the easiness and rapidity of python for building all the GUI-related (and in general high-level-related) part of the program
* the robustness and real-time affinity of the C programming

Imagine if, instead of /dev/urandom, we need to read a low resolution thermal camera via serial port. The C part allows us to be consistent with speed and realtime (in case it is needed) performances that cannot be acheived with a standalone python program.

## Needed dependences

In order to compile and run this project, the requirements are:

* gcc (tested with version 7.4.0)
* python3 (I tested it with python 3.6.7, but previouos versions should work)
* kivy 1.10 or above (tested with 1.10.1)

## Compiling and running the application

Before trying to get fun with the application, you should open a terminal in the directory in which you downloaded the code, compile the C part of the project with
```
make
```
then launch the startup script with
```
./pyc1.sh
```

## License

This project is released under the GPL v2 licence.
