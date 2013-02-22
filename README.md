DigiSpark LED USB Lamp
======================

Digispark USB Mood Lamp that Sleeps When the Computer Does.

I wanted an LED mood lamp that's only on when my computer is awake. The DigiSpark and its LED shield was perfect!

With a full USB connection, the host sents frame pulses to the devices. The Digispark counts these and when it sees they're not being updated it fades away until it sees frame pulses again.

usbconfig.h
===========

To get the Digispark version of the Arduino software to use the included usbconfig.h you'll have to replace the one that comes with the application (back up the file first).

/Applications/Digispark\ Ready\ -\ Arduino\ 1.03.app/Contents/Resources/Java/libraries/DigisparkUSB/usbconfig.h

Video
=====

See it in action:

http://www.youtube.com/watch?v=95n7XkzuqnQ
