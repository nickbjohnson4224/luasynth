SOURCES := luasynth.c player.c
DEPS := state.h
OBJECTS := $(patsubst %.c,%.o,$(SOURCES))

CFLAGS := -Wall -Wextra -std=c99
CFLAGS += -O3 -fomit-frame-pointer -pipe
LDFLAGS += -lasound -lluajit-5.1 -lncurses -lpthread

.PHONY: clean

luasynth: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(LDFLAGS) -o $@

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $<

clean:
	rm luasynth $(OBJECTS)
