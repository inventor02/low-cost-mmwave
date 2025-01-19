#!/bin/bash

echo 'Copy config.txt into place'
cp /tmp/packer-files/config.txt /boot/config.txt

echo 'Set hostname'
hostnamectl set-hostname webcontl
raspi-config nonint do_hostname webcontl

echo 'Set RF country'
raspi-config nonint do_wifi_country GB
