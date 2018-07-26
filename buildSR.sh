#!/bin/bash

echo "Installation librairies capabilities"
sudo apt-get install libcap2
sudo apt-get install libcap2-bin
sudo apt-get install libcap-ng-dev
sudo apt-get install libcap-dev

echo "Installation PAM"
# wget linux-pam.org/library/Linux-PAM-1.3.0.tar.bz2
# tar -xvf Linux-PAM-1.3.0.tar.bz2
# cd Linux-PAM-1.3.0
# ./configure
# make
# make check
# sudo make install
# sudo make xtests
# cd ..
sudo apt-get install libpam0g-dev

gcc -fPIC -fno-stack-protector -c srFolder/pam_moduleSR.c

sudo ld -x --shared -o /lib/x86_64-linux-gnu/security/pam_moduleSR.so pam_moduleSR.o

rm pam_moduleSR.o

gcc -o sr srFolder/sr.c -lpam -lpam_misc -lcap -lcap-ng
sudo setcap cap_dac_override,cap_setpcap+ep sr
gcc -o sr_aux srFolder/sr_aux.c -lcap -lcap-ng
sudo setcap cap_setfcap+ep /sbin/setcap

sudo cp srFolder/sr /etc/pam.d
sudo chmod 644 /etc/pam.d/sr

sudo cp capabilityRole.conf /etc/security
sudo chmod 644 /etc/security/capabilityRole.conf

echo "Please restart your computer to make changes effective"
