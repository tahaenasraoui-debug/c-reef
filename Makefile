CC      = gcc
CFLAGS  = -std=c99 -O2 -Wall -Wextra -Wpedantic
LDFLAGS = -lm
TARGET  = c-reef
PREFIX  = /usr/local

.PHONY: all run debug install uninstall clean

all: $(TARGET)

$(TARGET): aquarium.c
	$(CC) $(CFLAGS) aquarium.c -o $(TARGET) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

# Sanitizer build for development and CI
debug: aquarium.c
	$(CC) -std=c99 -O1 -g -Wall -Wextra -Wpedantic -fsanitize=address,undefined aquarium.c -o $(TARGET)-asan $(LDFLAGS)

install: $(TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(TARGET)

clean:
	rm -f $(TARGET) $(TARGET)-asan
