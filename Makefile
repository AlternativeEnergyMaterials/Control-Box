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
all:	AEM-CB

AEM-CB:	$(OBJ)
	$Q echo [Link]
	$Q $(CC) -o $@ $(OBJ) $(LDFLAGS) $(LIBS)

.c.o:
	$Q echo [Compile] $<
	$Q $(CC) -c $(CFLAGS) $< -o $@

.PHONY:	clean
clean:
	$Q echo "[Clean]"
	$Q rm -f $(OBJ) AEM-CB *~ core tags *.bak

.PHONY:	install
install: AEM-CB
	$Q echo "[Install]"
	$Q cp AEM-CB		$(DESTDIR)$(PREFIX)/bin

.PHONY:	uninstall
uninstall:
	$Q echo "[UnInstall]"
	$Q rm -f $(DESTDIR)$(PREFIX)/bin/AEM-CB
	$Q rm -f $(DESTDIR)$(PREFIX)/man/man1/AEM-CB.1
