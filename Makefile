#-----------------------------------
# Makefile für Oberon Loader + Wrapper
#-----------------------------------
CC = gcc
CFLAGS = -O2 -Wall -m32 -fPIC
LDFLAGS = -ldl -lm -lpthread

# Standardziele
all: oberon libobwrapper.so

#-----------------------------------
# Loader
#-----------------------------------
oberon: linux.oberon.o
	gcc -m32 -o oberon linux.oberon.o $(LDFLAGS)
	strip oberon

linux.oberon.o: linux.oberon.c
	gcc $(CFLAGS) -c linux.oberon.c -o linux.oberon.o

#-----------------------------------
# Wrapper
#-----------------------------------
obwrapper.o: obwrapper.c
	gcc $(CFLAGS) -c obwrapper.c -o obwrapper.o

libobwrapper.so: obwrapper.o
	gcc -m32 -shared -fPIC -o libobwrapper.so obwrapper.o -lpthread

#-----------------------------------
# Clean
#-----------------------------------
clean:
	rm -f oberon linux.oberon.o obwrapper.o libobwrapper.so

.PHONY: all clean
