# Requirements / Setup

 * Nvidia drivers (https://developer.nvidia.com/cuda-downloads?target_os=Linux&target_arch=x86_64&Distribution=Ubuntu&target_version=20.04&target_type=deb_network)
 * Visual Studio 2019 (or something else to compile the project source code)
 * sudo package for Nvidia-SMI functionality

Add `your_username_here ALL = (root) NOPASSWD: /usr/bin/nvidia-smi` to `/etc/sudoers` for Nvidia-SMI functionality to work.

# Usage / Example output from help dialog

```
./LinuxNvidiaSettingsOC.out -h
Card list:
        +---> GPU#0, bus(hex): 06:00.0, bus(dec): 6:0.0
        +---> GPU#1, bus(hex): 0a:00.0, bus(dec): 10:0.0
        +---> GPU#2, bus(hex): 0c:00.0, bus(dec): 12:0.0
        +---> GPU#3, bus(hex): 0e:00.0, bus(dec): 14:0.0
        +---> GPU#4, bus(hex): 12:00.0, bus(dec): 18:0.0

Generated xorg.conf, Xwrapper.config, and edid.bin (copy these files to /etc/X11/)
Edit xorg.conf to enable fully-headless mode (default config requires a monitor plugged into the primary/first card!).

Usage:              ./LinuxNvidiaSettingsOC.out -f <fan percentage> -l <absolute core clock> -m <memory clock offset> -i <card ID>
Usage(alt example): ./LinuxNvidiaSettingsOC.out -f <fan percentage> -c <core clock offset> -m <memory clock offset> -i <card ID>
        -f      Fan percentage (ie: 0 to 100)
                    If you do not want to set a fan speed use: -f -1
        -c      Core clock offset
        -m      Memory clock offset
        -l      Lock core clock
        -v      Lock memory clock
        -r      Reset core/memory clocks (for supplied card ID)
        -i      Card ID (ie: the value for GPU#0 would be 0)
                    You can get a list of card ID's associated to card names by running: nvidia-xconfig --query-gpu-info

        -h      Print help, list card ID's, and automatically generate Xorg config files

NOTES:
        Locked/absolute values are mutually exclusive to their offset-method counterpart (do not try to lock your core clock and set a core clock offset).

        Reset clocks before adjusting any cards which *already* have any clocks set (usage: ./LinuxNvidiaSettingsOC.out -i <card ID> -r)
        Add the following to the bottom of /etc/sudoers (required for Nvidia-SMI functionality): ml ALL = (root) NOPASSWD: /usr/bin/nvidia-smi


Please report issues/suspected bugs to the GitHub page: https://github.com/Synergyst/LinuxNvidiaSettingsOC/issues
```