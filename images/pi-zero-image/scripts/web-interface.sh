#!/bin/bash

curl -o /tmp/packer-files/web-interface.tar.gz http://10.7.4.167/web-interface/web-interface-latest.tar.gz

mkdir /web
tar -xzvf /tmp/packer-files/web-interface.tar.gz -C /web

cp /tmp/packer-files/web-wsgi.service /etc/systemd/system/web-wsgi.service
systemctl enable web-wsgi.service

cp /tmp/packer-files/nginx-webcontl /etc/nginx/sites-available/webcontl
rm /etc/nginx/sites-enabled/default
ln -s /etc/nginx/sites-available/webcontl /etc/nginx/sites-enabled/webcontl
