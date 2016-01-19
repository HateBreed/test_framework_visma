PREFIX=src
SOURCES=$(PREFIX)/main.c $(PREFIX)/utils.c $(PREFIX)/jsonutils.c $(PREFIX)/jsonhandler.c
COMPILER=gcc
COPTS=-Wall --std=gnu99 
LIBS=`pkg-config --cflags --libs glib-2.0 json-glib-1.0`
BINARY=main

compile:
	$(COMPILER) $(COPTS) $(LIBS) $(SOURCES) -o $(BINARY)

run:
	./$(BINARY)

all:
	compile
