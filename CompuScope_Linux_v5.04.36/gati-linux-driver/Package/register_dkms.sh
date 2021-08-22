#!/bin/bash

from_rpm=false
distro=""

fn_usage()
{
	echo "Usage: ./register_dkms.sh [add|remove] [module_version] {--fromRPM}"
	echo -e "    add              Adds the module to the dkms tree."
	echo -e "    remove           Removes the module to the dkms tree."
	echo -e "    module_version   The version of the module to register to dkms."
	echo -e "    --fromRPM        Optionnal. Will not install any required packages if they"
	echo -e "                     are missing. Will exit witout sending an error code upon"
	echo -e "                     failure."
}

fn_verify_repo()
{
	if [ ! $# -ge 1 ]; then
		echo "A package name is required"
		return 2
	fi

	local retVal=0

	if [ $distro = "redhat" ]; then
		if rpm -qa | grep -qi $1; then
			return 0
		else
			return 1
		fi
	else
		if dpkg -l | grep -qi $1; then
			return 0
		else
			return 1
		fi
	fi
}

fn_install_package()
{
	local package_name=""

	if [ $# -ge 1 ]; then
		package_name=$1
	fi

	if [ $# -ge 2 ]; then
		package_name="$package_name-$2"
	fi

	if [ $package_name = "" ]; then
		return 1
	fi

	if [ $distro = "redhat" ]; then
		sudo yum -qy install $package_name
	else
		sudo apt -y install $package_name
	fi
}

fn_prompt_install_package()
{
	if [ $# = 0 ]; then 
		return 1
	fi

	while true; do
		read -p "Package \"$1\" is required. Do you wish to install this package? (Y/N) " yn
		case $yn in
			[Yy]* )
				fn_install_package $1 $2;
				break;;
			[Nn]* )
				return 1;;
			*)
				echo "Please answer yes or no.";;
		esac
	done
}

fn_verify_package()
{
	local package_installed=0
	local promted_succeeded=0

	if [ $# -eq 0 ]; then
		return 1
	fi

	fn_verify_repo $1
	package_installed=$?

	if [ $package_installed -eq 1 ] ; then
		if $from_rpm; then
			exit 0
		fi
		fn_prompt_install_package $1 $2
		promted_succeeded=$?

		if [ $promted_succeeded -ne 0 ]; then
			return 1
		fi
	fi
}

# Removes existing files, as they can conflict with the dkms ones
fn_remove_existing_files()
{
	local ker_ver=$(uname -r)

	if ls /lib/modules/$ker_ver/extra/Cs*.ko 1> /dev/null 2>&1; then
		local ker_dirs=$(ls /lib/modules)
		for dir in $ker_dirs; do
			if [ -d /lib/modules/$dir/extra ]; then
				sudo rm -rf /lib/modules/$dir/extra/Cs*.ko
			fi
			if [ -d /lib/modules/$dir/weak-updates ]; then
				sudo rm -rf /lib/modules/$dir/weak-updates/Cs*.ko
			fi
		done
	fi
}

fn_register_dkms()
{
	if [ $# -ne 1 ]; then
		return 1
	fi

	local quiet_flag=""
	if $from_rpm; then
		quiet_flag="-q"
	fi

	sudo dkms add $quiet_flag -m gage-driver -v $1 --rpm_safe_upgrade
	if [ $? -eq 0 ]; then
	    sudo dkms build $quiet_flag -m gage-driver -v $1
		if [ $? -eq 0 ]; then
		    sudo dkms install $quiet_flag -m gage-driver -v $1
		fi
	fi
}

fn_remove_dkms()
{
	if [ $# -ne 1 ]; then
		return 1
	fi
	sudo dkms remove -q -m gage-driver -v $1 --all
}

fn_main()
{
	local retVal=0
	local dkms_operation=""
	local module_version=""
	local module_exists=false
	local packages_to_check=""

	if [ $# -ge 3 ] && [ $3 = "--fromRPM" ]; then
		from_rpm=true
	fi

	if [ $# -lt 2 ]; then
		if $from_rpm; then
			exit 0
		else
			fn_usage
			exit 1
		fi
	fi

	cat /etc/*release|grep -qi 'red[ ]*hat' && distro="redhat"
	cat /etc/*release|grep -qi 'ubuntu' && distro="ubuntu"
	cat /etc/*release|grep -qi 'debian' && distro="ubuntu"

	if [ $distro = "redhat" ]; then
		packages_to_check="epel-release dkms"
	else
		packages_to_check="dkms"
	fi

	dkms_operation=$1
	module_version=$2

	if [[ $module_version == -* ]]; then
		if $from_rpm; then
			exit 0
		else
			echo "Module version cannot start with a dash (-)"
			exit 1
		fi
	fi

	for pkg in ${packages_to_check[@]}; do
		fn_verify_package $pkg
		retVal=$?
		if [ $retVal -ne 0 ]; then
			if $from_rpm; then
				exit 0
			else
				exit 1
			fi
		fi
	done

	$(dkms status | grep -q "gage-driver, $module_version")
	if [ $? -eq 0 ]; then
		module_exists=true
	fi

	if $module_exists; then
		if [ $dkms_operation = "add" ]; then
			if ! $from_rpm; then
				echo "Module already in dkms tree."
			fi
			exit 0
		else
			fn_remove_dkms $module_version
			retVal=$?
			if [ $retVal -ne 0 ]; then
				if $from_rpm; then
					exit 0
				else
					exit 1
				fi
			fi
			exit 0
		fi
	else
		if [ $dkms_operation = "remove" ]; then
			if ! $from_rpm; then
				echo "Module not found in dkms tree."
			fi
			exit 0
		fi
	fi

	fn_remove_existing_files

	fn_register_dkms $module_version
	retVal=$?

	if [ $retVal -ne 0 ]; then
		if $from_rpm; then
			exit 0
		else
			exit 1
		fi
	fi

}

fn_main "$@"
