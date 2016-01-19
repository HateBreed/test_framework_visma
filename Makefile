PREFIX=src
SOURCES=$(PREFIX)/main.c $(PREFIX)/utils.c $(PREFIX)/jsonutils.c $(PREFIX)/jsonhandler.c $(PREFIX)/connectionutils.c $(PREFIX)/tests.c
COMPILER=gcc
COPTS=-Wall --std=gnu99 -g
LIBS=`pkg-config --cflags --libs glib-2.0 json-glib-1.0`
BINARY=main

compile:
	$(COMPILER) $(COPTS) $(LIBS) $(SOURCES) -o $(BINARY)

run:
	./$(BINARY)
	
leaktest:
	G_DEBUG=gc-friendly G_SLICE=alwqays-malloc valgrind --leak-check=full ./$(BINARY)

all:
	compile
