rtw88
===========
### A repo for the newest Realtek wifi 5 codes.

The code in this repo is kept up to date with the code in the wireless-next repository.
The only difference is the changes that are required to compensate for kernel API changes
with time.

Note: Wireless-next contains the code that will be in the NEXT kernel. At the moment,
kernel 6.3 is released, the kernel mainline repo has 6.4-rcX, and wireless-next has
the code that will be in 6.5! As long as possible, that relationship will be kept!

This code will build on any kernel 5.4 and newer as long as the distro has not modified
any of the kernel APIs. IF YOU RUN UBUNTU, YOU CAN BE ASSURED THAT THE APIs HAVE CHANGED.
NO, I WILL NOT MODIFY THE SOURCE FOR YOU. YOU ARE ON YOUR OWN!!!!!

This repository includes drivers for the following cards:

RTW8822BE, RTW8822CE, RTW8821CE, and RTW8723DE  (All PCIe)
RTW8822BU, RTW8822CU, RTW8821CU, and RTW8723DU (All USB)
RTW8822BS, RTW8822CS, RTW8821CS, and RTW8723DS  (All SDIO)

If you are looking for a driver for chips such as 
RTL8188EE, RTL8192CE, RTL8192CU, RTL8192DE, RTL8192EE, RTL8192SE, RTL8723AE, RTL8723BE, or RTL8821AE,
these should be provided by your kernel. If not, then you should go to the Backports Project
(https://backports.wiki.kernel.org/index.php/Main_Page) to obtain the necessary code.

Please let me know of build problems.

If you see a line such as:
make[1]: *** /lib/modules/5.17.5-300.fc36.x86_64/build: No such file or directory. Stop.
that indicates that you have NOT installed the kernel headers. Use the following instructions for that step.

### Installation instruction
##### Requirements
You will need to install "make", "gcc", "kernel headers", "kernel build essentials", and "git".
You can install them with the following command, on **Ubuntu**:
```bash
sudo apt-get update
sudo apt-get install make gcc linux-headers-$(uname -r) build-essential git
```
For **Fedora**: You can install them with the following command
```bash
sudo dnf install kernel-headers kernel-devel
sudo dnf group install "C Development Tools and Libraries"
```
For **openSUSE**: Install necessary headers with
```bash
sudo zypper install make gcc kernel-devel kernel-default-devel git libopenssl-devel
```
For **Arch**: After installing the necessary kernel headers and base-devel,
```bash
git clone https://aur.archlinux.org/rtw88-dkms-git.git
cd rtw88-dkms-git
makepkg -sri
```
If any of the packages above are not found check if your distro installs them like that.

##### Installation
For all distros:
```bash
git clone https://github.com/lwfinger/rtw88.git
cd rtw88
make
sudo make install
```
##### Installation with module signing for SecureBoot
For all distros:
```bash
git clone git://github.com/lwfinger/rtw88.git
cd rtw88
make
sudo make sign-install
```
You will be prompted a password, please keep it in mind and use it in next steps.
Reboot to activate the new installed module.
In the MOK managerment screen:
1. Select "Enroll key" and enroll the key created by above sign-install step
2. When prompted, enter the password you entered when create sign key. 
3. If you enter wrong password, your computer won't not bebootable. In this case,
   use the BOOT menu from your BIOS, to boot into your OS then do below steps:
```bash
sudo mokutil --reset
```
Restart your computer
Use BOOT menu from BIOS to boot into your OS
In the MOK managerment screen, select reset MOK list
Reboot then retry from the step make sign-install

##### Blacklisting (needed if you want to use these modules)
Some distros provide `RTL8723DE` drivers. To use this driver, that one MUST be
blacklisted. How to do that is left as an exercise as learning that will be very beneficial.

If your system has ANY conflicting drivers installed, you must blacklist them as well. For kernels
5.6 and newer, this will include drivers such as rtw88_xxxx.
Here is a useful [link](https://askubuntu.com/questions/110341/how-to-blacklist-kernel-modules) on how to blacklist a module

Once you have reached this point, then reboot. Use the command `lsmod | grep rtw` and check if there are any
conflicting drivers. The correct ones are:
- `rtw_8723de rtw_8723du rtw_8723d  rtw_8822be  rtw_8822bu rtw8822bs rtw_8822b  rtw_8822ce  rtw_8822cu rtw_8822cs
   rtw_8821ce rtw_8821cu rtw_8821cs rtw_8822c rtw_8723ds rtw_core  and rtw_pci`

If you have other modules installed, see if you blacklisted them correctly.

##### Problem with recovery after sleep or hibernation
Some BIOSs have trouble changing power state from D3hot to D0. If you have this problem, then

sudo cp suspend_rtw8822be /usr/lib/systemd/system-sleep/.

That script will unload the driver before sleep or hibernation, and reload it following resumption.
f you have some device other than the 8822be, edit the script before copying it.

##### How to disable/enable a Kernel module
 ```bash
sudo modprobe -r rtw_8723de         #This unloads the module
sudo modprobe -r rtw_core

Due to some pecularities in the modprobe utility, two steps are required.

sudo modprobe rtw_8723de            #This loads the module

Only a single modprobe call is required to load.
```

##### Option configuration
If it turns out that your system needs one of the configuration options, then do the following:
```bash
sudo nano /etc/modprobe.d/<dev_name>.conf 
```
There, enter the line below:
```bash
options <device_name> <<driver_option_name>>=<value>
```
The available options for rtw_pci are disable_msi and disable_aspm.
The available options for rtw_core are lps_deep_mode, support_bf,  and debug_mask.

***********************************************************************************************

When your kernel changes, then you need to do the following:
```bash
cd ~/rtw88
git pull
make
sudo make install
```

Remember, this MUST be done whenever you get a new kernel - no exceptions.

These drivers will not build for kernels older than 4.14. If you must use an older kernel,
submit a GitHub issue with a listing of the build errors. Without the errors, the issue
will be ignored. I am not a mind reader.

When you have problems where the driver builds and loads correctly, but fails to work, a GitHub
issue is NOT the best place to report it. I have no idea of the internal workings of any of the
chips, and the Realtek engineers who do will not read these issues. To reach them, send E-mail to
linux-wireless@vger.kernel.org. Include a detailed description of any messages in the kernel
logs and any steps that you have taken to analyze or fix the problem. If your description is
not complete, you are unlikely to get any satisfaction.

