/*
** SlipFree.cpp
**
** Requests access to the com port from SLIP.
**
** Mike Lempriere <mikel@networx.com> 22-Nov-94
**
** Last mod 29-Nov-94
**
** This program would not have been possible without the help of
** the original IBM TCP/IP SLIP author, David Bolen <db3l@ans.net>.
*/

/*
	  - - - - - - - - - - - - - - - - - - - - - - - - -

SLIP Utility Program Access to IBM OS/2 Slip Driver v1.0  -- David Bolen

The following should be done by the program wanting to tell the SLIP
driver it would like access to the COM port:

     Initialization
	- Check if the SLIP driver is running by opening the system
	  semaphore \sem32\slip\monitor.
	- Open the following three system semaphores:
		\sem32\slip\pause		(com_pause)
		\sem32\slip\paused		(com_paused)
		\sem32\slip\continue		(com_continue)

     COM Port Request
	- Post com_pause
	- Wait for com_paused

     --- Access COM port ---

     COM Port Release
	- Post com_continue


The above should not be considered a "published" procedure or API -
but at the same time, it's what is part of v1.0.  It will likely
change in subsequent versions when the SLIP driver starts supporting
multiple interfaces, and provides a more extensive functional
interface for external utilities.

Once the SLIP driver posts com_paused, although the driver still has a
shared open on the port, it will not issue any I/O requests to the
port until the com_continue semaphore is posted.  Thus, any other
utility (including the one that requested the pause as is the case
with SLIPTERM) can access the port as long as they issue the DosOpen
in sharing mode (OPEN_SHARE_DENYNONE).

	  - - - - - - - - - - - - - - - - - - - - - - - - -
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INCL_DOS
#define INCL_DOSDEVIOCTL
#include <os2.h>

const char * Version = "ver 1.0, 29-Nov-94";

// This stupid thing is because Borland's BSEDOS.H is not
// properly CONST'ed, so we must cast away the CONST'ness.
#ifdef __BORLANDC__
#define BORCAST (PSZ)
#else
#define BORCAST
#endif

static int Debug = 0;
static short Timeout = 0;
static char Command[1024];

static HEV com_pause;
static HEV com_paused;
static HEV com_continue;
static HEV slip_running;


static void Usage(void) {
    fprintf(stderr, "SlipFree %s - Mike Lempriere (mikel@networx.com)\n", Version);
    fprintf(stderr, "\nAsks SLIP for COM port access, starts given program,\n");
    fprintf(stderr, "and returns COM port control back to SLIP when that program completes.\n");
    fprintf(stderr, "\nUsage: SLIPFREE [switches] command [command args]\n");
    fprintf(stderr, "SWITCHES:\n");
    fprintf(stderr, "-?, -h\tThis help screen.\n");
    fprintf(stderr, "-w#\tWait up to '#' seconds for SLIP to start.\n");
    fprintf(stderr, "\n'command' will be executed as a command line,\n");
    fprintf(stderr, "with any remaining args passed as it's args.\n");
    fprintf(stderr, "\nEXAMPLES:\n");
    fprintf(stderr, "    SLIPFREE commn\n");
    fprintf(stderr, "        runs 'commn' without your having to shutdown or restart slip.\n");
    exit(1);
}						// Usage()


static void WaitForSlip(void) {

    int FirstTime = 1;
    APIRET rc;
    short Seconds = Timeout;
    do {
        rc = DosOpenEventSem("\\sem32\\slip\\monitor", &slip_running);
        if (rc) {
            if (FirstTime) {
                printf("[ Waiting for SLIP - maximum wait = %d seconds ]\n",
                       Timeout);
                FirstTime = 0;
            }
            Seconds--;
            if (Seconds >= 0) {
                if (Debug) {
                    fprintf(stderr, ".");
                    fflush(stderr);
                }
                DosSleep(1000L);
            }
        }
    } while (rc && (Seconds >= 0));
    if (Debug) {
        fprintf(stderr, "\n");
    }
    if (Seconds < 0) {
        printf("The SLIP driver appears not to have started.\n");
        exit(1);
    }
}						// WaitForSlip()


static void OpenASem(const char *Name, HEV * Hev) {

    APIRET rc = DosOpenEventSem(BORCAST Name, Hev);
    if (rc != 0) {
        fprintf(stderr, "Error %lu accessing %s - is SLIP up?\n", rc, Name);
        exit(1);
    }
}						// OpenASem()


static void ParseArgs(int argc, const char * const * argv) {

    int MoreSwitches;
    for (int argi = 1; argi < argc; argi++) {
        MoreSwitches = (argv[argi][0] == '-');
        if (MoreSwitches) {
            char Ch = (argv[argi][1]);
            switch (Ch) {
            case 'd':
                Debug = (argv[argi][2] - '0');
                if (Debug < 1) {
                    Debug = TRUE;
                }
                break;
            case 'w':
                Timeout = (short) atoi(&argv[argi][2]);
                if (Timeout < 1) {
                    Timeout = 30;
                }
                break;
            case '?':
            case 'h':
                Usage();
                break;
            default:
                fprintf(stderr, "Unrecognized command line switch %d\n", Ch);
                Usage();
            }
        }
        else {
            if (*Command) {
                strcat(Command, " ");
            }
            strcat(Command, argv[argi]);
        }
    }
}						// ParseArgs()


int main(int argc, const char * const * argv) {

    ParseArgs(argc, argv);

    if (!(*Command)) {
        Usage();
    }

    if (Timeout) {
        WaitForSlip();
    }

    OpenASem("\\sem32\\slip\\com\\pause", &com_pause);
    OpenASem("\\sem32\\slip\\com\\paused", &com_paused);
    OpenASem("\\sem32\\slip\\com\\continue", &com_continue);

    printf("[ Requesting COM port access from SLIP driver ]\n");
    DosPostEventSem(com_pause);

    // Wait for SLIP to let us have it.
    DosWaitEventSem(com_paused, SEM_INDEFINITE_WAIT);

    // Let them know they got it.
    printf("[ SLIP driver granted access ]\n");

    // Perform the command.
    printf("starting %s\n", Command);
    int Err = system(Command);
    if (Err != 0) {
        fprintf(stderr, "system err=%d\n", Err);
    }

    // And lastly - notify SLIP driver we are done.
    DosPostEventSem(com_continue);
    return 0;
}						// main()
