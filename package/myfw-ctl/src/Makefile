EXTRA_CFLAGS ?= -Wall -O2
EXTRA_CFLAGS += -I$(STAGING_DIR)/usr/include/libnl3

LIBS = -L$(STAGING_DIR)/usr/lib -lnl-genl-3 -lnl-3

all: myfw-ctl

myfw-ctl: myfw_ctl.c
	$(CC) $(EXTRA_CFLAGS) myfw_ctl.c -o myfw-ctl $(LIBS)

clean:
	rm -f myfw-ctl