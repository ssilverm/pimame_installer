#!/bin/bash
if [ "$(id -u)" != "0" ]; then
   echo "This script must be run like: sudo ./uninstall.sh" 1>&2
   exit 1
fi

echo "Starting Removal of PiMAME :( ...."

sudo apt-get -q -y remove vsftpd imagemagick lighttpd php5-cgi

rm -r /home/pi/pimame_files 
sudo rm -r /var/www

#sudo dpkg --force-overwrite -i advancemenu_2.6-1_armhf.deb 
#sudo dpkg --force-overwrite -i advancemame-raspberrypi_1-1_armhf.deb
#sudo dpkg --force-overwrite -i gngeo_0.8-1_armhf.deb

sudo dpkg -r advancemame
sudo dpkg -r advancemenu
sudo dpkg -r gngeo


rm -r /home/pi/emulators



if sudo grep --quiet www-data /etc/sudoers; then
    sudo sed -i '/www-data/d' /etc/sudoers
else
  echo "www-data does not exist, ignoring."
fi
##############
if grep --quiet /home/pi/pimame_files/getip.sh /home/pi/.profile; then
  sed -i '/getip.sh/d' /home/pi/.profile
else
  echo "getip does not exist, ignoring."
fi
##############
if grep --quiet /home/pi/pimame_files/menu.py /home/pi/.profile; then
  sed -i '/menu.py/d' /home/pi/.profile
else
  echo "menu does not exist, ignoring."
fi

rm  -r /home/pi/pimame_installer/PiMAME



