#pragma once

#include <common.h>

#define	SIGHUP		1	/* Hangup.  */
#define	SIGINT		2	/* Interactive attention signal.  */
#define	SIGQUIT		3	/* Quit.  */
#define	SIGILL		4	/* Illegal instruction.  */
#define	SIGTRAP		5	/* Trace/breakpoint trap.  */
#define	SIGABRT		6	/* Abnormal termination.  */
#define SIGEMT      7   /* Emulation trap */
#define	SIGFPE		8	/* Erroneous arithmetic operation.  */
#define	SIGKILL		9	/* Killed.  */
#define SIGBUS      10  /* Bus error */
#define	SIGSEGV		11	/* Invalid access to storage.  */
#define SIGSYS      12  /* Bad system call */
#define	SIGPIPE		13	/* Broken pipe.  */
#define	SIGALRM		14	/* Alarm clock.  */
#define	SIGTERM		15	/* Termination request.  */

#define SIG_COUNT   15

typedef struct SIGNAL
{
    uint16_t num;
    uint64_t handler;
} signal_t;