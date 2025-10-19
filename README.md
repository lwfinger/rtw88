# rtw88 downstream 🐧
### This is a downstream repo with a primary purpose of supporting development, testing and maintenance for the Realtek rtw88 series of WiFi 5 drivers in the Linux kernel.

The most recent addition to this repo is the driver for the RTL8814AU chipset. Testing is needed so if you have an adapter based on the RTL8814AU chipset, please test and report.

Update: There is now also a driver for RTL8814AE PCIe cards, ID 10ec:8813. lspci may call it RTL8813AE, same thing. Please test and report if you still have this card.

🌟 The code in this repo stays in sync with the `wireless-next` repository, with additional changes to accommodate kernel API changes over time.

📌 **Note**: The `wireless-next` repo contains the code set for the ***next*** kernel version. If kernel 6.X is out, kernel mainline repo is on 6.X+1-rcY, and `wireless-next` targets kernel 6.X+2 material.

---

## Compatibility
Compatible with **Linux kernel versions 5.4 and newer** as long as your distro hasn't modified any kernel APIs. RHEL and all distros based on RHEL will have modified kernel APIs and are unlikely to be compatible with this driver.


#### Supported Chipsets
- **PCIe**: RTL8723DE, RTL8814AE, RTL8821CE, RTL8822BE, RTL8822CE
- **SDIO**: RTL8723CS, RTL8723DS, RTL8821CS, RTL8822BS, RTL8822CS
- **USB** : RTL8723DU, RTL8811AU, RTL8811CU, RTL8812AU, RTL8812BU, RTL8812CU
- **USB** : RTL8814AU, RTL8821AU, RTL8821CU, RTL8822BU, RTL8822CU

---

