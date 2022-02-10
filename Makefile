obj-m += oled.o

KDIR = /lib/modules/$(shell uname -r)/build

all: font.h
	make -C $(KDIR) M=$(shell pwd) modules

clean:
	make -C $(KDIR) M=$(shell pwd) clean

