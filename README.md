# Lines Plus
A 2 player (controllers) line game.<BR />
<BR />
Survivor gets 3 points.<BR />
Squares are worth 1 point.<BR />
Do not collide with other lines or (yellow) circles with your head.<BR />
Bouncing circles erase lines and another appears every 5 seconds with a short no collision (magenta).<BR />
You are invincible until first move unless you hit the wall on the other side.<BR />
F toggles fullscreen<BR />
1-5 will eventually toggle AI difficulty. (0 to return to 2 Player).<BR />
Steer with controller triggers.<BR />
You get one short invincibility and invisibility per round by pressing A (X).<BR />
B X Y or circle square triangle pauses during gameplay.<BR />
Q or ESC quits<BR />
Win condition is 100 points for a Set.<BR />
There is a game.ini to modify settings. (Options coming soon(tm))<BR />
<BR />
Below is the licensing.<BR />
No liability if you use anything here. 
<BR />
This license updates and the software and all iterations are covered by the latest version.<BR />
This is not free software. If it is used for commercial use then it requires negotiable royalty (1%+).<BR />
This license covers this software across all iterations, including initial upload.<BR />
Negotiations are with the original orange. The guy typing this out.<BR />
This is not free software and requires royalties for commercial use.<BR />
If this helps you make money on your project, think of me.<BR />
If you make a free project, enjoy.<BR />
Royalties are required for songgen.cpp songgen.h and instruments.h instruments.dat<BR />
The other linesplus code is free and cannot be resold.<BR />
Interested parties can find my contact information at https://github.com/ZacGeurts<BR />
<BR />
Why is this not free? Because Lines Plus is 3% of the product. The real meat is the songgen that pretty much makes more songs than grains of sand in the universe.<BR />
All the songs in the universe? No way, that would be crazy. It does not sing like an artist or bang on a cooking pot.<BR />
Somtimes sounds like it. I am making the instruments.h as a side project.<BR />
It is difficult to add new instruments accurately. Many are left undone.
It will want to be asked with the range of frequencies like A subwoofer should be limited to 20-80hz. (Grok3 was tested)
How does it work? ./songgen<BR />
Non-lines files - songgen.h - makes songs. songgen.cpp reads song format. instruments.h intruments.h.
Note: linesplus previous code works more stably<BR />
<BR /><BR />
Type `make` to build. Needs OpenGL (Mesa) and SDL2.<BR />
Type `make songgen` if you just want to see songgen build. Needs OpenGL (Mesa) and SDL2.<BR />
Type `make clean` before rebuilding.<BR />
