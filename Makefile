CC=gcc
RM=rm

CFLAGS=-std=c99 -Wall -Wextra -pedantic -O3
LDFLAGS=

.PHONY: all clean

all: pdf2laser

clean:
	$(RM) -f pdf2laser ta10 live-laser cups-pdf2laser

pdf2laser: src/pdf2laser.c
	$(CC) \
		$(CFLAGS) \
		$(LDFLAGS) \
		-lm \
		-o $(@) \
		$(<)

INSTALL_LOCATION=/usr/libexec/cups/backend/pdf2laser

install: pdf2laser
	sudo cp $< $(INSTALL_LOCATION)
	sudo chown root:wheel $(INSTALL_LOCATION)
