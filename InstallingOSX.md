# Installing #

Copy the AuroraUSBMIDIDriver.plugin file to /Library/Audio/Midi Drivers/


# Debugging #

Run CoreMIDIServer from the terminal by typing the following into a terminal window
```
/System/Library/Frameworks/CoreMIDIServer.framework/MIDIServer
```
MIDServer must not already be running. You can either
  * run `killall MidiServer` from the terminal
  * Close any applications that use MIDI
    * run `ps -A | grep MIDIServer` command to check if it's still running