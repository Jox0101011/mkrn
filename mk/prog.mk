# prog.mk

ifndef PROG
$(error PROG is not set, use e.g. 'make PROG=foo')
endif

.DEFAULT_GOAL := all

CC       ?= cc
INSTALL  ?= install
RM       ?= rm -f
MKDIR_P  ?= mkdir -p

PREFIX   ?= /usr/local
BINDIR   ?= $(PREFIX)/bin
MANDIR   ?= $(PREFIX)/share/man
MANSECTION ?= 1

CPPFLAGS ?=
CFLAGS   ?= -Os -pipe
LDFLAGS  ?=
LDLIBS   ?=
LDADD    ?=

SRCS     ?= $(PROG).c

OBJS     := $(SRCS:.c=.o)
DEPS     := $(OBJS:.o=.d)

BUILD ?= release

ifeq ($(BUILD),debug)
CFLAGS += -O0 -g3
else ifeq ($(BUILD),release)
CFLAGS += -DNDEBUG
endif

ifneq ($(strip $(SANITIZE)),)
CFLAGS  += -fsanitize=$(SANITIZE)
LDFLAGS += -fsanitize=$(SANITIZE)
endif


.PHONY: all clean install install-bin install-man install-strip \
        uninstall help

all: $(PROG)


%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@


$(PROG): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) $(LDADD) -o $@


-include $(DEPS)


clean:
	$(RM) $(PROG) $(OBJS) $(DEPS)


install: install-bin install-man

install-bin: all
	$(MKDIR_P) "$(DESTDIR)$(BINDIR)"
	$(INSTALL) -m 755 "$(PROG)" "$(DESTDIR)$(BINDIR)/$(PROG)"

install-strip: all
	$(MKDIR_P) "$(DESTDIR)$(BINDIR)"
	$(INSTALL) -s -m 755 "$(PROG)" "$(DESTDIR)$(BINDIR)/$(PROG)"


ifneq ($(strip $(MAN)),)

install-man:
	$(MKDIR_P) "$(DESTDIR)$(MANDIR)/man$(MANSECTION)"
	$(INSTALL) -m 644 "$(MAN)" \
		"$(DESTDIR)$(MANDIR)/man$(MANSECTION)/$$(basename $$(notdir $(MAN))).$(MANSECTION)"

uninstall:
	$(RM) "$(DESTDIR)$(BINDIR)/$(PROG)"
	$(RM) "$(DESTDIR)$(MANDIR)/man$(MANSECTION)/$$(basename $$(notdir $(MAN))).$(MANSECTION)"

else

install-man:
	@:

uninstall:
	$(RM) "$(DESTDIR)$(BINDIR)/$(PROG)"

endif
