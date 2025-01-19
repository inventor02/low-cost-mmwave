#!/bin/bash

export DEBIAN_FRONTEND=noninteractive

apt-get update

apt-get install -y nginx python3 python3-pip python3-flask gunicorn
