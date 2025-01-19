#!/bin/bash

# NM will not load connections with mode other than 0600

cp /tmp/packer-files/hotspot.nmconnection /etc/NetworkManager/system-connections/hotspot.nmconnection
chmod 0600 /etc/NetworkManager/system-connections/hotspot.nmconnection

cp /tmp/packer-files/usb.nmconnection /etc/NetworkManager/system-connections/usb.nmconnection
chmod 0600 /etc/NetworkManager/system-connections/usb.nmconnection
