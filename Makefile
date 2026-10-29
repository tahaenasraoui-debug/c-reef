CC      = gcc
CFLAGS  = -std=c99 -O2 -Wall -Wextra
LDFLAGS = -lm
TARGET  = c-reef

all: $(TARGET)

$(TARGET): aquarium.c
	$(CC) $(CFLAGS) aquarium.c -o $(TARGET) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
