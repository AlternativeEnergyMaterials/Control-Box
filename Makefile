DESTDIR?=/usr
PREFIX?=/local

ifneq ($V,1)
Q ?= @
endif

CC	= gcc
CFLAGS	= $(DEBUG) -Wall -Wextra $(INCLUDE) -Winline -pipe

LDFLAGS	= -L$(DESTDIR)$(PREFIX)/lib
# LIBS    = -lpthread -lrt -lm -lcrypt

SRC	=	src/main.c
OBJ	=	$(SRC:.c=.o)
all:	aem-cb

aem-cb:	$(OBJ)
	$Q echo [Link]
	$Q $(CC) -o $@ $(OBJ) $(LDFLAGS) $(LIBS)

.c.o:
	$Q echo [Compile] $<
	$Q $(CC) -c $(CFLAGS) $< -o $@

.PHONY:	clean
clean:
	$Q echo "[Clean]"
	$Q rm -f $(OBJ) aem-cb *~ core tags *.bak

.PHONY:	install
install: aem-cb
	$Q echo "[Install]"
	$Q cp aem-cb		$(DESTDIR)$(PREFIX)/bin

.PHONY:	uninstall
uninstall:
	$Q echo "[UnInstall]"
	$Q rm -f $(DESTDIR)$(PREFIX)/bin/aem-cb
	$Q rm -f $(DESTDIR)$(PREFIX)/man/man1/aem-cb.1
