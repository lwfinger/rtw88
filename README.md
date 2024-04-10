# rtw88 📡🐧
### A Repo for the Latest Realtek WiFi 5 Codes on Linux

🌟 **Up-to-Date Drivers**: The code in this repo stays in sync with the `wireless-next` repository, with additional changes to accommodate kernel API changes over time. The current repo matches the kernel as of March 5, 2024.

📌 **Note**: The `wireless-next` repo contains the code set for the ***next*** kernel version. If kernel 6.X is out, kernel mainline repo is on 6.X+1-rcY, and `wireless-next` targets kernel 6.X+2 material.

---

### Compatibility
Compatible with **Linux kernel versions 5.4 and newer** as long as your distro hasn't modified any kernel APIs.

⚠️ **Ubuntu users, expect API changes!** We **will not** modify the source for you. You are on your own!

#### Supported Cards
- **PCIe**: RTW8822BE, RTW8822CE, RTW8821CE, RTW8723DE
- **USB**: RTW8822BU, RTW8822CU, RTW8821CU, RTW8723DU
- **SDIO**: RTW8822BS, RTW8822CS, RTW8821CS, RTW8723DS

**Are you looking for support for these drivers?** 🔎 ⚠️
```bash
# We do not support the following:
RTL8188EE
RTL8192CE
RTL8192CU
RTL8192DE
RTL8192EE
RTL8192SE
RTL8723AE
RTL8723BE
RTL8821AE
```
Check your current kernel or visit the [Backports Project](https://backports.wiki.kernel.org/index.php/Main_Page).

## Troubleshooting & Support

These drivers **won't build for kernels older than 5.4**. Submit GitHub issues **only** for build errors.

For operational problems, reach out to Realtek engineers via E-mail at [linux-wireless@vger.kernel.org](mailto:linux-wireless@vger.kernel.org).

## Issues 🚨
Report any build problems and [see the FAQ](#faq) at bottom of this README.


⚠️ If you see a line such as:
`make[1]: *** /lib/modules/5.17.5-300.fc36.x86_64/build: No such file or directory.` **Stop.**

This indicates **you have NOT installed the kernel headers.** 

Use the following instructions for that step.

## Installation Guide

### Prerequisites 📋
Below are prerequisites for common Linux distributions __before__ you do a [basic installation](#basic-installation-for-all-distros-) or [installation with SecureBoot](#installation-with-secureboot-for-all-distros-):

#### Ubuntu
```bash
sudo apt-get update
sudo apt-get install make gcc linux-headers-$(uname -r) build-essential git
```

#### Fedora
```bash
sudo dnf install kernel-headers kernel-devel
sudo dnf group install "C Development Tools and Libraries"
```

#### openSUSE
```bash
sudo zypper install make gcc kernel-devel kernel-default-devel git libopenssl-devel
```

#### Arch
```bash
git clone https://aur.archlinux.org/rtw88-dkms-git.git
cd rtw88-dkms-git
makepkg -sri
```
---
### Basic Installation for All Distros 🛠

```bash
git clone https://github.com/lwfinger/rtw88.git
cd rtw88
make
sudo make install
```
---
### Installation with SecureBoot for All Distros 🔒

```bash
git clone git://github.com/lwfinger/rtw88.git
cd rtw88
make
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

## Important Information
Below is important information for using this driver.

### 1. Blacklisting 🚫
If your system has ANY conflicting drivers installed, you must blacklist them as well. For kernels
5.6 and newer, this will include drivers such as rtw88_xxxx. Here is a useful [link](https://askubuntu.com/questions/110341/how-to-blacklist-kernel-modules) on how to blacklist a module.

Once you have reached this point, then reboot. Use the command `lsmod | grep rtw` and check if there are any
conflicting drivers. The correct ones are:
- `rtw_8723de rtw_8723du rtw_8723d  rtw_8822be  rtw_8822bu rtw8822bs rtw_8822b  rtw_8822ce  rtw_8822cu rtw_8822cs
   rtw_8821ce rtw_8821cu rtw_8821cs rtw_8822c rtw_8723ds rtw_core rtw_pci`

If you have other modules installed, see if you blacklisted them correctly.

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

### 4. Option Configuration 📝
```bash
sudo nano /etc/modprobe.d/<dev_name>.conf
```

There, enter the line below:
```bash
options <device_name> <<driver_option_name>>=<value>
```
The available options for rtw_pci are disable_msi and disable_aspm.
The available options for rtw_core are disable_lps_deep, support_bf,  and debug_mask.

---

## Kernel Updates 🔄

When your kernel updates, run:
```bash
cd ~/rtw88
git pull
make
sudo make install
```
💡 **Remember, every new kernel requires this step - no exceptions**. If the update means that you have no network, skip the 'git pull' and build the possible oldeer version, but do that step once you have network, and rebuild if any updates were pulled.

## FAQ

Below is a set Frequently Asked Questions when using this repository.

---

### Q1: My driver builds and loads correctly, but fails to work properly. Can you help me?
When you have problems where the driver builds and loads correctly, but it fails to work, a GitHub
issue on this repository is **NOT** the place to report it. 

We have no idea about the internal workings of any of the chips, and the Realtek engineers who do will not read these issues. To reach them, send E-mail to [linux-wireless@vger.kernel.org](mailto:linux-wireless@vger.kernel.org). 

Be sure to include a detailed description of any messages in the kernel logs and any steps that you have taken to analyze or fix the problem. If your description is not complete, you are unlikely to get the help you need. NOTE: E-mail sent to that mailing list MUST be plain test. HTML mail will be rejected.

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
