#!/bin/bash

cp /tmp/packer-files/shutdown-radarman.sh /usr/local/bin/shutdown-radarman
cp /tmp/packer-files/shutdown-radarman.service /etc/systemd/system/shutdown-radarman.service

systemctl enable shutdown-radarman.service
