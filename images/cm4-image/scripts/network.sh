#!/bin/bash

# NM will not load connections with mode other than 0600

cp /tmp/packer-files/usb.nmconnection /etc/NetworkManager/system-connections/usb.nmconnection
chmod 0600 /etc/NetworkManager/system-connections/usb.nmconnection

cp /tmp/packer-files/usb-ethernet-config.sh /usr/local/sbin/usbethernet
cp /tmp/packer-files/usbethernet.service /etc/systemd/system/usbethernet.service
systemctl enable usbethernet.service
