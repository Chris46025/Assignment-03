obj-m = testfifo.o

KBUILD_CFLAGS += -w

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules $(KBUILD_CFLAGS)
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
