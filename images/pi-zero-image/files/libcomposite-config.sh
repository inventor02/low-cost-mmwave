#!/bin/bash

modprobe libcomposite

mkdir -p /sys/kernel/config/usb_gadget/cororanet
cd /sys/kernel/config/usb_gadget/cororanet

echo 0x1d6b > idVendor
echo 0x0104 > idProduct
echo 0x0100 > bcdDevice
echo 0x0200 > bcdUSB

mkdir -p strings/0x409
cat /sys/firmware/devicetree/base/serial-number > strings/0x409/serialnumber
echo 'University of Southampton' > strings/0x409/manufacturer
echo 'GDP53 webcontl RNDIS' > strings/0x409/product

mkdir -p configs/c.1/strings/0x409
echo 'Config 1: ECM network' > configs/c.1/strings/0x409/configuration
echo 250 > configs/c.1/MaxPower

mkdir -p functions/ecm.usb0
echo '12:1b:da:53:aa:aa' > functions/ecm.usb0/host_addr
echo '16:1b:da:53:aa:aa' > functions/ecm.usb0/dev_addr

ln -s functions/ecm.usb0 configs/c.1/

ls /sys/class/udc > UDC

nmcli connection up usb0
