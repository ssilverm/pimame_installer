#!/bin/bash
if [ "$(id -u)" != "0" ]; then
   echo "This script must be run like: sudo ./update.sh" 1>&2
   exit 1
fi

echo "Starting Update..."

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

sudo cp -r PiMAME/www/ /var/

sudo /etc/init.d/lighttpd force-reload





