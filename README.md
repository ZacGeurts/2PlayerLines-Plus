# Lines Plus
![Banner](images/banner.png)<BR />
![Screenshot](images/linesplus.png)<BR />
This software is not free for profit. (see below)<BR />
A 2 player (controllers) line game.<BR />
Quickstart: Steer with triggers and press buttons.<BR />
If you have Linux you can grab the zip from the releases on the top right.<BR />
Songgen creates and plays music .song files.<BR />
<BR />
Type `make` to build linesplus and songgen. Needs OpenGL (Mesa) and SDL2.<BR />
`./linesplus` or use icon to play<BR />
`./songgen` from a terminal to use songgen<BR />
`Makefile` has more information if you open it with a text editor.<BR />
<BR />
Type `make songgen` if you just want to use songgen build.<BR />
Type `make clean` before rebuilding.<BR />
<BR />
M starts and stops the music.
Survivor gets 3 points.<BR />
Squares are worth 1 point.<BR />
Do not collide with other lines or (<span style="color: yellow;">yellow</span>) circles with your head.<BR />
Bouncing circles erase lines and another appears every 5 seconds with a short no collision (<span style="color: magenta;">magenta</span>).<BR />
You are invincible until first move unless you hit the wall on the other side.<BR />
M starts and stops the music. I had a working ai.h. Tell it to prioritize the green square.<BR />
F toggles fullscreen. M starts and stops the music.<BR />
1 and 2 will eventually toggle AI and 2 player. I had a smarter AI that beat a human, but I broke it when it could not beat me.<BR />
Steer with controller triggers.<BR />
You get one 2 second invincibility and invisibility per round by pressing A (X).<BR />
Use this to test your opponent memory and time a trap.<BR />
Beware, they can counter with their own.<BR />
B X Y or circle square triangle pauses during gameplay.<BR />
ESC quits<BR />
Win condition is 50 points for a Set. Modify game.ini for additional options.<BR />
There is a game.ini file to modify settings.<BR />
<BR />
Below is the licensing.<BR />
No liability if you use anything here. None of it can be sold commercially by anyone but me.<BR />
How much does it cost? You are not allowed to sell it or distribute commercial products from it.<BR />
<BR />
This license updates and the software and all iterations are covered by the latest version.<BR />
This is not free software. If it is used for commercial use then it requires negotiable royalty (1%+).<BR />
This license covers this software across all iterations, including initial upload.<BR />
Negotiations are with the original orange. The guy typing this out.<BR />
This is not free software and requires royalties for commercial use.<BR />
If this helps you make money on your project, think of me.<BR />
If you make a free project, enjoy.<BR />
Royalties are required for songgen.cpp songgen.h and instruments.h instruments.dat (.dat was previous iterations)<BR />
The other linesplus code is free and cannot be resold.<BR />
Interested parties can find my contact information at https://github.com/ZacGeurts<BR />
<BR />
Songgen is the songgen that pretty much makes more songs than grains of sand in the universe.<BR />
All the songs in the universe? No way, that would be crazy. It does not sing like an artist or bang on a cooking pot.<BR />
Somtimes sounds like it. I am making the instruments.h as a side project.<BR />
Space is infinite, matter is not. Music has rules (tempo, beats per minute, etc) and random noise is not music.<BR />
How does it work? `./songgen` from a terminal.<BR />
<BR />
Test your instrment clout in instruments.h. Copy generateViolinWave code block and tell it what you do not like. Somethings like too flat, too wobbly, whatever<BR />
Paste the new code block back in (save) and `make clean` then `make` to have your new instrument ready to hear.<BR />
Change song1.song Instrument: to violin and `./songgen song1.song` to hear it.<BR />
<BR />
If you can make money with it, keep your bro in mind.<BR />
This is not free for commercial use, and if you update my base, it is subject to terms if you try to profit.<BR />
In short, you either pay me or you give it away for free.<BR />
If you give it away free, and someone makes money off it, they have to pay me.<BR />
If you make a free project, then be clear that it is using non free software.<BR />
<BR />
# Songgen files:
songgen.h - makes structured songs<BR />
songgen.cpp - reads .song format and plays using the instruments file<BR />
instruments.h is intruments.h.<BR />
