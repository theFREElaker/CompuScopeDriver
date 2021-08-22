#!/bin/bash

xml_file="/usr/local/share/Gage/GageDriver.xml"
xml_backup="/usr/local/share/Gage/GageDriver.backup"
boot_driver_file="/etc/modules-load.d/gage-driver.conf"
boot_driver_backup="/etc/modules-load.d/gage-driver.backup"

fn_help()
{
	echo "Usage: ./driver.sh [OPTIONS]"
	echo "Adds or removes gage drivers to load at boot time and that are known to the gage libraries"
	echo "Drivers known to the libraries are located in $xml_file."
	echo "Drivers loaded at boot are located in $boot_driver_file."
	echo "Root privileges are required to run this script"
	echo ""
	echo "OPTIONS:"
	echo "  -a, --add      Adds driver(s) to the list. This is the default option when none are specifed"
	echo "  -h, --help     Display this help message and exits"
	echo "  -r, --remove   Removes driver(s) to the list."
	echo "      --restore  Restores the backup files. The backup files contains every drivers supported."
}

fn_kmodname()
{
	local pcname=$(echo "$1" | tr '[:upper:]' '[:lower:]')
	case "$pcname" in
		"csjohndeere" | "johndeere") echo "CscdG12" ;;
		"csrabbit"    | "rabbit"   ) echo "CsxyG8"  ;;
		"cscobramax"  | "cobramax" ) echo "CsEcdG8" ;;
		"csspider"    | "spider"   ) echo "Cs8xxx"  ;;
		"cssplenda"   | "splenda"  ) echo "Cs16xyy" ;;
		"csdecade12"  | "decade"   ) echo "CsE12xGy" ;;
		"cshexagon"   | "hexagon"  ) echo "CsE16bcd" ;;
		*                          ) exit 1
	esac
}

fn_add()
{
	local driver=$1

	if [[ $# -lt 1 ]]; then
		return 1
	fi

	if ! grep -q $driver $xml_file; then
		sed -i "/<\/gageDriver>/ s/.*/\t<driver name=\"$driver\"><\/driver>\n&/" $xml_file
	fi

	[ -f $boot_driver_file ] || echo "# Load gage drivers at boot" > $boot_driver_file
	grep -qi "$driver" $boot_driver_file || echo "$driver" >> $boot_driver_file
}

fn_remove()
{
	local driver=$1

	if [[ $# -lt 1 ]]; then
		return 1
	fi
	sed -i '/'$driver'/d' $xml_file
	sed -i '/'$driver'/d' $boot_driver_file
}

fn_create_backup()
{
	# Create a xml backup if it doesn't exists yet
	if [ ! -f $xml_backup ]; then
		cp $xml_file $xml_backup
		sed -i '/driver/d' $xml_file
	fi

	# Module load backup
	if [ ! -f $boot_driver_backup ] && [ -f $boot_driver_file ]; then
		cp $boot_driver_file $boot_driver_backup
		sed -i '/Cs/d' $boot_driver_file
	fi
}

fn_restore_backup()
{
	cp $xml_backup $xml_file
	cp $boot_driver_backup $boot_driver_file
}

fn_main()
{
	local mode="add"
	local cardnames=""
	local codenames=""

	while [ "$1" != "" ]; do
		case $1 in
		"-h" | "--help")
				fn_help
				exit 0
				;;
		"-a" | "--add") mode="add" ;;
		"-r" | "--remove") mode="remove" ;;
		"--restore")
			fn_restore_backup
			exit 0
			;;
		*)
			echo -e "\nUnknown option!\n"
			fn_help
			exit 1
			;;
		esac
		shift
	done;

	if [ $EUID -ne 0 ]; then
		echo -e "\nRoot privileges are required to run this script.\n"
		exit 1
	fi

	# The script requires an existing XML file to run
	if [ ! -f $xml_file ]; then
		echo -e "\nThe GageDriver.xml file is required to run this script.\n"
		exit 1
	fi

	fn_create_backup

	echo "Enter the card name you want to $mode, separated by space if there are multiples cards."
	echo "Accepted names :"
	echo "  - johndeere"
	echo "  - rabbit"
	echo "  - cobramax"
	echo "  - spider"
	echo "  - splenda"
	echo "  - decade"
	echo "  - hexagon"

	read -p "Selected card(s) : " cardnames

	for card in $cardnames; do
		tmp=$(fn_kmodname $card)
		if [[ $? -eq 0 ]]; then
			codenames="$codenames $tmp"
		else
			echo -e "\nInvalid codename!\n"
			exit 1
		fi
	done

	# If there is no specifed board when adding the names, and the xml file does not have any boards, restore backups
	if [[ -z "${codenames// }" ]] && [ $mode = "add" ] && ! grep -q "<driver" $xml_file; then
		fn_restore_backup
	fi

	for codename in $codenames; do
		fn_$mode $codename
	done
}

fn_main $@
