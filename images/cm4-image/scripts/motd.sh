#!/bin/bash

# set the MOTD from a template
sed -e "s/%COMMIT_HASH%/${COMMIT_HASH_FIRST6}/g" \
    -e "s/%BUILD_DATE%/${BUILD_DATE}/g" \
    -e "s/%BUILD_USER%/${BUILD_USER}/g" \
    -e "s/%BUILD_DEVICE%/${BUILD_DEVICE}/g" \
    /tmp/packer-files/motd.template > /etc/motd

# set issue
cp /tmp/packer-files/issue /etc/issue
