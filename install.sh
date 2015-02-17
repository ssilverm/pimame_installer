#!/bin/bash
#if [ "$(id -u)" != "0" ]; then
#   echo "This script must be run like: sudo ./install.sh" 1>&2
#   exit 1
#fi

echo "THIS IS THE OLD VERSION OF PIMAME AND IS NOT SUPPORTED!!!!!!"
echo "REMOVE THE exit 1 from this line to continue"
exit 1

echo "Starting Install..."
sudo apt-get update
sudo addgroup --system www-data
sudo adduser www-data www-data
sudo apt-get -q -y install build-essential automake libmpg123-dev libmodplug-dev libphysfs-dev libfreetype6-dev libdevil-dev liblua5.1-0-dev libopenal-dev libsdl1.2-dev libvorbis-dev vsftpd imagemagick lighttpd php5-cgi git
sudo lighty-enable-mod cgi

sudo cp config_files/vsftpd.conf /etc/vsftpd.conf
sudo cp config_files/php.ini /etc/php5/cgi/php.ini
sudo cp config_files/lighttpd.conf /etc/lighttpd/lighttpd.conf
sudo cp config_files/10-cgi.conf /etc/lighttpd/conf-enabled/10-cgi.conf
sudo cp config_files/inittab /etc/

git clone https://github.com/ssilverm/PiMAME.git


##remove this
#cd PiMAME/
#git checkout mupen
#cd ..

cp -r PiMAME/pimame_files /home/pi/
cp -r PiMAME/.advance/ /home/pi/
cp -r PiMAME/.gngeo /home/pi/
sudo cp -r PiMAME/www/ /var/


sudo /etc/init.d/lighttpd force-reload


#wget http://sheasilverman.com/rpi/raspbian/debs/advancemame_1.2-1_armhf.deb
wget http://sheasilverman.com/rpi/raspbian/debs/advancemame-raspberrypi_1-1_armhf.deb
wget http://sheasilverman.com/rpi/raspbian/debs/advancemenu_2.6-1_armhf.deb
wget http://sheasilverman.com/rpi/raspbian/gngeo_0.8-1_armhf.deb
wget http://sheasilverman.com/rpi/raspbian/debs/advancemess_0.102.0.1-1_armhf.deb
sudo dpkg --force-overwrite -i advancemenu_2.6-1_armhf.deb 
sudo dpkg --force-overwrite -i advancemame-raspberrypi_1-1_armhf.deb
sudo dpkg --force-overwrite -i gngeo_0.8-1_armhf.deb
sudo dpkg --force-overwrite -i advancemess_0.102.0.1-1_armhf.deb
sudo apt-get install stella
wget http://sheasilverman.com/rpi/raspbian/pcsx_rearmed_22042013.tgz
tar zxfv pcsx_rearmed_22042013.tgz
mkdir /home/pi/emulators
cp -r pcsx_rearmed/ /home/pi/emulators/
#sudo chown -R pi:pi /home/pi/emulators
#sudo chown -R pi:pi /home/pi/pimame_files

chmod +x /home/pi/pimame_files/getip.sh

#rm advancemame_1.2-1_armhf.deb
rm advancemame-raspberrypi_1-1_armhf.deb 
rm advancemenu_2.6-1_armhf.deb 
rm pcsx_rearmed_22042013.tgz
rm -rf pcsx_rearmed
rm gngeo_0.8-1_armhf.deb
rm advancemess_0.102.0.1-1_armhf.deb

mkdir /home/pi/roms
mkdir /home/pi/.advance/image
mkdir /home/pi/.advance/image/nes
ln -s /home/pi/.advance/image/nes/ /home/pi/roms/nes
#mkdir /home/pi/roms/nes

###pisnes
#wget https://pisnes.googlecode.com/files/pisnes.zip
#wget http://sheasilverman.com/rpi/raspbian/installer/pisnes2013-05-25.zip
wget http://sheasilverman.com/rpi/raspbian/installer/pisnes.zip
mkdir /home/pi/emulators/pisnes
mv pisnes.zip /home/pi/emulators/pisnes
cd /home/pi/emulators/pisnes
unzip -o pisnes.zip
chmod +x /home/pi/emulators/pisnes/snes9x
chmod +x /home/pi/emulators/pisnes/snes9x.gui
ln -s /home/pi/emulators/pisnes/roms/ /home/pi/roms/snes
cd /home/pi/pimame_installer



###mame4all
git clone https://code.google.com/p/mame4all-pi/
mkdir /home/pi/emulators/mame4all-pi/
cp mame4all-pi/mame4all_pi.zip /home/pi/emulators/mame4all-pi/
cd /home/pi/emulators/mame4all-pi/
unzip -o mame4all_pi.zip
ln -s /home/pi/emulators/mame4all-pi/roms/ /home/pi/roms/mame4all
cd /home/pi/pimame_installer
rm -rf mame4all-pi/

###dgen
wget http://sheasilverman.com/rpi/raspbian/installer/dgen.zip
mv dgen.zip /home/pi/emulators/dgen.zip
cd /home/pi/emulators/
unzip -o dgen.zip
mkdir /home/pi/roms/genesis
rm dgen.zip
cd /home/pi/pimame_installer
chmod +x /home/pi/emulators/dgen-sdl-1.32/dgen


###cavestory
#wget http://sheasilverman.com/rpi/raspbian/installer/cavestory.zip
wget https://github.com/vanfanel/cavestory_rpi/archive/master.zip
mv master.zip /home/pi/emulators/cavestory.zip
cd /home/pi/emulators/
unzip -o cavestory.zip
rm cavestory.zip
cd /home/pi/pimame_installer
cp /home/pi/pimame_installer/config_files/cs.sh /home/pi/emulators/cs.sh
chmod +x /home/pi/emulators/cs.sh
chmod +x /home/pi/emulators/cavestory_rpi-master/nx

