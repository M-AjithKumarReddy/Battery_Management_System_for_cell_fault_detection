# EV Battery Management System (BMS) Makefile
# Targets:
#   - Raspberry Pi 4 / 5 (aarch64) under QNX SDP 8.0 / Momentics IDE: 'make' or 'make rpi4'
#   - QNX x86_64 target (VM/PC): 'make x86_64'
#   - Local Laptop Host build: 'make host'

ifeq ($(origin CC),default)
CC = qcc -Vgcc_ntoaarch64le
endif
CC ?= qcc -Vgcc_ntoaarch64le
HOST_CC  ?= gcc
CFLAGS   ?= -O2 -Wall -Wextra -std=gnu11
OUT      = build
BINS     = $(OUT)/bms_sensor $(OUT)/bms_core $(OUT)/bms_web $(OUT)/bms_cli $(OUT)/test_gpio

all: $(BINS)
rpi4: all
rpi5: all

$(OUT):
	mkdir -p $(OUT)

$(OUT)/%: %.c bms.h | $(OUT)
	$(CC) $(CFLAGS) $< -o $@ -lsocket -lm 2>/dev/null || $(CC) $(CFLAGS) $< -o $@ -lm

# Build for QNX x86_64 architecture
x86_64:
	$(MAKE) CC="qcc -Vgcc_ntox86_64" all

# Build CLI and Web tools for local laptop terminal (Mac/Linux/Windows)
host: | $(OUT)
	$(HOST_CC) $(CFLAGS) bms_cli.c -o $(OUT)/bms_cli -lm
	$(HOST_CC) $(CFLAGS) bms_web.c -o $(OUT)/bms_web -lm
	@echo "Host binaries successfully built in $(OUT)/: bms_cli, bms_web"

clean:
	-rm -rf $(OUT)

.PHONY: all rpi5 x86_64 host clean
