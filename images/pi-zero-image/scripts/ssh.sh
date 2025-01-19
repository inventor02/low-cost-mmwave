#!/bin/bash

rm /etc/ssh/sshd_config.d/rename_user.conf
cp /tmp/packer-files/10-root-ssh.conf /etc/ssh/sshd_config.d/10-root-ssh.conf

systemctl enable ssh.service
