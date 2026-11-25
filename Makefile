CC      = gcc
CFLAGS  = -std=c99 -O2 -Wall -Wextra -Wpedantic
LDFLAGS = -lm
TARGET  = c-reef
PREFIX  = /usr/local

.PHONY: all run debug test test-asan install uninstall clean

all: $(TARGET)

$(TARGET): aquarium.c
	$(CC) $(CFLAGS) aquarium.c -o $(TARGET) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

# Sanitizer build for development and CI
debug: aquarium.c
	$(CC) -std=c99 -O1 -g -fno-omit-frame-pointer -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined aquarium.c -o $(TARGET)-asan $(LDFLAGS)

test: $(TARGET)
	./tests/test_cli.sh ./$(TARGET)

test-asan: debug
	./tests/test_cli.sh ./$(TARGET)-asan

install: $(TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(TARGET)

clean:
	rm -f $(TARGET) $(TARGET)-asan
