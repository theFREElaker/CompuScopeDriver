#!/bin/bash

os=$(lsb_release -i)
echo "$os"
if echo "$os" | grep -qi "centos" || echo "$os" | grep -qi "redhat"; then
	sudo yum install -y qt5-qtbase qt5-qtbase-devel
	qmake-qt5 cstestqt.pro && make
	sudo install -m 755 "cstestqt" "/usr/local/bin"

elif echo "$os" | grep -qi "ubuntu"; then
	sudo apt-get install -y qt5-default
	qmake cstestqt.pro && make
	sudo install -m 755 "cstestqt" "/usr/local/bin"
else
	echo -e "\nUnsupported OS...\n"
	exit 1
fi
		
