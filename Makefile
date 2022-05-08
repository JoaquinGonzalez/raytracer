VERSION = 0.0.1
CC = c99
PKG_CONFIG = pkg-config
SRC = raytracer.c
OBJ = $(SRC:.c=.o)
STCFLAGS = -O0 -g
STLDFLAGS = -lm

.c.o:
	$(CC) $(STCFLAGS) -c $<

raytracer: $(OBJ)
	$(CC) -o $@ $(OBJ) $(STLDFLAGS)

run:
	./raytracer > image.ppm

clean:
	rm -rf $(OBJ) raytracer
