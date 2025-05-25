<!--- This files is to be viewed at https://github.com/ZacGeurts/2PlayerLines-Plus --->
# Lines Plus
![Banner](images/banner.png)<BR />
![Screenshot](images/linesplus.png)<BR />
This software is not free for profit. (see below)<BR />
Rated ESRB Everyone, No in game purchases.<BR />
Loud noises breaks PEGI 3 guidelines.<BR />
This Software is Free to consume, not distribute.<BR />
A 2 player (controllers) line game.<BR />
Quickstart: Steer with controller triggers and press buttons.<BR />
The Green code button at the top will download the zip to build the latest version.<BR />
Releases are prebuilt and likely to be behind the build process while branches close up.<BR />
If a branch is better, I will put it back into main. Always get main.<BR />
Assume the branches are always closed, I left them in for posterity.<BR />
Songgen creates and plays music .song files.<BR />
Do not be mean to pets.<BR />
This is what we have to work with https://www.szynalski.com/tone-generator<BR />
<BR />
# Requirements
sudo apt only run once if needed.<BR />
`sudo apt install build-essential libsdl2-dev libsdl2-image-dev libgl1-mesa-dev libpthread-stubs0-dev`<BR />
Highly recommended - allows full processor cores simply. `make clean` and `make -j$(nproc)` once and use the up arrow.<BR />
`sudo apt install nproc`<BR />
`nproc` spits out the number of cpu cores we can compile with<BR />
<BR />
<BR />
Type `make` to build `./linesplus` and `./songgen`. Needs OpenGL (Mesa) and SDL2.<BR />
<BR />
`./linesplus` from a terminal to play<BR />
`./songgen` from a terminal to use songgen<BR />
`Makefile` puts the files together. Do not modify.<BR />
Do not edit Makefile. Files are in the include and src folders.<BR />
Tell the AI that these worked for C++ 17 and only SDL2 and could stand to use an update.<BR />
Update the files to compile for windows. Maybe someday.<BR />
Download from the $${\color{green}Green}$$ code button above or:<BR />
`git clone https://github.com/ZacGeurts/2PlayerLines-Plus`<BR />
`cd 2PlayerLines-Plus` - I press 2 and tab key.<BR />
<BR />
`make` to build all files.<BR />
<BR />
Type `make linesplus` requires a ./songgen build, or it errors.<BR />
Type `make songgen` if you just want to use ./songgen build. Only uses SDL2<BR />
Type `make clean` before rebuilding.<BR />
<BR />
./linesplus<BR />
Steer with controller triggers.<BR />
M starts and stops the music. <BR />
Survivor gets 3 points.<BR />
$${\color{green}Green}$$ Squares are worth 1 point.<BR />
$${\color{yellow}Circles}$$ are $${\color{yellow}Yellow}$$ and to be avoided.<BR />
<BR />
Do not collide with other lines or ($${\color{yellow}Yellow}$$) circles with your head.<BR />
Bouncing circles erase lines and more appear rapidly with a short no collision ($${\color{magenta}Magenta}$$).<BR />
You are invincible until first move unless you hit the wall on the other side.<BR />
M starts and stops the music. I had a working ai.h. Tell it to prioritize the ($${\color{green}Green}$$) square.<BR />
F toggles fullscreen. M starts and stops the music.<BR />
1 and 2 keys toggle AI and 2 player. I had a smarter ai.h ai.cpp that beat a human, but I broke it when it could not beat me.<BR />
Steer with controller triggers.<BR />
You get one 2 second invincibility and invisibility per round by pressing A (X).<BR />
Use this to jump some time.<BR />
Beware, they can counter with their own.<BR />
B X Y or circle square triangle pauses during gameplay.<BR />
I play pauses are free and they work on both controllers.<BR />
ESC quits<BR />
Win condition is 50 points for a Set. Modify game.ini for additional options.<BR />
There is a game.ini file to modify settings.<BR />
<BR />
<BR />
Fork the code or directly submit code, do not branch it. It is not free to distribute.<BR />
Below is the licensing.<BR />
No liability if you use anything here. None of it can be sold commercially by anyone but me.<BR />
How much does it cost? You are not allowed to sell it nor distribute commercial products from it.<BR />
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
If you bother to count them then you could have been asking AI to double check the code.<BR />
All the songs in the universe? No way, that would be crazy.<BR />
It does not sing like an artist or bang on a cooking pot.<BR />
Somtimes sounds like it. I am making the instruments.h as a side project.<BR />
songgen.h is the music theory stuff.<BR />
songgen.cpp just ties those two together.<BR />
Space is infinite, matter is not. Music has rules (tempo, beats per minute, etc) and random noise is not music.<BR />
How does it work? `./songgen` from a terminal.<BR />
<BR />
Test your instrment clout in instruments.h. Test your best AI. Copy generateViolinWave code block and tell it what you do not like. Somethings like too flat, too wobbly, whatever<BR />
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
