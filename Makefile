
CC = gcc

TESTS = $(wildcard tests/*.c)
OBJ = $(patsubst tests/%.c, obj/.tests/%.o, $(SRC))

SRC = $(wildcard src/*.c) $(wildcard src/query/*.c)
OBJ = $(patsubst src/%.c, obj/%.o, $(SRC))

LIBNAME = libwalrus
VALIDATOR = vwalrus

CFLAGS = -Wall -I include -I src/query/include
VALIDATOR_CFLAGS = $(CFLAGS) libwalrus

all: lib validator

validator: $(VALIDATOR)

$(VALIDATOR): main.c $(LIBNAME)
	$(CC) $< $(VALIDATOR_CFLAGS) -o $(VALIDATOR)

lib: $(LIBNAME)

$(LIBNAME): $(OBJ)
	ar rcs $@ $^

obj/%.o: src/%.c
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	rm -f obj/*.o obj/query/*.o $(LIBNAME)