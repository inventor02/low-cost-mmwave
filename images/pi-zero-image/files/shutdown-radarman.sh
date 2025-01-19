#!/bin/bash

curl http://172.16.0.200:8010/shutdown
sleep 20

echo 'radarman should be off now'
