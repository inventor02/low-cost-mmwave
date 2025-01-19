#!/bin/bash

COMMIT_HASH_FIRST6="$(git rev-parse --short HEAD)"
BUILD_DATE="$(date +'%F %T')"
BUILD_USER="$(whoami)"
BUILD_DEVICE="$(hostname -s)"

docker run --rm --privileged \
    -v /dev:/dev \
    -v ${PWD}:/build \
    mkaczanowski/packer-builder-arm:latest \
    build \
    --var "COMMIT_HASH_FIRST6=${COMMIT_HASH_FIRST6}" \
    --var "BUILD_DATE=${BUILD_DATE}" \
    --var "BUILD_USER=${BUILD_USER}" \
    --var "BUILD_DEVICE=${BUILD_DEVICE}" \
    pi.json