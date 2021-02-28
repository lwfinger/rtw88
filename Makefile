SHELL := /bin/sh
KVER  ?= $(shell uname -r)
KSRC := /lib/modules/$(KVER)/build
FIRMWAREDIR := /lib/firmware/
PWD := $(shell pwd)
CLR_MODULE_FILES := *.mod.c *.mod *.o .*.cmd *.ko *~ .tmp_versions* modules.order Module.symvers
SYMBOL_FILE := Module.symvers
# Handle the move of the entire rtw88 tree
ifneq ("","$(wildcard /lib/modules/$(KVER)/kernel/drivers/net/wireless/realtek)")
MODDESTDIR := /lib/modules/$(KVER)/kernel/drivers/net/wireless/realtek/rtw88
else
MODDESTDIR := /lib/modules/$(KVER)/kernel/drivers/net/wireless/rtw88
endif
#Handle the compression option for modules in 3.18+
ifneq ("","$(wildcard $(MODDESTDIR)/*.ko.gz)")
COMPRESS_GZIP := y
endif
ifneq ("","$(wildcard $(MODDESTDIR)/*.ko.xz)")
COMPRESS_XZ := y
endif

EXTRA_CFLAGS += -O2
EXTRA_CFLAGS += -DCONFIG_RTW88_8822BE=1
EXTRA_CFLAGS += -DCONFIG_RTW88_8821CE=1
EXTRA_CFLAGS += -DCONFIG_RTW88_8822CE=1
EXTRA_CFLAGS += -DCONFIG_RTW88_8723DE=1
EXTRA_CFLAGS += -DCONFIG_RTW88_DEBUG=1
EXTRA_CFLAGS += -DCONFIG_RTW88_DEBUGFS=1
EXTRA_CFLAGS += -DCONFIG_RTW88_REGD_USER_REG_HINTS

obj-m	+= rtw_core.o
rtw_core-objs += main.o \
	   mac80211.o \
	   util.o \
	   debug.o \
	   tx.o \
	   rx.o \
	   mac.o \
	   phy.o \
	   coex.o \
	   efuse.o \
	   fw.o \
	   ps.o \
	   sec.o \
	   wow.o \
	   bf.o \
	   regd.o

obj-m       += rtw_8822b.o
rtw_8822b-objs                := rtw8822b.o rtw8822b_table.o

obj-m      += rtw_8822be.o
rtw_8822be-objs               := rtw8822be.o

obj-m       += rtw_8822c.o
rtw_8822c-objs                := rtw8822c.o rtw8822c_table.o

obj-m      += rtw_8822ce.o
rtw_8822ce-objs               := rtw8822ce.o

obj-m       += rtw_8723d.o
rtw_8723d-objs                := rtw8723d.o rtw8723d_table.o

obj-m      += rtw_8723de.o
rtw_8723de-objs               := rtw8723de.o

obj-m	+= rtw_8821c.o
rtw_8821c-objs		:= rtw8821c.o rtw8821c_table.o

obj-m	+= rtw_8821ce.o
rtw_8821ce-objs		:= rtw8821ce.o

obj-m         += rtw_pci.o
rtw_pci-objs                  := pci.o

ccflags-y += -D__CHECK_ENDIAN__

all: 
	$(MAKE) -C $(KSRC) M=$(PWD) modules
install: all
ifeq (,$(wildcard ./backup_drivers.tar))
	@echo Making backups
	@tar cPf backup_drivers.tar $(MODDESTDIR)
	@rm -f $(MODDESTDIR)/rtw88*.ko*
	@rm -f $(MODDESTDIR)/rtwpci.ko*
else
	@rm -f $(MODDESTDIR)/rtw88*.ko*
	@rm -f $(MODDESTDIR)/rtwpci.ko*
endif

	@mkdir -p $(MODDESTDIR)
	@install -p -D -m 644 *.ko $(MODDESTDIR)	
	@#copy firmware images to target folder
	@mkdir -p $(FIRMWAREDIR)/rtw88/
	@cp -f *.bin $(FIRMWAREDIR)/rtw88/
ifeq ($(COMPRESS_GZIP), y)
	@gzip -f $(MODDESTDIR)/*.ko
endif
ifeq ($(COMPRESS_XZ), y)
	@xz -f $(MODDESTDIR)/*.ko
endif

	@depmod -a $(KVER)

	@echo "Install rtw88 SUCCESS"

uninstall:
	@modprobe -r rtw_8822be
	@modprobe -r rtw_8822ce
	@modprobe -r rtw_8723de
	@rm -f $(MODDESTDIR)/rtw_*.ko
ifneq (,$(wildcard ./backup_drivers.tar))
	@echo Restoring backups
	@tar xPf backup_drivers.tar
	@rm backup_drivers.tar
endif
	
	@depmod -a
	
	@echo "Uninstall rtw88 SUCCESS"

clean:
	@rm -fr *.mod.c *.mod *.o .*.cmd .*.o.cmd *.ko *~ .*.o.d .cache.mk
	@rm -fr .tmp_versions
	@rm -fr Modules.symvers
	@rm -fr Module.symvers
	@rm -fr Module.markers
	@rm -fr modules.order
