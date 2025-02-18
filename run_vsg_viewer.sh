#!/bin/bash

# Check if a data file argument is provided
if [ -z "$1" ]; then
    echo "Usage: $0 <path-to-data-file>"
    exit 1
fi

# Start Xvfb on display :99
Xvfb :99 -screen 0 1920x1080x24 &

# Set the DISPLAY environment variable
export DISPLAY=:99

# Run your application with the provided data file
/home/ravi/dev/photon/vsgviewer/build/vsgviewer "$1"