###scummvm
sudo apt-get -y install scummvm

###gameboy - thanks beta_tester
wget http://sheasilverman.com/rpi/raspbian/installer/gearboy.zip
#mkdir /home/pi/emulators/gearboy
mv gearboy.zip /home/pi/emulators/
cd /home/pi/emulators/
unzip -o gearboy.zip
rm gearboy.zip
cd /home/pi/pimame_installer
mkdir /home/pi/roms/gameboy

####gameboy advance
wget http://sheasilverman.com/rpi/raspbian/installer/gpsp.zip
mkdir /home/pi/emulators/gpsp
mv gpsp.zip /home/pi/emulators/gpsp
cd /home/pi/emulators/gpsp
unzip -o gpsp.zip
rm gpsp.zip
cd /home/pi/pimame_installer
mkdir /home/pi/roms/gameboyadvance

###xboxdriver
sudo apt-get -y install xboxdrv

####c64
wget http://sheasilverman.com/rpi/raspbian/installer/vice_2.3.21-1_armhf.deb
sudo dpkg -i vice_2.3.21-1_armhf.deb
rm -rf vice_2.3.21-1_armhf.deb

####fba
wget http://sheasilverman.com/rpi/raspbian/installer/piFBA.zip
mkdir /home/pi/emulators/fba
mv piFBA.zip /home/pi/emulators/fba
cd /home/pi/emulators/fba
unzip -o piFBA.zip
rm piFBA.zip
cd /home/pi/pimame_installer
ln -s /home/pi/emulators/fba/roms/ /home/pi/roms/fba


####mupen
wget http://sheasilverman.com/rpi/raspbian/mupen64plus-rpi.zip
#mkdir /home/pi/emulators/mupen
mv mupen64plus-rpi.zip /home/pi/emulators/
cd /home/pi/emulators/
unzip -o mupen64plus-rpi.zip
rm mupen64plus-rpi.zip
cd mupen64plus-rpi/test/
chmod +x mupen64plus
cd /home/pi/pimame_installer
mkdir /home/pi/roms/n64

###dispmanx
#wget http://sheasilverman.com/rpi/raspbian/installer/SDL12-kms-dispmanx.zip
#unzip -o SDL12-kms-dispmanx.zip
#cd SDL12-kms-dispmanx
#sudo make install
#cd ..
#rm SDL12-kms-dispmanx.zip
#rm -rf SDL12-kms-dispmanx/

wget http://socialcase.com/raspberrypi/gridlee.zip
mv gridlee.zip /home/pi/roms/

if sudo grep --quiet www-data /etc/sudoers; then
  echo "www-data already in sudoers, ignoring."
else
  sudo sh -c "echo 'www-data ALL=(ALL) NOPASSWD: ALL' >> /etc/sudoers"
fi
##############
if grep --quiet /home/pi/pimame_files/getip.sh /home/pi/.profile; then
  echo "getip already exists, ignoring."
else
	echo '/home/pi/pimame_files/getip.sh' >> /home/pi/.profile
fi
##############
if grep --quiet xboxdrv /home/pi/.profile; then
  echo "xboxdrv already exists, ignoring."
else
	echo 'sudo xboxdrv --silent --config /home/pi/pimame_files/xboxdrv_mapping.cfg --dbus session &' >> /home/pi/.profile
fi
##############
if grep --quiet /home/pi/pimame_files/menu.py /home/pi/.profile; then
  echo "menu already exists, ignoring."
else
echo 'python /home/pi/pimame_files/menu.py' >> /home/pi/.profile
fi


##############pikeyd############
#wget http://sheasilverman.com/rpi/raspbian/installer/pikeyd.zip
#mv pikeyd.zip /home/pi/pimame_files
#cd /home/pi/pimame_files
#unzip -o pikeyd.zip
#mv pikeyd/pikeyd.conf ~/.pikeyd.conf
#if grep --quiet /home/pi/pimame_files/pikeyd /home/pi/.profile; then
#  echo "pikeyd already exists, ignoring."
#else
#  echo '/home/pi/pimame_files/pikeyd/pikeyd -d' >> /home/pi/.profile
#fi

#if sudo grep --quiet uinput /etc/modules; then
#  echo "Modules have already been added"
#else
#	sudo sh -c "echo 'uinput' >> /etc/modules"
#	sudo sh -c "echo 'i2c-dev' >> /etc/modules"
#fi

#if sudo grep --quiet '^blacklist i2c-bcm2708$' /etc/modprobe.d/raspi-blacklist.conf ; then
#    echo "Blacklisting i2c-bcm2708"
#    sudo sed -i '/blacklist i2c-bcm2708/d' /etc/modprobe.d/raspi-blacklist.conf
#    sudo sh -c "echo '#blacklist i2c-bcm2708' >> /etc/modprobe.d/raspi-blacklist.conf"
#else
#	echo "Module already blacklisted"
#fi
###########

#sudo chown -R pi:pi /home/pi/roms
rm  -rf /home/pi/pimame_installer/PiMAME
#sudo echo 'www-data ALL=(ALL) NOPASSWD: ALL' >> /etc/sudoers
#echo '/home/pi/pimame_files/getip.sh' >> /home/pi/.profile
#echo 'python /home/pi/pimame_files/menu.py' >> /home/pi/.profile

echo "Please restart to activate PiMAME :)"
