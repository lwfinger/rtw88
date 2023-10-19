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

ifneq ("$(INSTALL_MOD_PATH)", "")
DEPMOD_ARGS = -b $(INSTALL_MOD_PATH)
else
DEPMOD_ARGS =
endif

#Handle the compression option for modules in 3.18+
ifneq ("","$(wildcard $(MODDESTDIR)/*.ko.gz)")
COMPRESS_GZIP := y
endif
ifneq ("","$(wildcard $(MODDESTDIR)/*.ko.xz)")
COMPRESS_XZ := y
endif
ifeq ("","$(wildcard MOK.der)")
NO_SKIP_SIGN := y
endif

EXTRA_CFLAGS += -O2
EXTRA_CFLAGS += -DCONFIG_RTW88_8822BE=1
EXTRA_CFLAGS += -DCONFIG_RTW88_8821CE=1
EXTRA_CFLAGS += -DCONFIG_RTW88_8822CE=1
EXTRA_CFLAGS += -DCONFIG_RTW88_8723DE=1
EXTRA_CFLAGS += -DCONFIG_RTW88_DEBUG=1
EXTRA_CFLAGS += -DCONFIG_RTW88_DEBUGFS=1
#EXTRA_CFLAGS += -DCONFIG_RTW88_REGD_USER_REG_HINTS

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
	   regd.o \
	   sar.o

obj-m       += rtw_8822b.o
rtw_8822b-objs                := rtw8822b.o rtw8822b_table.o

obj-m      += rtw_8822be.o
rtw_8822be-objs               := rtw8822be.o

obj-m      += rtw_8822bu.o
rtw_8822bu-objs		:= rtw8822bu.o

obj-m	   += rtw_8822bs.o
rtw_8822bs-objs		:= rtw8822bs.o

obj-m       += rtw_8822c.o
rtw_8822c-objs                := rtw8822c.o rtw8822c_table.o

obj-m      += rtw_8822ce.o
rtw_8822ce-objs               := rtw8822ce.o

obj-m      += rtw_8822cu.o
rtw_8822cu-objs		:= rtw8822cu.o

obj-m	   += rtw_8822cs.o
rtw_8822cs-objs		:= rtw8822cs.o

obj-m       += rtw_8723d.o
rtw_8723d-objs          := rtw8723d.o rtw8723d_table.o

obj-m      += rtw_8723de.o
rtw_8723de-objs               := rtw8723de.o

obj-m      += rtw_8723du.o
rtw_8723du-objs		:= rtw8723du.o

obj-m      += rtw_8723ds.o
rtw_8723ds-objs		:= rtw8723ds.o

obj-m	+= rtw_8821c.o
rtw_8821c-objs		:= rtw8821c.o rtw8821c_table.o

obj-m	+= rtw_8821ce.o
rtw_8821ce-objs		:= rtw8821ce.o

obj-m	   += rtw_8821cs.o
rtw_8821cs-objs		:= rtw8821cs.o

obj-m	+= rtw_8821cu.o
rtw_8821cu-objs		:= rtw8821cu.o

obj-m			+= rtw_pci.o
rtw_pci-objs		:= pci.o

obj-m			+= rtw_sdio.o
rtw_sdio-objs		:= sdio.o

obj-m			+= rtw_usb.o
rtw_usb-objs		:= usb.o

obj-m         += rtw_usb.o
rtw_usb-objs                := usb.o

ccflags-y += -D__CHECK_ENDIAN__

all: 
	$(MAKE) -C $(KSRC) M=$(PWD) modules
install: all
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

	@depmod $(DEPMOD_ARGS) -a $(KVER)

	@echo "Install rtw88 SUCCESS"

uninstall:
	@modprobe -r rtw_8822be
	@modprobe -r rtw_8822ce
	@modprobe -r rtw_8723de
	@rm -f $(MODDESTDIR)/rtw_*.ko
	
	@depmod $(DEPMOD_ARGS)
	
	@echo "Uninstall rtw88 SUCCESS"

clean:
	@rm -fr *.mod.c *.mod *.cmd *.o .*.cmd *.ko *~ .*.o.d .cache.mk
	@rm -fr .tmp_versions
	@rm -fr Modules.symvers
	@rm -fr Module.symvers
	@rm -fr Module.markers
	@rm -fr modules.order

sign:
ifeq ($(NO_SKIP_SIGN), y)
	@openssl req -new -x509 -newkey rsa:2048 -keyout MOK.priv -outform DER -out MOK.der -nodes -days 36500 -subj "/CN=Custom MOK/"
	@mokutil --import MOK.der
else
	echo "Skipping key creation"
endif
	@$(KSRC)/scripts/sign-file sha256 MOK.priv MOK.der rtw_pci.ko
	@$(KSRC)/scripts/sign-file sha256 MOK.priv MOK.der rtw_usb.ko
	@$(KSRC)/scripts/sign-file sha256 MOK.priv MOK.der rtw_sdio.ko
	@$(KSRC)/scripts/sign-file sha256 MOK.priv MOK.der rtw_core.ko
	@$(KSRC)/scripts/sign-file sha256 MOK.priv MOK.der rtw_8723d.ko
	@$(KSRC)/scripts/sign-file sha256 MOK.priv MOK.der rtw_8723de.ko
	@$(KSRC)/scripts/sign-file sha256 MOK.priv MOK.der rtw_8723du.ko
	@$(KSRC)/scripts/sign-file sha256 MOK.priv MOK.der rtw_8723ds.ko
	@$(KSRC)/scripts/sign-file sha256 MOK.priv MOK.der rtw_8822b.ko
	@$(KSRC)/scripts/sign-file sha256 MOK.priv MOK.der rtw_8822be.ko
	@$(KSRC)/scripts/sign-file sha256 MOK.priv MOK.der rtw_8822bu.ko
	@$(KSRC)/scripts/sign-file sha256 MOK.priv MOK.der rtw_8822bs.ko
	@$(KSRC)/scripts/sign-file sha256 MOK.priv MOK.der rtw_8821c.ko
	@$(KSRC)/scripts/sign-file sha256 MOK.priv MOK.der rtw_8821ce.ko
	@$(KSRC)/scripts/sign-file sha256 MOK.priv MOK.der rtw_8821cu.ko
	@$(KSRC)/scripts/sign-file sha256 MOK.priv MOK.der rtw_8821cs.ko
	@$(KSRC)/scripts/sign-file sha256 MOK.priv MOK.der rtw_8822c.ko
	@$(KSRC)/scripts/sign-file sha256 MOK.priv MOK.der rtw_8822ce.ko
	@$(KSRC)/scripts/sign-file sha256 MOK.priv MOK.der rtw_8822cu.ko
	@$(KSRC)/scripts/sign-file sha256 MOK.priv MOK.der rtw_8822cs.ko

sign-install: all sign install

