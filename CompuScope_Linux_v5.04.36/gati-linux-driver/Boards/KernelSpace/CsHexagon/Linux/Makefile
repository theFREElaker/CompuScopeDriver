DEBUG_ON:= n
MODNAME := CsE16bcd
TARGET  := $(MODNAME).ko
KERNVER  = $(shell uname -r)
KDIR    := /lib/modules/$(KERNVER)/build
INSTDIR := /lib/modules/$(KERNVER)/extra
SRCFILES:= ../../KS_Common/Linux/DriverEntry.c \
				../../KS_Common/Linux/GageWrapper.c \
				../../KS_Common/Linux/GageMisc.c \
				../../KS_Common/Linux/GageDma.c \
				../CsHexagonDrv.c \
				../CsHexagonFlashObj.c \
				HexagonGageDrv.c \
				HexagonNiosApi.c \
				HexagonAlteraSupport.c \
				CsHexagonFlash.c \
				CsHexagonLinux.c

ifeq ($(DEBUG_ON),y)
#MYFLAGS = -Wall -O -g -DXD48_DEBUG
MYHIDE=
MYFLAGS=-Wall -O -g -D_linux_ -DGEN_LINUX -DGEN_KERNEL_MODE -D_HEXAGON_DRV_
else
MYHIDE=@
MYFLAGS=-Wall -O2   -D_linux_ -DGEN_LINUX -DGEN_KERNEL_MODE -D_HEXAGON_DRV_
endif

ifeq ($(shell getconf LONG_BIT),64)
MYFLAGS+=-DUSE_DMA_64
endif

ifneq ($(KERNELRELEASE),)
obj-m        := $(MODNAME).o
$(MODNAME)-y := $(SRCFILES:.c=.o)
ccflags-y    += $(MYFLAGS) \
					 -I$(src)/../../../../Include \
					 -I$(src)/../../../../Include/Private \
					 -I$(src)/../../../../Include/Public \
					 -I$(src)/../../../../Include/Linux \
					 -I$(src)/../../../Bd_Include \
					 -I$(src)/../../KS_Include \
					 -I$(src)/../../KS_Include/Linux \
					 -I$(src)/../Include
else
.PHONY : modules install clean tidy uninstall distclean

modules : $(TARGET)

$(TARGET) : $(SRCFILES)
	@echo "- Building kernel-mode driver..."
	$(MYHIDE) $(MAKE) -C $(KDIR) M=$(shell pwd) modules

install : $(TARGET)
	@echo "- Installing kernel-mode driver..."
	@if [ ! -d $(INSTDIR) ]; then mkdir $(INSTDIR); fi
	@install -p -m 666 $(TARGET) $(INSTDIR)
	@depmod -a

clean :
	@echo "- Cleaning kernel-mode driver..."
	$(MYHIDE) $(MAKE) -C $(KDIR) M=$(shell pwd) clean
	$(MYHIDE) rm -f ../../KS_Common/Linux/*.o ../../KS_Common/Linux/.*.o.cmd
	$(MYHIDE) rm -f ../*.o ../.*.o.cmd
	$(MYHIDE) rm -f *.o .*.o.cmd

tidy :
	$(MYHIDE) rm -f *~

uninstall :
	@echo "- Uninstalling kernel-mode driver..."
	$(MYHIDE) rm -f $(INSTDIR)/$(TARGET)

distclean : clean tidy
	@echo "- Completely cleaning kernel-mode driver..."
	$(MYHIDE) rm -f TAGS
	$(MYHIDE) rm -f Module.symvers
	$(MYHIDE) rm -f Module.markers modules.order

endif
