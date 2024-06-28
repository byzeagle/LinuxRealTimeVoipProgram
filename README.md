# LinuxRealTimeVoipApp
(Almost) Real-time Voip Program with ALSA for Linux platforms.

server.c file is UDP-Hole-Punching Server.

You need to compile it separately. Makefile doesn't include it.

This program is a simplified version of commandline program **parole**.

Link: http://holdenc.altervista.org/parole/index.html

The program uses the same algorithm, sound library(alsa), system calls(poll, linux sockets), software parameters (related to sound) and sound encoding algorithm (opus) (I didn't know what software parameters to use for sound).

It is quite fast (used it to communicate overseas) and has low latency, really resource efficient and really small in size and it can be used as a base for many applications.

I will later build an untrackable terminal-based voip program with ncurses and alsa. I am not working on it right now. Graphical libraries can be used with the base program, too.

The program is not trackable since it is truly a P2P one and there is not a server that is gateway to communication. UDP-Hole-Punching mechanism is used for bypassing NAT.

Another advantage of this program is that it is single thread. It can work on platforms that do not support pthreads.

I used Linux sockets (UDP) to send data across, however, the RTP protocol can also be used with a library like oRTP (C library). This program doesn't use RTP protocol because I don't have a full understanding of the library.

The program constantly checks the network socket to see if there is any incoming data, if not, it reads microphone device with alsa and sends sound data across, and if there is any incoming data, it sends it to alsa for it to play. Speaker and microphone devices are constantly polled to see if there is any data available.

I had problems with properly using alsa library and therefore had to experiment with the program. Some of the alsa system calls are arbitrarly and experimentally placed in the program given the fact that it works.
