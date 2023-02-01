# Makefile semplice

CC = gcc
CFLAGS   = $(DEFINES) $(OPTIMIZE) -std=gnu99 -pthread

PROGS    = dftpd

PROGS_O  = main.o

LIBS     = FtpCommands.c SocketAperti.c TabellaFile.c FtpThread.c
all:    objs progs

progs :
	$(CC) $(CFLAGS) $(LFLAGS) -o $(PROGS) $(PROGS_O) $(LIBS)

objs:   $(PROGS_O)

.c.o:
	$(CC) $(CFLAGS) -c -o $*.o $<

.c.s:
	$(CC) $(CFLAGS) -S -o $*.s $<

.o:
	$(CC) $(CFLAGS) $(LFLAGS) -o $* $(PROGS_O) $(LIBS)