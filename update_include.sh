echo "Checking update_include script"
echo "Updating..."

if [ ! -f /home/pi/roms/gridlee.zip ];
then
    echo "downloading gridlee"
    wget http://socialcase.com/raspberrypi/gridlee.zip
    mv gridlee.zip /home/pi/roms/
fi

###pisnes
wget https://pisnes.googlecode.com/files/pisnes.zip
mkdir /home/pi/emulators/pisnes
mv pisnes.zip /home/pi/emulators/pisnes
cd /home/pi/emulators/pisnes/
unzip pisnes.zip
ln -s /home/pi/emulators/pisnes/roms/ /home/pi/roms/snes
cd /home/pi/pimame_installer



###mame4all
git clone https://code.google.com/p/mame4all-pi/
mkdir /home/pi/emulators/mame4all-pi/
cp mame4all-pi/mame4all_pi.zip /home/pi/emulators/mame4all-pi/
cd /home/pi/emulators/mame4all-pi/
unzip mame4all_pi.zip
ln -s /home/pi/emulators/mame4all-pi/roms/ /home/pi/roms/mame4all
cd /home/pi/pimame_installer


cp PiMAME/pimame_files/menu.py /home/pi/pimame_files/menu.py


#increment
echo "PiMAME is now version 0.6 Beta 6.3"
