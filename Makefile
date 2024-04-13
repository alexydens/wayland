# Directories
BUILD_DIR=build
SRC_DIR=src

# Flags for compiler and linker
CFLAGS = -std=c99 -Wall -Wextra
LDFLAGS = -O3 -ffast-math -lwayland-client

# For XDG
XDG_ITEMS = $(SRC_DIR)/xdg-shell-protocol.c
XDG_ITEMS += $(SRC_DIR)/xdg-shell-client-protocol.h

# File targets
# Source file for XDG shell protocol
$(SRC_DIR)/xdg-shell-protocol.c:
	wayland-scanner private-code \
		/usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml $@
# Header file for XDG shell client protocol
$(SRC_DIR)/xdg-shell-client-protocol.h:
	wayland-scanner client-header \
		/usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml $@

# Main test binary
$(BUILD_DIR)/main: $(SRC_DIR)/main.c $(XDG_ITEMS)
	$(CC) $(CFLAGS) $(SRC_DIR)/* -o $@ $(LDFLAGS)

# Debug test binary
$(BUILD_DIR)/main_debug: $(SRC_DIR)/main.c $(XDG_ITEMS)
	$(CC) $(CFLAGS) $(SRC_DIR)/* -o $@ $(LDFLAGS) -ggdb

# Phony rules
.PHONY: clean test debug

# Clear build directory
clean:
	rm -rf $(BUILD_DIR)/*

# Run test binary
test: $(BUILD_DIR)/main
	$(BUILD_DIR)/main

# Build debug binary
debug: $(BUILD_DIR)/main_debug
