#include <stdio.h>
#include <stdlib.h>

/* this function keeps track of how long it took the system to do the i/o
 * the value is accumulated in two statics and is used in computing the
 * approximate CPU time required for a computation
 */

static unsigned long io_second=0;
static unsigned long io_micro=0;

static unsigned long time_s, time_m;
static unsigned long time2_s, time2_m;

IO_lock()
{
	CurrentTime(&time_s,&time_m);
}

IO_unlock()
{
	long diff;

	CurrentTime(&time2_s,&time2_m);

	diff = time2_m-time_m;

	if (diff < 0) {
		io_second--;
		diff += 1000000;
	}

	io_micro += diff;
	io_second += (time2_s-time_s);
	while (io_micro > 1000000) {
		io_micro -= 1000000;
		io_second++;
	}
}

extern _Startsec;
extern _Startmic;

unsigned long time(secs)
{
	unsigned long csec,cmics;

	/* fetch current system time */
	CurrentTime(&csec,&cmics);

	/* subtract all i/o time */
	csec  -= io_second;
	while (cmics < io_micro) {
		csec--;
		cmics += 1000000;
	}
	cmics -= io_micro;

	/* remove the offset from the start of the session */
	csec -= _Startsec;
	while (cmics < _Startmic) {
		csec--;
		cmics += 1000000;
	}
	cmics -= _Startmic;

	return(csec);
}
