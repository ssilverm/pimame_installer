#!/bin/bash
#if [ "$(id -u)" != "0" ]; then
#   echo "This script must be run like: sudo ./install.sh" 1>&2
#   exit 1
#fi

echo "Starting Install..."

sudo apt-get update
sudo apt-get -y install vsftpd xboxdrv stella python-pip python-requests python-levenshtein libsdl1.2-dev bc gunicorn
sudo apt-get -y dist-upgrade --force-yes
sudo apt-get -y upgrade --force-yes
sudo apt-get -y autoremove --purge
sudo apt-get -y autoclean
git clone https://github.com/ssilverm/pimame-8 pimame
cd pimame
git submodule init
git submodule update
sudo pip install flask pyyaml
cp -r config/.advance/ ~/
sudo cp config/vsftpd.conf /etc/
sudo cp config/inittab /etc/

###SDL
wget http://sheasilverman.com/rpi/raspbian/8/sdl2_2.0.1-1_armhf.deb
sudo dpkg --force-overwrite -i sdl2_2.0.1-1_armhf.deb
rm sdl2_2.0.1-1_armhf.deb

###Atari 2600
sudo apt-get install stella

###Playstation
wget http://sheasilverman.com/rpi/raspbian/pcsx_rearmed_22042013.tgz
tar zxfv pcsx_rearmed_22042013.tgz
mkdir /home/pi/pimame/emulators
cp -r pcsx_rearmed/ /home/pi/pimame/emulators/
rm pcsx_rearmed_22042013.tgz
rm -rf pcsx_rearmed

###PiSNES
wget http://sheasilverman.com/rpi/raspbian/installer/pisnes.zip
mkdir /home/pi/pimame/emulators/pisnes
mv pisnes.zip /home/pi/pimame/emulators/pisnes
cd /home/pi/pimame/emulators/pisnes
unzip -o pisnes.zip
chmod +x /home/pi/pimame/emulators/pisnes/snes9x
chmod +x /home/pi/pimame/emulators/pisnes/snes9x.gui
cd /home/pi/pimame

###DGEN
git clone https://github.com/ssilverm/dgen-sdl /home/pi/pimame/emulators/dgen-sdl-1.32

###Cavestory
git clone https://github.com/ssilverm/cavestory_rpi /home/pi/pimame/emulators/cavestory_rpi-master

###Mednafen
git clone https://github.com/ssilverm/mednafen-dispmanx-sdl /home/pi/pimame/emulators/mednafen

###DISPMANX
git clone https://github.com/ssilverm/SDL12-kms-dispmanx /home/pi/pimame/dispmanx

###Controller Config
git clone https://github.com/ssilverm/controller-setup /home/pi/pimame/controller-setup

###ScummVM
sudo apt-get -y install scummvm

###Gameboy Advance
wget http://sheasilverman.com/rpi/raspbian/installer/gpsp.zip
mkdir /home/pi/pimame/emulators/gpsp
mv gpsp.zip /home/pi/pimame/emulators/gpsp
cd /home/pi/pimame/emulators/gpsp
unzip -o gpsp.zip
rm gpsp.zip
ln -s /home/pi/pimame/roms/gba/gba_bios.bin gba_bios.bin
cd /home/pi/pimame

###xboxdrv
sudo apt-get -y install xboxdrv

###C64
wget http://sheasilverman.com/rpi/raspbian/installer/vice_2.3.21-1_armhf.deb
sudo dpkg -i vice_2.3.21-1_armhf.deb
rm -rf vice_2.3.21-1_armhf.deb

###PiFBA
git clone https://github.com/ssilverm/pifba-config-patch /home/pi/pimame/emulators/fba
cd /home/pi/pimame

###NES
wget http://pimame.org/8files/fceux.zip
mkdir /home/pi/pimame/emulators/fceux
mv fceux.zip /home/pi/pimame/emulators/fceux
cd /home/pi/pimame/emulators/fceux
unzip -o fceux.zip
rm fceux.zip
cd /home/pi/pimame 

###ZX Spectrum
cd /home/pi/pimame/emulators/
wget http://pimame.org/8files/unreal-speccy-portable_0.0.43_rpi.zip
unzip -o unreal-speccy-portable_0.0.43_rpi.zip
rm unreal-speccy-portable_0.0.43_rpi.zip
cd /home/pi/pimame/

echo 'if [ "$DISPLAY" == "" ] && [ "$SSH_CLIENT" == "" ] && [ "$SSH_TTY" == "" ]; then' >> /home/pi/.profile

if grep --quiet /home/pi/pimame/pimame-web-frontend /home/pi/.profile; then
  echo "Web server already exists, ignoring."
else
	echo 'cd /home/pi/pimame/pimame-web-frontend/; sudo gunicorn app:app -b 0.0.0.0:80 &' >> /home/pi/.profile
fi


if grep --quiet /home/pi/pimame/pimame-menu /home/pi/.profile; then
  echo "PiPLAY menu already exists, ignoring."
else
	echo 'cd /home/pi/pimame/pimame-menu/' >> /home/pi/.profile
	echo 'python launchmenu.py' >> /home/pi/.profile
fi

echo 'fi' >> /home/pi/.profile


echo "Please restart to run PiPLAY :)"