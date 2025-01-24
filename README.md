# rtw88 downstream 🐧
### This is a downstream repo with a primary purpose of supporting development, testing and maintenance for the Realtek rtw88 series of WiFi 5 drivers in the Linux kernel.

The most recent addition to this repo is the driver for the RTL8814AU chipset. Testing is needed so if you have an adapter based on the RTL8814AU chipset, please test and report.

🌟 The code in this repo stays in sync with the `wireless-next` repository, with additional changes to accommodate kernel API changes over time.

📌 **Note**: The `wireless-next` repo contains the code set for the ***next*** kernel version. If kernel 6.X is out, kernel mainline repo is on 6.X+1-rcY, and `wireless-next` targets kernel 6.X+2 material.

---

## Compatibility
Compatible with **Linux kernel versions 5.4 and newer** as long as your distro hasn't modified any kernel APIs. RHEL and all distros based on RHEL will have modified kernel APIs and are unlikely to be compatible with this driver.


#### Supported Chipsets
- **PCIe**: RTL8723DE, RTL8821CE, RTL8822BE, RTL8822CE
- **SDIO**: RTL8723CS, RTL8723DS, RTL8821CS, RTL8822BS, RTL8822CS
- **USB** : RTL8723DU, RTL8811AU, RTL8811CU, RTL8812AU, RTL8812BU, RTL8812CU
- **USB** : RTL8814AU, RTL8821AU, RTL8821CU, RTL8822BU, RTL8822CU

---

## Issues 🚨
Report problems in Issues after you have checked the [FAQ](#faq) at bottom of this README.

⚠️ If you see a line such as:

`make[1]: *** /lib/modules/5.17.5-300.fc36.x86_64/build: No such file or directory.` **Stop.**

This indicates **you have NOT installed the kernel headers.** 

Use the following instructions for that step.

## Installation Guide

### Prerequisites 📋
Below are prerequisites for common Linux distributions __before__ you do a [basic installation](#basic-installation-for-all-distros-) or [installation with SecureBoot](#installation-with-secureboot-for-all-distros-):

#### Ubuntu
```bash
sudo apt update && sudo apt upgrade
```
```bash
sudo apt install linux-headers-$(uname -r) build-essential git
```

#### Fedora
```bash
sudo dnf update
```
```bash
sudo dnf install kernel-devel git
```

#### openSUSE
```bash
sudo zypper install make gcc kernel-devel kernel-default-devel git libopenssl-devel
```

#### Arch
```bash
sudo pacman -Syu
```
```bash
sudo pacman -Sy base-devel git linux-firmware
```
Remember to install the corresponding [kernel headers](https://archlinux.org/packages/?q=Headers+and+scripts+for+building+modules) which is also needed for compilation.

#### Raspberry Pi OS
```bash
sudo apt update && sudo apt upgrade
```
```bash
sudo apt install -y raspberrypi-kernel-headers build-essential git
```

---
### Basic Installation for All Distros 🛠

```bash
git clone https://github.com/lwfinger/rtw88
```
```bash
cd rtw88
```
```bash
make
```
```bash
sudo make install
```
```bash
sudo make install_fw
```
---

### Basic Installation for Arch-based Distros 🛠
```bash
git clone https://aur.archlinux.org/rtw88-dkms-git.git
```
```bash
cd rtw88-dkms-git
```
```bash
makepkg -si
```
---
### Installation with SecureBoot for All Distros 🔒

```bash
git clone https://github.com/lwfinger/rtw88
```
```bash
cd rtw88
```
```bash
make
```
```bash
sudo make install_fw
```
```bash
sudo make sign-install
```
You will be prompted a password, **please keep it in mind and use it in next steps.**

Reboot to activate the new installed module, then in the MOK managerment screen:
1. Select "Enroll key" and enroll the key created by above sign-install step
2. When promted, enter the password you entered when create sign key. 
3. If you enter wrong password, your computer won't be bootable. In this case,
   use the BOOT menu from your BIOS, to boot into your OS then do below steps:
```bash
sudo mokutil --reset
```
- Restart your computer.
- Use BOOT menu from BIOS to boot into your OS.
- In the MOK managerment screen, select reset MOK list.
- Reboot then retry from the step `make sign-install`.

---

## Important Information
Below is important information for using this driver.

### 1. Blacklisting 🚫
A file called `blacklist-rtw88.conf` will be installed into `/etc/modprobe.d` when you run `sudo make install`. It will blacklist all in-kernel rtw88 drivers, however, it will not blacklist out-of-kernel vendor drivers. You will need to uninstall any out-of-kernel vendor drivers that you have installed that may conflict. `blacklist-rtw88.conf` will be removed when you run `sudo make uninstall`.

### 2. Recovery Problems After Sleep/Hibernation 🛌
Some BIOSs have trouble changing power state from D3hot to D0. If you have this problem, then:

``` bash
sudo cp suspend_rtw8822be /usr/lib/systemd/system-sleep/.
```

That script will unload the driver before sleep or hibernation, and reload it following resumption.
If you have some device other than the 8822be, edit the script before copying it.


### 3. How to Disable/Enable a Kernel Module 🪛
 ```bash
sudo modprobe -r rtw_8723de # This unloads the module
sudo modprobe -r rtw_core

# Due to some pecularities in the modprobe utility, two steps are required.

sudo modprobe rtw_8723de    # This loads the module

# Only a single modprobe call is required to load.
```

### 4. Kernel Updates 🔄
When your kernel updates, run:
```bash
cd ~/rtw88
```
Change the above to the location where you downloaded the driver if necessary.

```
git pull
```
```
make
```

```
sudo make install
or
sudo make sign-install
```




💡 **Remember, every newly installed kernel requires this step - no exceptions**. If the kernel update means that you have no network, skip the 'git pull' and build the driver as it is, but run `git pull` once you have connectivity, and rebuild if any updates were pulled.

---

## FAQ

Below are some Frequently Asked Questions:

---

### Q1: Is Secure Boot supported?
Yes, this repository provides a way to sign the kernel modules to be compatible with Secure Boot. [Check out the Installation with SecureBoot section](#installation-with-secureboot-for-all-distros-).

---

### Q2: How to remove this driver if it doesn't work as expected?
Run this command in the rtw88 source directory and then the rtw88 driver will be unloaded and removed.

`sudo make uninstall`

For Arch-based distro users, run

`sudo pacman -Rn rtw88-dkms-git`

---

### Q3: My wifi adapter is plugged in an USB 3 port, how can I keep it in USB 2 mode to avoid potential interference in 2.4 GHz band?
The module `rtw_usb` has a parameter named `switch_usb_mode` which can enable or disable USB mode switching, setting it to "N" will keep your adapter in USB 2 mode. You can simply run the command below, unplug the adapter, reboot your computer, replug the adapter back and then your adapter will be in USB 2 mode.

`sudo sh -c 'echo options rtw_usb switch_usb_mode=N > /etc/modprobe.d/rtw88.conf'`

---

### Q4: My wifi adapter still doesn't work after installing this driver and `lsusb` shows it is in CD-ROM mode, what should I do?
Install `usb_modeswitch` which can switch your adapter from CD-ROM mode to Wi-Fi mode and then your wifi adapter should be in Wi-Fi mode after reboot.

---
