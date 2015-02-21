PiMAME Installer
================

http://pimame.org

To install PiMAME on the Raspberry Pi, follow these instructions:

```sh
sudo apt-get install git
git clone https://github.com/ssilverm/pimame_installer
cd pimame_installer
sudo ./install.sh
```

Following these instructions will result in the installation of PiMAME, a Web Frontend, FTP server,
and other useful software.

After the installation process has completed, you'll need to restart the Raspberry Pi.

## Updating PiMame
To update PiMame, execute the following command:

```sh
sudo ./update.sh
```

Updating PiMame will not overwrite custom configuration files.

## Removing PiMame
To remove PiMame, execute the following command:

```sh
sudo ./uninstall.sh
```
