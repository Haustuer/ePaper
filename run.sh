#!/bin/bash
# Shell script to pull the latest changes from a Git repository

# Navigate to the repository directory (replace with your actual path)
# cd /git/ePaper

# Run the git pull command with sudo
git pull
sudo make clean
sudo make -j4
sudo ./epd 1 2
