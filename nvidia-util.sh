#!/bin/bash

if [ ! -d "/tmp/nv-oc-util-temp" ]; then
  echo "Temporary directory not detected. Creating one."
  mkdir /tmp/nv-oc-util-temp
fi
cd /tmp/nv-oc-util-temp

generate_xorg_conf () {
  if [[ $# -ne 1 ]]; then
    echo
    echo "FATAL(INTERNAL): Not enough parameters in screen head section, exiting.."
    exit 1
  fi

  HEADSECTION=`echo -ne "Section "ServerFlags"\n        Option "BlankTime" "0"\n        Option "StandbyTime" "0"\n        Option "SuspendTime" "0"\n        Option "OffTime" "0"\nEndSection\n\nSection "ServerLayout"\n        Identifier     "Layout0"\n"`
  for ((m=0; m <= $1; m++)); do
    HEADSECTION+=`echo -ne "\n        Screen $m \"Screen$m\" 0 0\n"`
  done
  HEADSECTION+=`echo -ne "\n        InputDevice    "Mouse0" "CorePointer"\nEndSection\n\nSection "Module"\n        Disable "glx"\nEndSection
    \nSection "InputDevice"\n        Identifier     "Mouse0"\n        Driver         "mouse"\n        Option         "Protocol" "auto"\
    \n        Option         "Device" "/dev/psaux"\n        Option         "Emulate3Buttons" "no"\n        Option         "ZAxisMapping" "4 5"\nEndSection
    \nSection "InputDevice"\n        Identifier     "Keyboard0"\n        Driver         "kbd"\nEndSection\n\nSection "Monitor"\n        Identifier     "Monitor0"\n        Option         "DPMS" "0"\nEndSection\n\n\n"`
  for ((m=0; m <= $1; m++)); do
    DEVSCRSECTION+=`echo -ne "\n\nSection \"Device\"\n        Identifier     \"Device$m\"\n        Driver         \"nvidia\"\n        Option         \"Coolbits\" \"31\"\
    \n        BusID          \"PCI:"`
    DEVSCRSECTION+=$(nvidia-xconfig --query-gpu-info|grep -E 'PCI BusID'|cut -d' ' -f6|awk -F'[:]' '{ printf "%d:%d.%d\n", $2, $3, $4 }'|sed -n $(($m+1))p)
    DEVSCRSECTION+=`echo -ne "\"\n        Option         \"ConnectedMonitor\" \"DFP-$m\"\n        Option         \"CustomEDID\" \"DFP-$m:/etc/edidfake.bin\"\nEndSection\n\
    \nSection \"Screen\"\n        Identifier     \"Screen$m\"\n        Device         \"Device$m\"\n        Option         \"Coolbits\" \"31\"\n        Option         \"UseDisplayDevice\" \"none\"\nEndSection\n\n"`
  done
  HEADSECTION+="${DEVSCRSECTION}"
}

edid='AP///////wANCRu9AAAAAB4XAQOAWDJ4up2Bo1RMmSYPUFQlTwBxQIEAgUCBgJUAlQ+zAKlAZBkAQEEAJjAYiDYAoFoAAAAeAjqAGHE4LUBYLEUAoFoAAAAeAAAA/QAyPB5EDwIAICAgICAgAAAA/ABWR0EgRElTUExBWQogAa0CAxtyIwkHB4MBAABnAwwAEACAIUMBEITiAA8BHQByUdAeIG4oVQCBSQAAAB4AAAAQAAAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAvg=='
filename="NVIDIA-Linux-x86_64-510.60.02.run"

help_dialog() {
  echo -ne '\nUsage of command-line options\n\tOptions:'
  echo -ne '\n\t  drivers\tVerify driver installation and then ask to install drivers or not\n'
  echo -ne '\n\t  xorg\t\tVerify driver installation and then generate xorg.conf config files\n'
  echo -ne '\n\t  oc\t\tNot yet implemented\n'
  echo -ne '\n\t  clean\t\tRemoves the temporary storage directory (/tmp/nv-oc-util-temp)\n\n'
  exit 1
}

verify_driver_download() {
  origSum="a800dfc0549078fd8c6e8e6780efb8eee87872e6055c7f5f386a4768ce07e003"
  echo "$origSum $filename"|sha256sum --check --status
  sumResult=$?
  downloadSum=`sha256sum $filename|awk '{print $1}'`
  if [[ $sumResult == 0 ]]; then
    echo "File verified correctly. SHA256 sum matched $downloadSum"
  else
    echo "File verified incorrectly. SHA256 sum of downloaded file ($downloadSum) did not match $origSum, exiting.."
    exit 1
  fi
}

download_drivers() {
  if [ ! -f "$filename" ]; then
    wget --output-document=$filename https://us.download.nvidia.com/XFree86/Linux-x86_64/510.60.02/NVIDIA-Linux-x86_64-510.60.02.run
    chmod +x $filename
  fi
  verify_driver_download
}

install_drivers() {
  ./$filename --accept-license --no-questions --no-nouveau-check
}

verify_drivers () {
  # Step 1
  download_drivers
  ./$filename --driver-info --silent
  if [[ "$?" == "1" ]]; then
    echo "Run-file method of driver is not installed."
    echo
    echo "Checking for package manager/other driver installation method was used."
    command -v nvidia-smi
    if [[ "$?" -ne "0" ]]; then
      echo "No Nvidia drivers are likely installed."
    else
      echo "nvidia-smi was found."
      echo
      echo "Checking if nvidia-smi is functioning."
      nvidia-smi 2>&1 >/dev/null
      if [[ "$?" -ne "0" ]]; then
        echo "nvidia-smi did not return an exit code of 0, this is likely due to a bad installation of the Nvidia drivers or some mismatch of driver components."
      else
        echo
        echo "nvidia-smi returned an exit code of 0. You have a functioning installation of the Nvidia drivers already using a package manager/other driver installation method."
        echo
      fi
    fi
  else
    echo "Run-file method of driver is installed."
    echo "Assuming that everything is working.. This will be changed in the future to include checks for this method too of course."
  fi
}

case $1 in
  drivers)
    verify_drivers
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
      echo "Aborting driver installation.."
      exit 1
    fi
    install_drivers
    ;;
  xorg)
    echo "Verifying working install of Nvidia drivers.."
    verify_drivers
    num_of_cards=`nvidia-xconfig --query-gpu-info | awk 'NR==1 { print $4 }'`
    generate_xorg_conf "$(($num_of_cards-1))"
    echo "${HEADSECTION}" > /tmp/nv-oc-util-temp/xorg.conf
    echo "Generated xorg.conf file (stored at /tmp/nv-oc-util-temp/xorg.conf), exiting.."
    exit
    ;;
  cards)
    echo "Cards by decimal ID format:"
    nvidia-xconfig --query-gpu-info|grep -E 'PCI BusID'|cut -d' ' -f6|awk -F'[:]' '{ printf "%d:%d.%d\n", $2, $3, $4 }'
    echo "Cards by hex ID format:"
    nvidia-xconfig --query-gpu-info|grep -E 'PCI BusID'|cut -d' ' -f6|awk -F'[:]' '{ printf "%02x:%02x.%x\n", $2, $3, $4 }'
    ;;
  oc)
    echo
    echo "Not implemented yet, exiting.."
    exit 1
    ;;
  clean)
    if [ -d "/tmp/nv-oc-util-temp" ]; then
      rm -rf /tmp/nv-oc-util-temp
      exit $?
    else
      echo "There is nothing to cleanup, exiting.."
      exit 1
    fi
    ;;
  *)
    echo
    help_dialog
    exit 1
    ;;
esac
