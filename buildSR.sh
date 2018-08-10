#!/bin/bash

echo "Installation capabilities librairies"
sudo apt-get install libcap2 || exit
sudo apt-get install libcap2-bin || exit
sudo apt-get install libcap-ng-dev || exit
sudo apt-get install libcap-dev || exit

echo "Installation PAM"
sudo apt-get install libpam0g-dev || exit

echo "Installation sr"
sudo gcc -o /usr/bin/sr srFolder/sr.c -lpam -lpam_misc -lcap -lcap-ng || exit
sudo setcap cap_dac_override,cap_setpcap,cap_setfcap+p /usr/bin/sr || exit
sudo gcc -o /usr/bin/sr_aux srFolder/sr_aux.c -lcap -lcap-ng || exit

echo "Installation PAM file"
sudo cp srFolder/sr /etc/pam.d || exit
sudo chmod 644 /etc/pam.d/sr || exit

echo "Installation capabilities configuration file"
sudo cp capabilityRole.conf /etc/security || exit
sudo chmod 644 /etc/security/capabilityRole.conf || exit

echo "Remove useless installation file"
rm capabilityRole.conf || exit
rm srFolder/sr || exit
