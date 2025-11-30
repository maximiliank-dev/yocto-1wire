obj-m += onewire_dev.o

#Define variable if it is built outside of yocto
# KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build

# all:
# 	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) modules

# clean:
# 	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) clean


#SRC := $(shell pwd)
SRC=$(PWD)

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC)

modules_install:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) modules_install

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) clean