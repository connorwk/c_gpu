# C GPU
This is a project to attempt to create and understand the logic of a simple GPU using C.
This will ultimately be used as a base to end up making a VHDL implementation then do sim in GHDL with VHPI for co-simulation and verify VHDL. Then move to an FPGA implementation.

## Dependencies
Currently I only needed the following on debian to run the output executable:
```
sudo apt-get install libsdl2-2.0-0
```
You will also need docker for the build process:
```
sudo apt-get update
sudo apt-get install apt-transport-https ca-certificates curl gnupg2 software-properties-common
curl -fsSL https://download.docker.com/linux/debian/gpg | sudo apt-key add
sudo add-apt-repository "deb [arch=amd64] https://download.docker.com/linux/debian buster stable"
sudo apt-get update
sudo apt-get install docker-ce docker-ce-cli containerd.io
```
You can check it installed properly with:
```
sudo systemctl status docker
```

## Build
There is a dockerfile to build a docker image for building to make portability and build as easy and smooth as possible since sdl2 needs a lot of dependencies.
The build process from the first time pulling down the repository is:
```
sudo docker build --tag sdl2-build-env:1.0 .
sudo ./build.sh
```
This will output an executable in the same path.
