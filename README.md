# Lines Plus
![Banner](images/banner.png)<BR />
![Screenshot](images/linesplus.png)<BR />
A 2 player (controllers) line game.<BR />
Songgen is interesting on its own.<BR />
Songview I uploaded on accident and can be ignored. Might make a editor for the .song files.<BR />
<BR />
M starts and stops the music.
Survivor gets 3 points.<BR />
Squares are worth 1 point.<BR />
Do not collide with other lines or (yellow) circles with your head.<BR />
Bouncing circles erase lines and another appears every 5 seconds with a short no collision (magenta).<BR />
You are invincible until first move unless you hit the wall on the other side.<BR />
M starts and stops the music. I had a working ai.h. Tell it to prioritize the green square.<BR />
F toggles fullscreen. M starts and stops the music.<BR />
1 and 2 will eventually toggle AI and 2 player. I had a smarter AI that beat a human, but I broke it when it could not beat me.<BR />
Steer with controller triggers.<BR />
You get one 2 second invincibility and invisibility per round by pressing A (X).<BR />
Use this to test your opponent memory and time a trap.<BR />
Beware, they can counter with their own.<BR />
B X Y or circle square triangle pauses during gameplay.<BR />
Q or ESC quits<BR />
Win condition is 50 points for a Set. Modify game.ini for additional options.<BR />
There is a game.ini to modify settings.<BR />
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
Why is this not free? Because Lines Plus is 3% of the product. The real meat is the songgen that pretty much makes more songs than grains of sand in the universe.<BR />
All the songs in the universe? No way, that would be crazy. It does not sing like an artist or bang on a cooking pot.<BR />
Somtimes sounds like it. I am making the instruments.h as a side project.<BR />
It is difficult to add new instruments accurately. Many are progressing.<BR />
The generator is under somewhat constant update while I iron out code.<BR />
The goal is to have a generator that is more or less hardcoded for modification rather than the ghost of AI just spitting out music.<BR />
How does it work? ./songgen<BR />
Non-linesplus files - songgen.h - makes structured songs (WIP). songgen.cpp reads .song format and plays using the instruments file. instruments.h intruments.h.<BR />
<BR />TODO: Would benefit greatly with compression (500KB to 50KB per .song)).<BR /><BR />
Type `make` to build linesplus and songgen. Needs OpenGL (Mesa)) and SDL2.<BR />
Type `make songgen` if you just want to see songgen build. Needs OpenGL (Mesa) and SDL2.<BR />
Type `make clean` before rebuilding.<BR />
`./linesplus`<BR /><BR />
The new songview might cause build issues due to additional dependancies.<BR />
`make linesplus`<BR />
`make songgen`<BR />
Build them seperate (like above) if you experience build issues.<BR />
`make songview` builds, but currently has a broken display.<BR />
If you still have issues try a makefile from before May 19, 2025.
