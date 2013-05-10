echo "Checking update_include script"
echo "Updating..."

if [ ! -f /home/pi/roms/gridlee.zip ];
then
    echo "downloading gridlee"
    wget http://socialcase.com/raspberrypi/gridlee.zip
    mv gridlee.zip /home/pi/roms/
fi


#increment
echo "PiMAME is now version 0.6 Beta 6.2"
