**Project to host native drivers for the Aurora usb audio controller.**

# Status #

## Mixer Hardware ##
See http://www.auroramixer.com.

## OS X ##
Driver may or may not work. This was developed with a working knowledge of the FTDI serial chip and the MIDI framework for OS X. I (Joe the OS X dev) never had a unit in possession so this driver is untested.

Mainly, if the serial converter does not fragment the packets when sending down the USB channel, the driver should work just fine. If there is fragmentation, well, it won't work correctly at all.

If fragmentation is the case, an error checker and packet buffer will have to be implemented in the recievedData method. This is trivial bcs the Aurora only send one kind of MIDI packet of fixed size. If it is ever upgraded to send SYSEX data or use MIDI Running Status messages, then it because a much more difficult problem, because of variable packet lengths and also message interruption (SYSEX's of 'Realtime' are allowed to interrupt other messages).

| | OS X | Windows | Linux |
|:|:-----|:--------|:------|
| Author | Y | Y | ? |
| Status | Untested | Untested | ? |



---

This project is not associated with the developers of the Aurora Mixer hardware and solely those whom are authors of the software included on this page.