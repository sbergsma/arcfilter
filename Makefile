.SUFFIXES: .c .cpp

CC=g++
GO = -O2

CFLAGS = $(GO) -Wall
EXECS = ruleFilter linearFilter quadFilter

%.o:	%.cpp
	$(CC) -c -o $@ $(CFLAGS) $<

%.o:	%.c
	$(CC) -c -o $@ $(CFLAGS) $<

all: $(EXECS)

ruleFilter:	ruleFilter.o filterCommon.o
	$(CC) -o $@ $(CFLAGS) ruleFilter.o filterCommon.o

linearFilter:	linearFilter.o filterCommon.o
	$(CC) -o $@ $(CFLAGS) linearFilter.o filterCommon.o

quadFilter:	quadFilter.o filterCommon.o
	$(CC) -o $@ $(CFLAGS) quadFilter.o filterCommon.o

depend:
	$(CC) -MM $(CFLAGS) *.cpp >.dep

clean:
	rm -rf *.o core temp $(EXECS) *~

include .dep
