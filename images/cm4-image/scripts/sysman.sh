#!/bin/bash

cp -r /tmp/packer-files/sysman /sysman
cp /tmp/packer-files/sysman.service /etc/systemd/system/sysman.service

systemctl enable sysman.service
