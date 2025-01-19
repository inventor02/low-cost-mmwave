#!/bin/bash

cp /tmp/packer-files/libcomposite-config.sh /usr/local/sbin/usbethernet
chmod +x /usr/local/sbin/usbethernet

cp /tmp/packer-files/usbethernet.service /etc/systemd/system/usbethernet.service

systemctl enable usbethernet
