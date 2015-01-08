APP=pdf2laser

CC=gcc
RM=rm
INSTALL=install

CFLAGS=-std=c99 -Wall -Wextra -pedantic -O3
LDFLAGS=-lm

prefix=/usr/local
exec_prefix=$(prefix)
bindir=$(exec_prefix)/bin
srcdir=src

.PHONY: all clean install

all: $(APP)

clean:
	$(RM) -f $(APP)

install: all
	$(INSTALL) -d -m 755 '$(DESTDIR)$(bindir)'
	$(INSTALL) $(APP) '$(DESTDIR)$(bindir)'

$(APP): $(srcdir)/$(APP).c
	$(CC) $(CFLAGS) $(LDFLAGS) $(<) -o $(@)
