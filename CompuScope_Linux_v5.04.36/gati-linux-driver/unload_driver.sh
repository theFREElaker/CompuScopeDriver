#!/bin/sh
if [ $# -lt 1 ]; then
	echo "Usage : $0 product_code_name"
	echo "  where product_code_name must be one of the following:"
	echo "    - CsJohnDeere"
	echo "    - CsRabbit"
	echo "    - CsSpider"
	echo "    - CsSplenda"
	echo "    - CsDecade12"
	echo "    - CsHexagon"
	exit
fi
case "$1" in
	"CsJohnDeere")
		s_codename=CsJohnDeere
		s_drvname=CscdG12
		;;
	"CsRabbit")
		s_codename=CsRabbit
		s_drvname=CsxyG8
		;;
	"CsSpider")
		s_codename=CsSpider
		s_drvname=Cs8xxx
		;;
	"CsSplenda")
		s_codename=CsSplenda
		s_drvname=Cs16xyy
		;;
	"CsDecade12")
		s_codename=CsDecade12
		s_drvname=CsdcG12
		;;
	"CsHexagon")
		s_codename=CsHexagon
		s_drvname=CsdcG16
		;;
	*)
		echo "Unknown product_code_name!"
		exit
		;;
esac
s_distro=`cat /proc/version 2> /dev/null`
d_drvinst=/lib/modules/`uname -r`/extra
d_csiinst=/usr/local/share/Gage
f_autoload=/etc/sysconfig/modules/$s_drvname.modules
f_gagexml=$d_csiinst/GageDriver.xml
f_drvmod=$d_drvinst/$s_drvname.ko

sudo killall csrmd >/dev/null 2>&1
sleep 1
sudo rmmod $s_drvname >/dev/null 2>&1
sudo rm -f $f_autoload $f_gagexml >/dev/null 2>&1