## Issues 🚨
Report problems in Issues after you have checked the [Q&A](#qa) at bottom of this README.

⚠️ If you see a line such as:

`make[1]: *** /lib/modules/5.17.5-300.fc36.x86_64/build: No such file or directory.` **Stop.**

This indicates **you have NOT installed the kernel headers.** 

Use the following instructions for that step.

## Installation Guide

### Prerequisites 📋
Below are prerequisites for common Linux distributions __before__  installing this driver.

#### Ubuntu
```bash
sudo apt update && sudo apt upgrade
```
```bash
sudo apt install linux-headers-generic build-essential git
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
Remember to install the corresponding [kernel headers](https://archlinux.org/packages/?q=Headers+and+scripts+for+building+modules) which are also needed for compilation.

#### Raspberry Pi OS
```bash
sudo apt update && sudo apt upgrade
```
```bash
sudo apt install -y raspberrypi-kernel-headers build-essential git
```

---

### Installation Using DKMS 🔄
Installing this driver via DKMS is highly recommended, especially if Secure Boot is enabled on your machine. Using DKMS (Dynamic Kernel Module Support) ensures that the `rtw88` kernel modules are automatically rebuilt and re-signed whenever the Linux kernel is updated. Without DKMS, these drivers would stop working after each kernel update, requiring manual re-compilation and re-signing. DKMS should be available through your distribution’s package manager. You can learn more about DKMS [here](https://github.com/dell/dkms).

1. Install `dkms` and all its required dependencies using your preferred package manager.

2. Clone the `rtw88` GitHub repository
   ```
   git clone https://github.com/lwfinger/rtw88
   ```

3. Build, sign, and install the rtw88 driver
   ```
   cd rtw88
   ```
   ```
   sudo dkms install $PWD
   ```

4. Install the firmware necessary for the rtw88 driver
   ```
   sudo make install_fw
   ```

5. Copy the configuration file `rtw88.conf` to `/etc/modprobe.d/`
   ```
   sudo cp rtw88.conf /etc/modprobe.d/
   ```

6. Enroll the MOK (Machine Owner Key), this is needed **ONLY IF** Secure Boot is enabled on your machine.
   ```
   sudo mokutil --import /var/lib/dkms/mok.pub
   ```
   For Ubuntu-based distro users, run this command instead
   ```
   sudo mokutil --import /var/lib/shim-signed/mok/MOK.der
   ```
   
   Note: At this point, you will be requested to enter a password. Remember this password and re-enter it after rebooting your system in order to enroll your new MOK into your system's UEFI. Please see [this tutorial](https://github.com/dell/dkms?tab=readme-ov-file#secure-boot) for more details.


---

### Installation Using make🛠

You will need to rebuild and reinstall the driver manually after each kernel updates if you choose this way to install the driver. This method is **NOT RECOMMENDED** for systems with Secure Boot enabled.

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
```bash
sudo cp rtw88.conf /etc/modprobe.d/
```
---

### Installation for Arch-based Distros 🛠

This is the best way for Arch-based distro users to install this driver, one more step is required after running `makepkg -si` if Secure Boot is enabled on your machine: Enroll the MOK. Please see the step 6 in [Installation Using DKMS](#installation-using-dkms-) for details.

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

## Important Information
Below is important information for using this driver.

### 1. Blacklisting 🚫
The configuration file `rtw88.conf` in `/etc/modprobe.d/` will blacklist all in-kernel rtw88 drivers, however, it will not blacklist out-of-kernel vendor drivers. You will need to uninstall any out-of-kernel vendor drivers that you have installed that may conflict. 

### 2. Recovery Problems After Sleep/Hibernation 🛌
Some BIOSs have trouble changing power state from D3hot to D0. If you have this problem, run 

``` bash
sudo cp reload_rtw88.sh /usr/lib/systemd/system-sleep/
```

That script will unload the driver before sleep or hibernation, and reload it following resumption.

### 3. How To Load/Unload Kernel Modules (aka Drivers)🪛

The kernel modules for RTL8821CU are used to demonstrate here, you need to change the module name according to the chip that your Wi-Fi adapter uses.

This unloads the modules for RTL8821CU, due to some pecularities in the modprobe utility, two steps are required. If you are not sure the names of the loaded modules for your Wi-Fi adapter, run `lsmod | grep -i rtw`
```
sudo modprobe -r rtw_8821cu
sudo modprobe -r rtw_core

# This command will show nothing because the modules for RTL8821CU were unloaded.
lsmod | grep -i rtw
```

This loads the modules for RTL8821CU, only a single modprobe call is required.
```
sudo modprobe rtw_8821cu

# This command will show that the modules for RTL8821CU were loaded.
lsmod | grep -i rtw
rtw_8821cu             16384  0
rtw_usb                36864  1 rtw_8821cu
rtw_8821c              98304  1 rtw_8821cu
rtw_core              294912  2 rtw_usb,rtw_8821c
mac80211             1220608  2 rtw_usb,rtw_core
cfg80211             1064960  2 rtw_core,mac80211
```

### 4. How To Update The Driver Installed via DKMS

1. Remove the rtw88 driver and its source code.
   ```
   sudo dkms remove rtw88/0.6 --all
   ```
   ```
   sudo rm -r /usr/src/rtw88-0.6/
   ```

2. Run this command in the rtw88 source directory to pull the latest code 
   ```
   git pull
   ```

3. Build, sign, and install the rtw88 driver from the latest code
   ```
   sudo dkms install $PWD
   ```
---

## Q&A

### Q1: How to remove this driver if it doesn't work as expected?

For users who installed this driver via DKMS, run
```
sudo dkms remove rtw88/0.6 --all
```
```
sudo rm -rf /usr/src/rtw88-0.6
```
```
sudo rm -f /etc/modprobe.d/rtw88.conf
```

For users who installed this driver via `make`, run these commands in the rtw88 source directory and then the rtw88 driver will be unloaded and removed.

```
sudo make uninstall
```
```
sudo rm /etc/modprobe.d/rtw88.conf
```

For Arch-based distro users, run

```
sudo pacman -Rn rtw88-dkms-git
```

### Q2: My Wi-Fi adapter is plugged into a USB 3 port. How can I keep it in USB 2 mode to avoid potential interference in the 2.4 GHz band?
The module `rtw_usb` has a parameter named `switch_usb_mode` which can enable or disable USB mode switching, setting it to "n" will keep your adapter in USB 2 mode.

Steps:
1. Open /etc/modprobe.d/rtw88.conf with your preferred editor.
2. Change `switch_usb_mode=y` to `switch_usb_mode=n` in line 7.
3. Unplug the Wi-Fi adapter.
4. Reboot (or [unload the rtw88 drivers](#3-how-to-loadunload-kernel-modules-aka-drivers))
5. Plug your Wi-Fi adapter back in.

### Q3: My Wi-Fi adapter still doesn't work after installing this driver and `lsusb` shows it is in CD-ROM mode, what should I do?
Install `usb_modeswitch` which can switch your adapter from CD-ROM mode to Wi-Fi mode and then your Wi-Fi adapter should be in Wi-Fi mode after reboot.
