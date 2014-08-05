#!/bin/bash
#if [ "$(id -u)" != "0" ]; then
#   echo "This script must be run like: sudo ./install.sh" 1>&2
#   exit 1
#fi

echo "Starting Install..."

cd /home/pi
sudo apt-get update
sudo apt-get -y install vsftpd xboxdrv stella python-pip
git clone https://github.com/ssilverm/pimame-8 pimame
cd pimame
git submodule init
git submodule update
sudo pip install flask pyyaml
cp -r config/.advance/ ~/
sudo cp config/vsftpd.conf /etc/
sudo cp config/inittab /etc/
#need to add gngeo


##remove this
#cd PiMAME/
#git checkout mupen
#cd ..

# cp -r PiMAME/pimame_files /home/pi/
# cp -r PiMAME/.advance/ /home/pi/
# cp -r PiMAME/.gngeo /home/pi/
# sudo cp -r PiMAME/www/ /var/





#wget http://sheasilverman.com/rpi/raspbian/debs/advancemame_1.2-1_armhf.deb
wget http://sheasilverman.com/rpi/raspbian/debs/advancemame-raspberrypi_1-1_armhf.deb
wget http://sheasilverman.com/rpi/raspbian/debs/advancemenu_2.6-1_armhf.deb
wget http://sheasilverman.com/rpi/raspbian/gngeo_0.8-1_armhf.deb
wget http://sheasilverman.com/rpi/raspbian/debs/advancemess_0.102.0.1-1_armhf.deb
wget http://sheasilverman.com/rpi/raspbian/8/sdl2_2.0.1-1_armhf.deb
sudo dpkg --force-overwrite -i advancemenu_2.6-1_armhf.deb 
sudo dpkg --force-overwrite -i advancemame-raspberrypi_1-1_armhf.deb
sudo dpkg --force-overwrite -i gngeo_0.8-1_armhf.deb
sudo dpkg --force-overwrite -i advancemess_0.102.0.1-1_armhf.deb
sudo dpkg --force-overwrite -i sdl2_2.0.1-1_armhf.deb
sudo apt-get install stella
wget http://sheasilverman.com/rpi/raspbian/pcsx_rearmed_22042013.tgz
tar zxfv pcsx_rearmed_22042013.tgz
mkdir /home/pi/pimame/emulators
cp -r pcsx_rearmed/ /home/pi/pimame/emulators/
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
rm sdl2_2.0.1-1_armhf.deb

# mkdir /home/pi/roms
# mkdir /home/pi/.advance/image
# mkdir /home/pi/.advance/image/nes
# ln -s /home/pi/.advance/image/nes/ /home/pi/roms/nes
#mkdir /home/pi/roms/nes

###pisnes
#wget https://pisnes.googlecode.com/files/pisnes.zip
#wget http://sheasilverman.com/rpi/raspbian/installer/pisnes2013-05-25.zip
wget http://sheasilverman.com/rpi/raspbian/installer/pisnes.zip
mkdir /home/pi/pimame/emulators/pisnes
mv pisnes.zip /home/pi/pimame/emulators/pisnes
cd /home/pi/pimame/emulators/pisnes
unzip -o pisnes.zip
chmod +x /home/pi/pimame/emulators/pisnes/snes9x
chmod +x /home/pi/pimame/emulators/pisnes/snes9x.gui
cd /home/pi/pimame



###mame4all
git clone https://code.google.com/p/mame4all-pi/
mkdir /home/pi/pimame/emulators/mame4all-pi/
cp mame4all-pi/mame4all_pi.zip /home/pi/pimame/emulators/mame4all-pi/
cd /home/pi/pimame/emulators/mame4all-pi/
unzip -o mame4all_pi.zip
cd /home/pi/pimame
rm -rf mame4all-pi/

