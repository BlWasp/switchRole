#! /bin/bash

sed '$d' /home/$1/.bashrc > .bashrc
cp .bashrc /home/$1/
rm .bashrc