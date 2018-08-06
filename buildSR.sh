#!/bin/bash

echo "Installation librairies capabilities"
sudo apt-get install libcap2 || exit
sudo apt-get install libcap2-bin || exit
sudo apt-get install libcap-ng-dev || exit
sudo apt-get install libcap-dev || exit

echo "Installation PAM"
sudo apt-get install libpam0g-dev || exit

gcc -fPIC -fno-stack-protector -c srFolder/pam_moduleSR.c || exit

sudo ld -x --shared -o /lib/x86_64-linux-gnu/security/pam_moduleSR.so pam_moduleSR.o || exit

rm pam_moduleSR.o || exit

sudo gcc -o /usr/bin/sr srFolder/sr.c -lpam -lpam_misc -lcap -lcap-ng || exit
sudo setcap cap_dac_override,cap_setpcap,cap_setfcap+p /usr/bin/sr || exit
sudo gcc -o /usr/bin/sr_aux srFolder/sr_aux.c -lcap -lcap-ng || exit

sudo cp srFolder/sr /etc/pam.d || exit
sudo chmod 644 /etc/pam.d/sr || exit

sudo cp capabilityRole.conf /etc/security || exit
sudo chmod 644 /etc/security/capabilityRole.conf || exit
rm capabilityRole.conf || exit

sudo chmod 705 $PWD

echo "Please restart your computer to make changes effective"
