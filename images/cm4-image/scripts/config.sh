#!/bin/bash

echo 'Copy config.txt into place'
cp /tmp/packer-files/config.txt /boot/config.txt

echo 'Set hostname'
hostnamectl set-hostname radarman
raspi-config nonint do_hostname radarman
