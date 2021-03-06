PREFIX=src
SOURCES=$(PREFIX)/main.c $(PREFIX)/utils.c $(PREFIX)/jsonutils.c $(PREFIX)/preferences.c $(PREFIX)/connectionutils.c $(PREFIX)/tests.c
COMPILER=gcc
COPTS=-Wall --std=gnu99
COPTSD=$(COPTS) -g -DG_MESSAGES_DEBUG=all
LIBS=`pkg-config --cflags --libs glib-2.0 json-glib-1.0` -lcurl
BINARY=testfw

compile:
	$(COMPILER) $(COPTS) $(LIBS) $(SOURCES) -o $(BINARY)
	
debug:
	$(COMPILER) $(COPTSD) $(LIBS) $(SOURCES) -o $(BINARY)

run:
	./$(BINARY) -u john.doe@severa.com
	
leaktest:
	G_DEBUG=resident-modules G_SLICE=debug-blocks valgrind --leak-check=full ./$(BINARY) -u john.doe@severa.com

all:
	compile
