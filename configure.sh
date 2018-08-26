#!/bin/bash

echo "Capabilities & PAM packages installation"
sudo apt-get install gcc || exit
sudo apt-get install libcap2 libcap2-bin libcap-dev libcap-ng-dev || exit
sudo apt-get install libpam0g-dev || exit
sudo apt-get install libxml2 libxml2-dev || exit

echo "Configuration files installation"
sudo cp resources/sr_pam.conf /etc/pam.d/sr || exit
sudo chmod 0644 /etc/pam.d/sr || exit
sudo cp resources/capabilityRole.xml /etc/security || exit
sudo chmod 0644 /etc/security/capabilityRole.xml || exit

echo "configuration done. Ready to compile."
