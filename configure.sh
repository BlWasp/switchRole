#!/bin/bash

echo "Capabilities & PAM packages installation"
sudo apt-get install libcap2 || exit
sudo apt-get install libcap2-bin || exit
sudo apt-get install libcap-ng-dev || exit
sudo apt-get install libcap-dev || exit
sudo apt-get install gcc || exit
sudo apt-get install libpam0g-dev || exit

echo "Configuration files installation"
sudo cp resources/sr_pam.conf /etc/pam.d/sr || exit
sudo chmod 0644 /etc/pam.d/sr || exit
sudo cp resources/capabilityRole.conf /etc/security || exit
sudo chmod 0644 /etc/security/capabilityRole.conf || exit

echo "configuration done. Ready to compile."