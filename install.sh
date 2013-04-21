
#!/bin/bash
if [ "$(id -u)" != "0" ]; then
   echo "This script must be run like: sudo ./install.sh" 1>&2
   exit 1
fi

echo "Starting Install..."
sudo apt-get update
sudo addgroup --system www-data
sudo adduser www-data www-data
sudo apt-get install build-essential automake libmpg123-dev libmodplug-dev libphysfs-dev libfreetype6-dev libdevil-dev liblua5.1-0-dev libopenal-dev libsdl1.2-dev libvorbis-dev vsftpd imagemagick lighttpd php5-cgi git
sudo lighty-enable-mod cgi

sudo cp config_files/vsftpd.conf /etc/vsftpd.conf
sudo cp config_files/php.ini /etc/php5/cgi/php.ini
sudo cp config_files/lighttpd.conf /etc/lighttpd/lighttpd.conf
sudo cp config_files/10-cgi.conf /etc/lighttpd/conf-enabled/10-cgi.conf
sudo cp config_files/inittab /etc/

git clone https://github.com/ssilverm/PiMAME.git
cp -r PiMAME/pimame_files /home/pi/
cp -r PiMAME/.advance/ /home/pi/
sudo cp -r PiMAME/www/ /var/


sudo /etc/init.d/lighttpd force-reload


wget http://sheasilverman.com/rpi/raspbian/debs/advancemame_1.2-1_armhf.deb
wget http://sheasilverman.com/rpi/raspbian/debs/advancemenu_2.6-1_armhf.deb
sudo dpkg --force-overwrite -i advancemenu_2.6-1_armhf.deb 
sudo dpkg --force-overwrite -i advancemame_1.2-1_armhf.deb 

wget http://sheasilverman.com/rpi/raspbian/pcsx_rearmed_19042013.zip
unzip pcsx_rearmed_19042013.zip 
mkdir /home/pi/emulators
cp -r pcsx_rearmed/ /home/pi/emulators/
sudo chown -R pi:pi /home/pi/emulators
sudo chown -R pi:pi /home/pi/pimame_files

chmod +x /home/pi/pimame_files/getip.sh

rm advancemame_1.2-1_armhf.deb 
rm advancemenu_2.6-1_armhf.deb 
rm pcsx_rearmed_19042013.zip 
rm -r pcsx_rearmed

wget http://sheasilverman.com/rpi/raspbian/gngeo_install.zip
unzip gngeo_install.zip
cp gngeo/gngeo /usr/local/bin/gngeo
mkdir /usr/local/share/gngeo
cp gngeoFolder/gngeo_data.zip /usr/local/share/gngeo/gngeo_data.zip
cp gngeo/gngeo.1 /usr/local/share/man/man1/gngeo.1



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
rm  -r /home/pi/python_installer/PiMAME
#sudo echo 'www-data ALL=(ALL) NOPASSWD: ALL' >> /etc/sudoers
#echo '/home/pi/pimame_files/getip.sh' >> /home/pi/.profile
#echo 'python /home/pi/pimame_files/menu.py' >> /home/pi/.profile



