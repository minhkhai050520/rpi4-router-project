CFLAGS ?= -Wall -O2
CFLAGS += $(shell pkg-config --cflags libnl-3.0 libnl-genl-3.0)
LDFLAGS += $(shell pkg-config --libs libnl-3.0 libnl-genl-3.0)

all: myfw-ctl

myfw-ctl: myfw_ctl.c
	$(CC) $(CFLAGS) -o myfw-ctl myfw_ctl.c $(LDFLAGS)

clean:
	rm -f myfw-ctl