#!/bin/bash
if [ "$(id -u)" != "0" ]; then
   echo "This script must be run like: sudo ./install.sh" 1>&2
   exit 1
fi

echo "Starting Update..."


Update should git pull latest code
copy all config files to a backup file location timestamped by current date
change all files to be blah.default


sudo apt-get update

git pull
git clone https://github.com/ssilverm/PiMAME.git
cd PiMAME
PREVGITFILE_mame= git show HEAD~1:.advance/advmame.rc |  md5
CURRENTFILE_mame= /home/pi/.advance/advmame.rc |  md5
if $PREVGITFILE_mame == CURRENTFILE_mame; then
	#DO COPY STUFF HERE
	echo "Default AdvMAME configuration, updating."
	cp .advance/advmame.rc.default /home/pi/.advance/advmame.rc
else
	echo "AdvMAME config file has been changed.  Not overwriting.  Current Version in advmame.rc.default"
	cp .advance/advmame.rc.default /home/pi/.advance/advmame.rc.default
fi

PREVGITFILE_menu= git show HEAD~1:.advance/advmenu.rc |  md5
CURRENTFILE_menu= /home/pi/.advance/advmenu.rc |  md5
if $PREVGITFILE_me == CURRENTFILE_me; then
	#DO COPY STUFF HERE
	echo "Default AdvMENU configuration, updating."
	cp .advance/advmenu.rc.default /home/pi/.advance/advmenu.rc
else
	echo "AdvMENU config file has been changed.  Not overwriting.  Current Version in advmenu.rc.default"
	cp .advance/advmenu.rc.default /home/pi/.advance/advmenu.rc.default
fi

PREVGITFILE_neo= git show HEAD~1:.gngeo/gngeorc |  md5
CURRENTFILE_neo= /home/pi/.gngeo/gngeorc |  md5
if $PREVGITFILE_mame == CURRENTFILE_mame; then
	#DO COPY STUFF HERE
	echo "Default GNGeo configuration, updating."
	cp .gngeo/gngeorc.default /home/pi/.gngeo/gngeorc
else
	echo "GNGeo config file has been changed.  Not overwriting.  Current Version in gngeorc.default"
	cp .gngeo/gngeorc.default /home/pi/.gngeo/gngeorc.default
fi


sudo cp config_files/vsftpd.conf /etc/vsftpd.conf
sudo cp config_files/php.ini /etc/php5/cgi/php.ini
sudo cp config_files/lighttpd.conf /etc/lighttpd/lighttpd.conf
sudo cp config_files/10-cgi.conf /etc/lighttpd/conf-enabled/10-cgi.conf
sudo cp config_files/inittab /etc/

git clone https://github.com/ssilverm/PiMAME.git
cp -r PiMAME/pimame_files /home/pi/
cp -r PiMAME/.advance/ /home/pi/
cp -r PiMAME/.gngeo /home/pi/
sudo cp -r PiMAME/www/ /var/


sudo /etc/init.d/lighttpd force-reload


wget http://sheasilverman.com/rpi/raspbian/debs/advancemame_1.2-1_armhf.deb
wget http://sheasilverman.com/rpi/raspbian/debs/advancemenu_2.6-1_armhf.deb
wget http://sheasilverman.com/rpi/raspbian/gngeo_0.8-1_armhf.deb
sudo dpkg --force-overwrite -i advancemenu_2.6-1_armhf.deb 
sudo dpkg --force-overwrite -i advancemame_1.2-1_armhf.deb 
sudo dpkg --force-overwrite -i gngeo_0.8-1_armhf.deb

wget http://sheasilverman.com/rpi/raspbian/pcsx_rearmed_22042013.tgz
tar zxfv pcsx_rearmed_22042013.tgz
mkdir /home/pi/emulators
cp -r pcsx_rearmed/ /home/pi/emulators/
sudo chown -R pi:pi /home/pi/emulators
sudo chown -R pi:pi /home/pi/pimame_files

chmod +x /home/pi/pimame_files/getip.sh

rm advancemame_1.2-1_armhf.deb 
rm advancemenu_2.6-1_armhf.deb 
rm pcsx_rearmed_22042013.tgz
rm -r pcsx_rearmed
rm gngeo_0.8-1_armhf.deb



if sudo grep --quiet www-data /etc/sudoers; then
  echo "www-data already in sudoers, ignoring."
else
  sudo echo 'www-data ALL=(ALL) NOPASSWD: ALL' >> /etc/sudoers
fi
##############
if grep --quiet /home/pi/pimame_files/getip.sh /home/pi/.profile; then
  echo "getip already exists, ignoring."
else
	echo '/home/pi/pimame_files/getip.sh' >> /home/pi/.profile
fi
##############
if grep --quiet /home/pi/pimame_files/menu.py /home/pi/.profile; then
  echo "menu already exists, ignoring."
else
echo 'python /home/pi/pimame_files/menu.py' >> /home/pi/.profile
fi

mkdir /home/pi/roms
sudo chown -R pi:pi /home/pi/roms
rm  -r /home/pi/pimame_installer/PiMAME
#sudo echo 'www-data ALL=(ALL) NOPASSWD: ALL' >> /etc/sudoers
#echo '/home/pi/pimame_files/getip.sh' >> /home/pi/.profile
#echo 'python /home/pi/pimame_files/menu.py' >> /home/pi/.profile



