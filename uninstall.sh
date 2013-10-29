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
sudo dpkg -r advancemess

sudo apt-get install --reinstall libsdl1.2debian

rm -rf /home/pi/emulators


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


##############pikeyd############
if grep --quiet /home/pi/pimame_files/pikeyd /home/pi/.profile; then
  sed -i '/pikeyd/d' /home/pi/.profile
else
  echo "pikeyd does not exist, ignoring"
fi

if sudo grep --quiet uinput /etc/modules; then
  sudo sed -i '/uinput/d' /etc/modules
  sudo sed -i '/i2c-dev/d' /etc/modules
else
	echo "removing modules"
fi

if sudo grep --quiet '^#blacklist i2c-bcm2708$' /etc/modprobe.d/raspi-blacklist.conf ; then
    sudo sed -i '/blacklist i2c-bcm2708/d' /etc/modprobe.d/raspi-blacklist.conf
    sudo sh -c "echo 'blacklist i2c-bcm2708' >> /etc/modprobe.d/raspi-blacklist.conf"
else
	echo "Module not blacklisted"
fi

rm  -rf /home/pi/pimame_installer/PiMAME

##############

if grep --quiet xboxdrv /home/pi/.profile; then
  sed -i '/sudo xboxdrv --silent --config /home/pi/pimame_files/xboxdrv_mapping.cfg --dbus session &/d' /home/pi/.profile
else
  echo "xboxdrv does not exist, ignoring."
fi


##############

