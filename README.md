rtw88
===========
### A repo for the newest Realtek rtlwifi codes.

This code will build on any kernel 4.19 and newer as long as the distro has not modified
any of the kernel APIs. IF YOU RUN UBUNTU, YOU CAN BE ASSURED THAT THE APIs HAVE CHANGED.
NO, I WILL NOT MODIFY THE SOURCE FOR YOU. YOU ARE ON YOUR OWN!!!!!

This repository includes drivers for the following cards:

RTL8822BE, RTL8822CE, RTL8821CE, and RTL8723DE

If you are looking for a driver for chips such as 
RTL8188EE, RTL8192CE, RTL8192CU, RTL8192DE, RTL8192EE, RTL8192SE, RTL8723AE, RTL8723BE, or RTL8821AE,
these should be provided by your kernel. If not, then you should go to the Backports Project
(https://backports.wiki.kernel.org/index.php/Main_Page) to obtain the necessary code.

This repo has been brought up to date with the kernel code on Sep. 25, 2020.

The main changes are as follows:
1. The methods for obtaining DMA buffers has changed. This should have no effect.
2. The regulatory methods are changed. This may have some effect on users.
3. The firmware loading has been more resistent against timeouts.
4. The RX buffer size is increased.
5. Antenna selection code was modified. This change may help the low signal problems.
6. BlueTooth coexistence was modified.

When making these changes, I tried to watch for things that might be incompatible
with older kernels. As this kind of updating in really boring, I might have missed
something. Please let me know of build problems.


### Installation instruction
##### Requirements
You will need to install "make", "gcc", "kernel headers", "kernel build essentials", and "git".
You can install them with the following command, on **Ubuntu**:
```bash
sudo apt-get update
sudo apt-get install make gcc linux-headers-$(uname -r) build-essential git
```
If any of the packets above are not found check if your distro installs them like that. 

##### Installation
For all distros:
```bash
git clone https://github.com/lwfinger/rtw88.git
cd rtw88
make
sudo make install
```
##### Blacklisting (needed if you want to use these modules)
Some distros provide `RTL8723DE` drivers. To use this driver, that one MUST be
blacklisted. How to do that is left as an exercise as learning that will be very beneficial.

If your system has ANY conflicting drivers installed, you must blacklist them as well. For kernels
5.6 and newer, this will include drivers such as rtw88_xxxx.
Here is a useful [link](https://askubuntu.com/questions/110341/how-to-blacklist-kernel-modules) on how to blacklist a module

Once you have reached this point, then reboot. Use the command `lsmod | grep rtw` and check if there are any
conflicting drivers. The correct ones are:
- `rtw_8723de  rtw_8723d  rtw_8822be  rtw_8822b  rtw_8822ce  rtw_8822c  rtw_core  and rtw_pci`

If you have other modules installed, see if you blacklisted them correctly.

##### How to disable/enable a Kernel module
 ```bash
sudo modprobe -r rtw_8723de         #This unloads the module
sudo modprobe rtw_8723de            #This loads the module
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

