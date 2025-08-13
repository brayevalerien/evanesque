CC       ?= gcc
CFLAGS   ?= -O2 -Wall -Wextra -std=c99
CPPFLAGS ?=
LDFLAGS  ?=
LDLIBS   ?=

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

TARGET := evanesque
SRC    := src/evanesque.c
OBJ    := $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJ) $(LDLIBS)

src/%.o: src/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 0755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)

clean:
	rm -f $(TARGET) $(OBJ)

.PHONY: all install uninstall clean
