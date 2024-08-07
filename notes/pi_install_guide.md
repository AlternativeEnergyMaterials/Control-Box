# Control Box Raspberry Pi Software Installation
To set up a new Pi, run the commands below, in order.

## Update Pi
```
sudo apt update && sudo apt upgrade -y
```

## Update Pi settings
```
# Enable I2C
sudo raspi-config nonint do_i2c 0
sudo modprobe i2c-dev

# Update Wired connection ipv4 method
sudo nmcli c m "Wired connection 1" ipv4.method link-local
# Now disconnect and unplug and replug the ethernet cable
# Verify the connection
nmcli c show "Wired connection 1" | grep ipv4.method
# You should see "link-local"
```

## Install needed libraries
```
sudo apt install -y i2c-tools git libgpiod-dev
```

## Install PiPlates ADC library
```
sudo apt install python3-pip
pip install pi-plates --break-system-packages
```

## Install Software for Sequent Microsystem boards
### Smart Fan:
```
cd ~/
git clone https://github.com/SequentMicrosystems/SmartFan-rpi.git
cd ~/SmartFan-rpi
sudo make install
```

### 8 MOSFETs:
```
cd ~/
git clone https://github.com/SequentMicrosystems/8mosind-rpi.git
cd ~/8mosind-rpi
sudo make install
 ```

### 16 relays:
```
cd ~/
git clone https://github.com/SequentMicrosystems/16relind-rpi.git
cd ~/16relind-rpi
sudo make install
 ```

### Thermocouples:
```
cd ~/
git clone https://github.com/SequentMicrosystems/smtc-rpi.git
cd ~/smtc-rpi
sudo make install
```

## Install AEM Software
```
# TODO: clone test stand code from AEM repo
```
