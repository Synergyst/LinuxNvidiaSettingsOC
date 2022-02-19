# Requirements

 * Nvidia drivers (https://developer.nvidia.com/cuda-downloads?target_os=Linux&target_arch=x86_64&Distribution=Ubuntu&target_version=20.04&target_type=deb_network)
 * Visual Studio 2019 (or something else to compile the project source code)

*Compile project under the debug or release profile*

# Usage

```
./LinuxNvidiaSettingsOC.out -f <fan percentage> -c <core clock offset> -m <memory clock offset> -i <card ID>
        -f      Fan percentage (ie: 0 to 100)
                    If you do not want to set a fan speed use: -f -1
        -c      Core clock offset
        -m      Memory clock offset
        -r      Reset clocks
        -i      Card ID (ie: the value for GPU#0 would be 0)
                    You can get a list of card ID's associated to card names by running: nvidia-xconfig --query-gpu-info

        -h      Print help, list card ID's, and automatically generate Xorg config files
```