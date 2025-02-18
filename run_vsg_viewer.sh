#!/bin/bash

# Start Xvfb on display :99
Xvfb :99 -screen 0 1920x1080x24 &

# Set the DISPLAY environment variable
export DISPLAY=:99

# Run your application
/home/ravi/dev/photon/vsgviewer/build/vsgviewer  ~/dev/guiTestData/12140_Skull_v3_L2.obj