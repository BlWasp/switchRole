#!/bin/bash

echo "Capabilities & PAM packages installation"
apt-get install gcc || exit
apt-get install libcap2 libcap2-bin libcap-dev libcap-ng-dev || exit
apt-get install libpam0g-dev || exit
apt-get install libxml2 libxml2-dev || exit

echo "Define root role for the user:"
echo $SUDO_USER
sudouser=$SUDO_USER
sed -i 's/ROOTADMINISTRATOR/'$sudouser'/g' ./resources/capabilityRole.xml

echo "Configuration files installation"
cp resources/sr_pam.conf /etc/pam.d/sr || exit
chmod 0644 /etc/pam.d/sr || exit
cp resources/capabilityRole.xml /etc/security || exit
chmod 0644 /etc/security/capabilityRole.xml || exit



echo "configuration done. Ready to compile."
