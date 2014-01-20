SOURCES := luasynth.c
OBJECTS := $(patsubst %.c,%.o,$(SOURCES))

CFLAGS := -Wall -Wextra -std=c99
CFLAGS += -O3 -fomit-frame-pointer -pipe

.PHONY: clean

luasynth: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -lasound -lluajit-5.1 -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm luasynth $(OBJECTS)
