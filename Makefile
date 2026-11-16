ifneq ($(KERNELRELEASE),)
# kbuild part of makefile
obj-m := hello.o
ccflags-y += -g # Add DWARF debugging information
else
# normal makefile
KDIR ?= /home/bohdana/repos/linux-stable

default:
	$(MAKE) -C $(KDIR) M=$$PWD
	cp hello.ko hello.ko.unstripped
	$(CROSS_COMPILE)strip -g hello.ko # Strip debugging info from module binary

clean:
	$(MAKE) -C $(KDIR) M=$$PWD clean
	rm -f hello.ko.unstripped

%.s %.i: %.c # Targets for generating asm or preprocessed files directly
	$(MAKE) -C $(KDIR) M=$$PWD $@
endif
