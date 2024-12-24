# rtw88 downstream 🐧
### This is a downstream repo with a primary purpose of supporting development, testing and maintenance for the Realtek rtw88 series of WiFi 5 drivers in the Linux kernel.

The most recent addition to this repo is the driver for the rtw8814au chipset. Testing is needed so if you have an adapter based on the rtw8814au chipset, please test and report.

🌟 The code in this repo stays in sync with the `wireless-next` repository, with additional changes to accommodate kernel API changes over time.

📌 **Note**: The `wireless-next` repo contains the code set for the ***next*** kernel version. If kernel 6.X is out, kernel mainline repo is on 6.X+1-rcY, and `wireless-next` targets kernel 6.X+2 material.

---

## Compatibility
Compatible with **Linux kernel versions 5.4 and newer** as long as your distro hasn't modified any kernel APIs. RHEL and all distros based on RHEL will have modified kernel APIs and are unlikely to be compatible with this driver.


#### Supported Chipsets
- **USB** : RTW8814AU, RTW8812AU, RTW8821AU, RTW8811AU, RTW8822BU, RTW8812BU
- **USB** : RTW8822CU, RTW8812CU, RTW8821CU, RTW8811CU, RTW8723DU
- **PCIe**: RTW8822BE, RTW8822CE, RTW8821CE, RTW8723DE
- **SDIO**: RTW8822BS, RTW8822CS, RTW8821CS, RTW8723DS

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
sudo apt install make gcc linux-headers-$(uname -r) build-essential git
```

#### Fedora
```bash
sudo dnf install kernel-headers kernel-devel
```
```bash
sudo dnf group install "C Development Tools and Libraries"
```

#### openSUSE
```bash
sudo zypper install make gcc kernel-devel kernel-default-devel git libopenssl-devel
```

#### Arch
```bash
git clone https://aur.archlinux.org/rtw88-dkms-git.git
```
```bash
cd rtw88-dkms-git
```
```bash
makepkg -sri
```

#### Raspberry Pi OS
```bash
sudo apt update && sudo apt upgrade
```
```bash
sudo apt install -y raspberrypi-kernel-headers build-essential bc git
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
sudo make sign-install
```
You will be prompted a password, **please keep it in mind and use it in next steps.**

Reboot to activate the new installed module, then in the MOK managerment screen:
1. Select "Enroll key" and enroll the key created by above sign-install step
2. When promted, enter the password you entered when create sign key. 
3. If you enter wrong password, your computer won't not bebootable. In this case,
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

### 4. Driver Parameter Configuration (options) 📝

Example for chipset RTL8814AU and driver name rtw_8814au:

Note: the driver names in the rtw88 in the Linux Mainline kernel are slightly different: rtw88_8814au

```bash
sudo nano /etc/modprobe.d/rtw_8814au.conf
```

Note: Change rtw_8814au in the above to the name of the driver you wish to modify.

In the empty file, paste the following:

```bash
options rtw_8814au switch_usb_mode=N

```

See below for an explanation of the above parameter and for information about other parameters.

The available parameter (option) for USB 3 devices is:

switch_usb_mode=<Y/N>

Set to N to disable switching to USB 3 mode to avoid potential interference in the 2.4 GHz band (default: Y) (bool). This parameter (option) applies to the USB 3 capable devices: RTL8814AU (rtw_8814au), RTL8822CU (rtw_8822cu), RTL8812CU (rtw_8822cu), RTL8822BU (rtw_8822bu), RTL8812BU (rtw_8822bu), and RTL8812AU (rtw_8812au). It's okay to leave the parameter on even when plugging the device into a USB 2 port because it will try to switch to USB 3 mode only once.

The available options for rtw_pci are:

disable_msi
disable_aspm

The available options for rtw_core are:

disable_lps_deep
support_bf
debug_mask

### 5. Kernel Updates 🔄
When your kernel updates, run:
```bash
cd ~/rtw88
```
```
git pull
```
```
make
```
```
sudo make install
```
💡 **Remember, every newly installed kernel requires this step - no exceptions**. If the kernel update means that you have no network, skip the 'git pull' and build the driver as it is, but run `git pull` once you have connectivity, and rebuild if any updates were pulled.

---

## FAQ

Below is a set Frequently Asked Questions when using this repository.

---

### Q1: My driver builds and loads correctly, but fails to work properly. Can you help me?
When you have problems where the driver builds and loads correctly, but it fails to work, a GitHub
issue on this repository is **NOT** the place to report it. 

We have no idea about the internal workings of any of the chips, and the Realtek engineers who do will not read these issues. To reach them, send E-mail to [linux-wireless@vger.kernel.org](mailto:linux-wireless@vger.kernel.org). 

Be sure to include a detailed description of any messages in the kernel logs and any steps that you have taken to analyze or fix the problem. If your description is not complete, you are unlikely to get the help you need. NOTE: E-mail sent to that mailing list MUST be plain text. HTML mail will be rejected.

Start with this page for guidance: https://wireless.wiki.kernel.org/en/users/support

---

### Q2: I'm using Ubuntu, and the build failed. What do I do?
Ubuntu often modifies kernel APIs, which can cause build issues. You'll need to manually adjust the source code or look for solutions specific to your distribution. We cannot support these types of issues.

---

### Q3: How do I update the driver after a kernel update?
You will need to pull the latest code from this repository and recompile the driver. [Follow the steps in the Maintenance section](#kernel-updates-).

---

### Q4: Is Secure Boot supported?
Yes, this repository provides a way to sign the kernel modules to be compatible with Secure Boot. [Check out the Installation with SecureBoot section](#installation-with-secureboot-).

---

### Q5: My card isn't listed. Can I request a feature?
For feature requests like supporting a new card, you should reach out to Realtek engineers via E-mail at [linux-wireless@vger.kernel.org](mailto:linux-wireless@vger.kernel.org).

---

### Q6: How to remove this driver if it doesn't work as expected?
Run this command in the rtw88 source directory and then the rtw88 driver will be unloaded and removed.

`sudo make uninstall`


