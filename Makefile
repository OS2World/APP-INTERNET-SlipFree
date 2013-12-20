#
# Makefile for SLIPWAIT, Mike Lempriere (mikel@networx.com) 26-Nov-94
#

# Use the following 3 lines for IBM CSET++...
CPP=icc
CFLAGS=-q -Wall-cls -W3
LDFLAGS=-q -B"/batch /pm:vio"

# Use the following 3 lines for Borland compilers...
# CPP=bcc
# CFLAGS=-wall+
# LDFLAGS=


.cpp.obj:
	$(CPP) -c $(CFLAGS) -Dos2 $*.cpp

.obj.exe:
	$(CPP) $(LDFLAGS) $*.obj


all:	SlipFree.exe

SlipFree.obj: SlipFree.cpp


clean:
	-del *.exe *.obj *.map
