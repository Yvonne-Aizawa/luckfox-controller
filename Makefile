# Makefile for Switch Controller Emulator

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=gnu11
LDFLAGS = -lpthread

TARGETS = switch_controller switch_controller_api client_example

all: $(TARGETS)

switch_controller: switch_controller.c switch_descriptors.h
	$(CC) $(CFLAGS) -o $@ switch_controller.c $(LDFLAGS)

switch_controller_api: switch_controller_api.c switch_descriptors.h
	$(CC) $(CFLAGS) -o $@ switch_controller_api.c $(LDFLAGS)

client_example: client_example.c
	$(CC) $(CFLAGS) -o $@ client_example.c

clean:
	rm -f $(TARGETS)

install: all
	@echo "Installing to /usr/local/bin..."
	@install -m 755 switch_controller /usr/local/bin/
	@install -m 755 switch_controller_api /usr/local/bin/
	@install -m 755 client_example /usr/local/bin/
	@echo "Installation complete"

uninstall:
	@echo "Removing from /usr/local/bin..."
	@rm -f /usr/local/bin/switch_controller
	@rm -f /usr/local/bin/switch_controller_api
	@rm -f /usr/local/bin/client_example
	@echo "Uninstallation complete"

.PHONY: all clean install uninstall
