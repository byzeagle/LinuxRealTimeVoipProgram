# LinuxRealTimeVoipApp
(Almost) Real-time Voip Program with ALSA for Linux platforms.

server.c file is UDP-Hole-Punching Server.

You've got to compile it separately.

Makefile doesn't include it.

This program is a simplified version of commandline program **parole**.

Link: http://holdenc.altervista.org/parole/index.html

The program uses the same algorithm, sound library(alsa) and system calls(poll) and software parameters (I didn't know what software parameters to use).

It is quite fast, really resource efficient and really small in size and it can be used as a base for many applications.

I will later build an untrackable terminal-based voip program with ncurses and alsa. I am not working on it right now.

The program is not trackable since it is truly a P2P one and there is not a server that is gateway to communication.

Another advantage of this program is that it is single thread. It can work on platforms that do not support pthreads.
