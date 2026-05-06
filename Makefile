CC := gcc
TARGET := gstreamer_basic
SRC := app/main.c
PKG_CONFIG := pkg-config
PKGS := gstreamer-1.0 glib-2.0
CFLAGS += -Wall -Wextra -g

PKG_CFLAGS = $(shell $(PKG_CONFIG) --cflags $(PKGS) 2>/dev/null)
PKG_LIBS = $(shell $(PKG_CONFIG) --libs $(PKGS) 2>/dev/null)

.PHONY: all clean run check-deps

all: check-deps $(TARGET)

check-deps:
	@$(PKG_CONFIG) --exists $(PKGS) || { \
		echo "Missing development packages for: $(PKGS)"; \
		echo "Install GStreamer/GLib development headers, then run 'make' again."; \
		exit 1; \
	}

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(PKG_CFLAGS) -o $@ $< $(PKG_LIBS)

clean:
	rm -f $(TARGET)
