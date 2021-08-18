CC = /mnt/c/wsl/avr8-gnu-toolchain-linux_x86_64/bin/avr-gcc
OBJCOPY = /mnt/c/wsl/avr8-gnu-toolchain-linux_x86_64/bin/avr-objcopy
SIZE = /mnt/c/wsl/avr8-gnu-toolchain-linux_x86_64/bin/avr-size
AVRDUDE = /mnt/c/programs/avrdude-6.3/avrdude.exe

LDFLAGS = -mmcu=atmega328p
CFLAGS = -c -O -mmcu=atmega328p -Wall
AVRDUDE_FLAGS = -P com13 -p m328p

OBJ = main.o
FLAGS = 

all: main.elf

flash:
	$(OBJCOPY) main.elf main.hex -O ihex
	$(AVRDUDE) $(AVRDUDE_FLAGS) -c arduino -U flash:w:main.hex

main.elf: $(OBJ)
	$(CC) -o main.elf $(LDFLAGS) $(OBJ)
	$(SIZE) main.elf

%.o: c/src/%.c #c/inc/*.h
	$(CC) $< -Ic/inc $(CFLAGS) $(FLAGS)

clean:
	rm -rf *.o main.hex
