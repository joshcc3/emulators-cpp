Game Boy Advance Emulator
====

Step 1
===
Develop a Chip-8 Emulatopr
https://tobiasvl.github.io/blog/write-a-chip-8-emulator/
chip8 roms: https://github.com/loktar00/chip8/blob/master/roms/IBM%20Logo.ch8
For graphics we are using: https://github.com/SFML/SFML

Debugging Sound: ALSOFT_LOGLEVEL=3 ALSOFT_LOGFILE=/tmp/alsoft.log

https://www.copetti.org/writings/consoles/game-boy-advance/

https://emudev.org/system_resources

Step 2
===

Substep 1:
Get the code in the romBank0 to print out the nintendo logo.

Game Boy/Colour emulator:
https://emudev.de/gameboy-emulator/overview/
excellent talk: https://www.youtube.com/watch?v=HyzD8pNlpwI

https://github.com/jsmolka/eggvance

https://emudev.de/gameboy-emulator/overview/
https://www.coranac.com/tonc/text/hardware.htm


https://github.com/gbadev-org/awesome-gbadev#emulator-development



https://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware


https://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware


https://rgbds.gbdev.io/docs/v0.4.2/gbz80.7/
https://gekkio.fi/files/gb-docs/gbctr.pdf

https://ez80.readthedocs.io/en/latest/docs/bit-shifts/rla.html

https://raw.githubusercontent.com/lmmendes/game-boy-opcodes/master/opcodes.json

http://www.csounds.com/manual/html/RealTimeLinux.html

https://www.alsa-project.org/wiki/File:Alsa_bps_formula.png
https://www.linuxjournal.com/article/6735


Bugs
----
when serializing the devices need to make sure that the granularity of clock increments is fine enough.
Updated entire vblank and then fast forwarded cpu, but it relies on a wait of 480 clocks to 
fast forward past the end of a display line which is 456 clocks.

Some docs refer to 1Mhz whereas others 4Mhz (all clocks are div by 4 as a memory load takes 4 clocks *shrug*)

ALSA has a setting latency which needs to be set appropriately for real time audio.
You want the audio to be responsive which means the clock and flush rate have to actually match in timing.
If the clock is too fast then you enqueue too much sound so that when a new sound needs to be played
it takes a while. If the flush rate is too slow then the sound gets broken up and there are clear artifacts
and beeps.

Questions
---
whats an io register


https://gbdev.io/pandocs/Memory_Map.html


GBA Stuff:


Goal - to play game boy ruby/sapphire via C++.

Some good answers here:
https://www.reddit.com/r/EmuDev/comments/cfy60z/resources_to_build_a_gba_emulator/

https://media.ccc.de/v/rustfest-rome-3-gameboy-emulator
http://pastraiser.com/cpu/gameboy/gameboy_opcodes.html
https://www.youtube.com/watch?v=HyzD8pNlpwI&list=PL1BolmHM1v7-kWa8JtIRMnE8wYhQjVwZS&index=17&t=0s



Link to the technical sepcifictions: https://problemkaputt.de/gbatek.htm
