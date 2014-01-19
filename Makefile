dict:dict.c
	cc -g -o $@ $< `pkg-config --cflags --libs libxml-2.0` `pkg-config --cflags --libs gtk+-3.0`
