INTDIR          = ./bin/obj
OUTDIR          = ./bin

PROJ_NAME       = isdecomp
TARGET_BIN      = $(OUTDIR)/$(PROJ_NAME)

TARGET_OBJS     = \
	$(INTDIR)/main.o    \
	$(INTDIR)/isdlib.o  \
	$(INTDIR)/blast.o

LINK            = gcc
CFLAGS          = -c -O2 -Wall
LINKFLAGS       =


all: $(OUTDIR) $(INTDIR) $(TARGET_BIN)

clean: cleanobj
	-rm $(TARGET_BIN)
	-rm -r $(INTDIR)
	-rm -r $(OUTDIR)

cleanobj:
	-rm -f $(TARGET_OBJS)

$(INTDIR):
	test -d $(INTDIR) || mkdir $(INTDIR)

$(OUTDIR):
	test -d $(OUTDIR) || mkdir $(OUTDIR)

$(TARGET_BIN) : $(TARGET_OBJS)
	$(LINK) -o $@ $(LINKFLAGS) $(LINK_LIBS) $(TARGET_OBJS)

$(INTDIR)/%.o: %.c
	$(CC) $(CFLAGS) -o $@ $<

main.c: isdecomp.h blast.h
isdlib.c: isdecomp.h blast.h
blast.c: blast.h