###dgen
wget http://sheasilverman.com/rpi/raspbian/installer/dgen.zip
mv dgen.zip /home/pi/pimame/emulators/dgen.zip
cd /home/pi/pimame/emulators/
unzip -o dgen.zip
mkdir /home/pi/pimame/roms/genesis
rm dgen.zip
cd /home/pi/pimame
chmod +x /home/pi/pimame/emulators/dgen-sdl-1.32/dgen


###cavestory
#wget http://sheasilverman.com/rpi/raspbian/installer/cavestory.zip
wget https://github.com/vanfanel/cavestory_rpi/archive/master.zip
mv master.zip /home/pi/pimame/emulators/cavestory.zip
cd /home/pi/pimame/emulators/
unzip -o cavestory.zip
rm cavestory.zip
# cd /home/pi/pimame_installer
# cp /home/pi/pimame_installer/config_files/cs.sh /home/pi/pimame/emulators/cs.sh
# chmod +x /home/pi/pimame/emulators/cs.sh
# chmod +x /home/pi/pimame/emulators/cavestory_rpi-master/nx

###scummvm
sudo apt-get -y install scummvm

###gameboy - thanks beta_tester
wget http://sheasilverman.com/rpi/raspbian/installer/gearboy.zip
#mkdir /home/pi/pimame/emulators/gearboy
mv gearboy.zip /home/pi/pimame/emulators/
cd /home/pi/pimame/emulators/
unzip -o gearboy.zip
rm gearboy.zip
cd /home/pi/pimame


####gameboy advance
wget http://sheasilverman.com/rpi/raspbian/installer/gpsp.zip
mkdir /home/pi/pimame/emulators/gpsp
mv gpsp.zip /home/pi/pimame/emulators/gpsp
cd /home/pi/pimame/emulators/gpsp
unzip -o gpsp.zip
rm gpsp.zip
cd /home/pi/pimame

###xboxdriver
sudo apt-get -y install xboxdrv

####c64
wget http://sheasilverman.com/rpi/raspbian/installer/vice_2.3.21-1_armhf.deb
sudo dpkg -i vice_2.3.21-1_armhf.deb
rm -rf vice_2.3.21-1_armhf.deb

####fba
wget http://sheasilverman.com/rpi/raspbian/installer/piFBA.zip
mkdir /home/pi/pimame/emulators/fba
mv piFBA.zip /home/pi/pimame/emulators/fba
cd /home/pi/pimame/emulators/fba
unzip -o piFBA.zip
rm piFBA.zip



# ####mupen
# wget http://sheasilverman.com/rpi/raspbian/mupen64plus-rpi.zip
# #mkdir /home/pi/pimame/emulators/mupen
# mv mupen64plus-rpi.zip /home/pi/pimame/emulators/
# cd /home/pi/pimame/emulators/
# unzip -o mupen64plus-rpi.zip
# rm mupen64plus-rpi.zip
# cd mupen64plus-rpi/test/
# chmod +x mupen64plus
# cd /home/pi/pimame_installer
# mkdir /home/pi/roms/n64



wget http://socialcase.com/raspberrypi/gridlee.zip
mv gridlee.zip /home/pi/pimame/roms/advmame/


if grep --quiet xboxdrv /home/pi/.profile; then
  echo "xboxdrv already exists, ignoring."
else
	echo 'sudo xboxdrv --silent --config /home/pi/pimame/config/xboxdrv_mapping.cfg --dbus session &' >> /home/pi/.profile
fi


if grep --quiet /home/pi/pimame/pimame-web-frontend /home/pi/.profile; then
  echo "website already exists, ignoring."
else
	echo 'sudo python /home/pi/pimame/pimame-web-frontend/app.py &' >> /home/pi/.profile
fi


if grep --quiet /home/pi/pimame/pimame-menu /home/pi/.profile; then
  echo "menu already exists, ignoring."
else
	echo 'cd /home/pi/pimame/pimame-menu/' >> /home/pi/.profile
	echo 'python launchmenu.py' >> /home/pi/.profile
fi




echo "Please restart to activate PiMAME :)"
