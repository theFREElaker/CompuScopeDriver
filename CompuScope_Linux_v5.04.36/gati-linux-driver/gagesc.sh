#!/bin/bash
if [ $# -lt 1 ]; then
   echo ""
   echo "Usage:"
   echo "   $0 ops [-c argp] [-t argt] [-e arge] [-m argm] [-o argo] [-p argp] [-ad]"
   echo ""
   echo "   Important"
   echo "      Ensure that the libraries are not in use (except csrmd)"
   echo ""
   echo "   ops"
   echo "      Valid operations:"
   echo "         * install   [ install SDK, Driver ]"
   echo "         * uninstall [ uninstall SDK, Driver ]"
   echo "         * buildrpm  [ create rpm package                   ]"
   echo "         * builddeb  [ create debian package                ]"
   echo "         * drvdev    [ driver development mode              ]"
   echo "         * sdkdev    [ sdk development mode                 ]"
   echo "         * cleanst   [ clean source tree                    ]"
   echo "         * init      [ generate all Makefiles               ]"
   echo "      Note that the installation process creates an application"
   echo "      helper script, gagehp.sh, to assist in csrmd control."
   echo "      Usage:"
   echo "         gagehp.sh [-r argr] [-c argc] [-d argd] [-i]"
   echo ""
   echo "         -r, --run"
   echo "            Execute appl; e.g."
   echo "               * -r ./executable"
   echo "               * -r ~/some/folder/executable"
   echo "               * -r /absolute/path/to/executable"
   echo ""
   echo "         -c, --csrmd"
   echo "            csrmd command, where argc must be one of the following"
   echo "               * start"
   echo "               * stop"
   echo "               * restart"
   echo ""
   echo "         -d, --driver"
   echo "            driver command, where argd shares the same list as argc."
   echo ""
   echo "         -i, --info"
   echo "            Display csrmd status and driver(s) loaded."
   echo ""
   echo "   -c, --codename"
   echo "      Product codename, where argp must be one of the following:"
   echo "         * johndeere"
   echo "         * rabbit"
   echo "         * cobramax"
   echo "         * spider"
   echo "         * splenda"
   echo "         * decade"
   echo "         * hexagon"
   echo "      Mandatory for 'install-' and 'drvdev-' ops."
   echo "      Multiple codenames must be enclosed in double quote and"
   echo "      separated by a comma, e.g \"spider,splenda\"."
   echo ""
   echo "   -t, --target"
   echo "      Makefile target, where argt must be one of the following:"
   echo "         * install   [ compile the source codes,"
   echo "                       install the module(s),"
   echo "                       set up env dependencies, if any, "
   echo "                       load/run the module(s)/daemon ]"
   echo "         * uninstall"
   echo "         * clean"
   echo "      Mandatory for all *dev ops."
   echo ""
   echo "   -e, --extra"
   echo "      Extra option(s) to pass to make, e.g."
   echo "         * \"CXXFLAGS='-g -O0' CFLAGS='-g -O0'\""
   echo "         * \"CXXFLAGS='-g -O2 -pg'\""
   echo "         * \"CXXFLAGS='-g -O0' V=1 MYDEBUG=y\""
   echo "      Note that"
   echo "         * Default options for CXXFLAGS & CFLAGS are '-g -O2'"
   echo "         * The options string must be enclosed in double quote,"
   echo "           as shown above."
   echo "         * To make the extra option(s) permanent, use -f switch."
   echo ""
   echo "   -m, --module"
   echo "      Module name, which is to be used in conjunction with a"
   echo "      'clean' target, i.e. '-t clean'; argm must be one of the"
   echo "      following:"
   echo "         * disp"
   echo "         * fs"
   echo "         * rm"
   echo "         * rmdll"
   echo "         * ssm"
   echo "         * appsupport"
   echo "         * johndeere"
   echo "         * rabbit"
   echo "         * spider"
   echo "         * splenda"
   echo "      Multiple module names must be enclosed in double quote and"
   echo "      separated by a comma, e.g \"disp,ssm\"."
   echo ""
   echo "   -a, --autoload"
   echo "      Autoload kernel module on boot."
   echo ""
   echo "   -d, --depsinst"
   echo "      Install toolchain and/or dependencies."
   echo ""
   echo "   -o, --os"
   echo "   -f, --force"
   echo "   -p, --patch"
   echo "      Debugging purposes."
   exit
fi
if [ $EUID -eq 0 ]; then
   echo -e "\nDo not run this script with root privilege.\n"
   exit 1
fi

fn_helper_create()
{
   local name="$1"
   (
   cat <<'__helper_eof__'

#!/bin/bash
if [ $EUID -eq 0 ]; then
   echo -e "\nDo not run this script with root privileges.\n"
   exit 1
fi
if [ $# -lt 1 ]; then
   echo ""
   echo "Usage:"
   echo "   $0 [-r argr] [-c argc] [-d argd] [-i]"
   echo ""
   echo "   -r, --run"
   echo "      Execute appl; e.g."
   echo "         * -r ./executable"
   echo "         * -r ~/some/folder/executable"
   echo "         * -r /absolute/path/to/executable"
   echo ""
   echo "   -c, --csrmd"
   echo "      csrmd command, where argc must be one of the following"
   echo "         * start"
   echo "         * stop"
   echo "         * restart"
   echo ""
   echo "   -d, --driver"
   echo "      driver command, where argd shares the same list as argc."
   echo ""
   echo "   -i, --info"
   echo "      Display csrmd status and driver(s) loaded."
   echo ""
   exit
fi

fn_main()
{
   local arg=""
   local appl=""
   local found=""
   local info=""
   local drivers=""
   local lsmodlist=""
   local xmlfile="/usr/local/share/Gage/GageDriver.xml"
   while [ "$1" != "" ]; do
      case $1 in
         "-c" | "--csrmd" )
            shift
            arg="$1"
            [ ! -x "/usr/local/bin/csrmd" ] && { echo -e "\ncsrmd's missing or access denied!\n"; exit 1; }
            found=$(ps ax|grep -v grep|grep -i 'csrmd')
            case "$arg" in
               "restart") [ ! -z "$found" ] && { killall csrmd >/dev/null 2>&1; sleep 1; }; csrmd& ;;
               "start"  ) [   -z "$found" ] && csrmd& ;;
               "stop"   )
                  [ ! -z "$appl"  ] && { echo -e "\nConflicting options!\n"; exit 1; }
                  [ ! -z "$found" ] && { killall csrmd >/dev/null 2>&1; sleep 1; }
                  ;;
               *        ) echo -e "\nInvalid -c argument!\n"; exit 1
            esac
            ;;
         "-d" | "--driver")
            shift
            arg="$1"
            [ ! -e "$xmlfile" ] && { echo -e "\nGageDriver.xml not found!\n"; exit 1; }
            drivers=$(grep 'name=' "$xmlfile"|sed 's|.*name="\(.*\)".*|\1|g'|tr '\n' ' ')
            lsmodlist=$(lsmod)
            for drv in $drivers; do
               found=$(echo "$lsmodlist"|grep -i "$drv")
               case "$arg" in
                  "restart") [ ! -z "$found" ] && sudo modprobe -r "$drv"; sudo modprobe "$drv" ;;
                  "start"  ) [   -z "$found" ] && sudo modprobe    "$drv"                       ;;
                  "stop"   ) [ ! -z "$found" ] && sudo modprobe -r "$drv"                       ;;
                  *        ) echo -e "\nInvalid -d argument!\n"; exit 1
               esac
            done
            ;;
         "-i" | "--info")
            info="yes"
            ;;
         "-r" | "--run")
            shift
            appl="$1"
            ;;
         *) echo -e "\nUnknown option!\n"; exit 1
      esac
      shift
   done
   [ ! -z "$appl" ] && {
      ps ax|grep -v grep|grep -qi 'csrmd' || eval 'csrmd&';
      eval "$appl";
   }
   [ "$info" = "yes" ] && {
      found=$(ps ax|grep -v grep|grep -i 'csrmd')
      drivers=$(lsmod|grep -i '\bcs[e]\?[cdx][cdy]g[1-9]\+\b\|\bcs[e]\?[1-9]\+[bx][cdgxy]\{2\}\b'|sed 's|[ \t].*||g'|tr '\n' ' ');
      [ ! -z "$found" ] && found="yes" || found="no"
      echo -e "\ncsrmd running   : $found\ndriver(s) loaded: ${drivers:-none}\n";
   }
}

fn_main "$@"

__helper_eof__
   ) > "$name"
}

fn_helper_install()
{
   [ -e "/usr/local/bin/gagehp.sh" ] && return 0
   fn_helper_create "gagehp.sh"
   sudo install -m 755 "gagehp.sh" "/usr/local/bin"
   rm "gagehp.sh"
}

fn_helper_uninstall()
{
   sudo rm -f "/usr/local/bin/gagehp.sh" >/dev/null 2>&1
}

fn_kmodname()
{
   local pcname="$1"
   case "$pcname" in
      "CsJohnDeere") echo "CscdG12" ;;
      "CsRabbit"   ) echo "CsxyG8"  ;;
      "CsCobraMax" ) echo "CsEcdG8" ;;
      "CsSpider"   ) echo "Cs8xxx"  ;;
      "CsSplenda"  ) echo "Cs16xyy" ;;
      "CsDecade12" ) echo "CsE12xGy" ;;
      "CsHexagon"  ) echo "CsE16bcd" ;;
      *            ) echo -e "\nInvalid codename!\n"; exit 1
   esac
}

fn_codename()
{
   local kname="$1"
   case "$kname" in
      "CscdG12") echo "CsJohnDeere" ;;
      "CsxyG8" ) echo "CsRabbit"    ;;
      "CsEcdG8") echo "CsCobraMax"  ;;
      "Cs8xxx" ) echo "CsSpider"    ;;
      "Cs16xyy") echo "CsSplenda"   ;;
      "CsE12xGy") echo "CsDecade12"  ;;
      "CsE16bcd") echo "CsHexagon"   ;;
     *        ) echo -e "\nInvalid codename!\n"; exit 1
   esac
}

fn_umodpath()
{
   local mod="$1"
   local modpath=(Middle/CsDisp \
                  Middle/CsFs \
                  Middle/CsRmDLL \
                  Middle/CsRm \
                  Middle/CsSsm \
                  Sdk/CsAppSupport \
                  Boards/UserSpace/CsRabbit \
                  Boards/UserSpace/CsSpider \
                  Boards/UserSpace/CsSplenda \
                  Boards/UserSpace/CsDecade12 \
                  Boards/UserSpace/CsHexagon \
                  Boards/UserSpace/CsJohnDeere)
   [ "$mod" = "all" ] && { echo "${modpath[*]}"; return; }
   for dir in "${modpath[@]}"; do echo $dir|grep -qi "\<cs$mod\>" && { echo $dir; return; } done
   echo "\nInvalid -m argument!\n"
   exit 1
}

fn_is_running()
{
   local appl="$1"
   local ret="no"
   ps ax|grep -v grep|grep -qi "$appl" && ret="yes"
   echo $ret
}

fn_exit_if_null()
{
   local str="$1"
   local info="$2"
   [ -z "$str" ] && { echo -e "\nMissing $info!\n"; exit 1; }
}

fn_exit_if_failed()
{
   local ret="$1"
   local info="$2"
   [ $ret -ne 0 ] && { [ ! -z "$info" ] && echo -e "\n$info\n"; exit 1; }
}

fn_daemon_load()
{
   csrmd& >/dev/null 2>&1
}

fn_daemon_unload()
{
   killall csrmd >/dev/null 2>&1
   sleep 1
}

fn_driver_load()
{
   local pcname="$1"
   local kdname=$(fn_kmodname "$pcname")
   sudo modprobe "$kdname"
   fn_exit_if_failed $?
}

fn_driver_unload()
{
   local pcname="$1"
   local kdname=$(fn_kmodname "$pcname")
   lsmod|grep -qi "$kdname" && { sudo modprobe -r "$kdname"; fn_exit_if_failed $?; }
}

fn_driver_install()
{
   local pcname="$1"
   cd "Boards/KernelSpace/$pcname/Linux"
   sudo make install
   local ret=$?
   cd - >/dev/null
   fn_exit_if_failed $ret
   fn_helper_install
}

fn_driver_uninstall()
{
   local pcname="$1"
   local opsys="$4"
   local kdname=$(fn_kmodname "$pcname")
   local sharedir="/usr/local/share/Gage"
   local xmlfile="$sharedir/GageDriver.xml"
   cd "Boards/KernelSpace/$pcname/Linux"
   sudo make uninstall
   local ret=$?
   cd - >/dev/null
   fn_exit_if_failed $ret
   # cleanup
   fn_autoload_disable "$pcname" "$opsys"
   [ -e "$xmlfile" ] && {
      eval "sudo sed -i '/$kdname/ d' $xmlfile";
      grep -q '<driver name=' "$xmlfile" && return 0;
   }
   # leave csi in for now; too much work to delete
   [ -d "$sharedir" ] && {
      sudo rm -rf "$sharedir";
      fn_helper_uninstall;
   }
}

fn_driver_make()
{
   local pcname="$1"
   local xtr="$2"
   cd "Boards/KernelSpace/$pcname/Linux"
   local makecmd="make"
   [ ! -z "$xtr" ] && makecmd="$makecmd $(echo "$xtr"|base64 -d)"
   eval $makecmd
   local ret=$?
   cd - >/dev/null
   fn_exit_if_failed $ret
}

fn_driver_build()
{
   local pcname="$1"
   local xtr="$2"
   local persistent="$3"
   local opsys="$4"
   local kdname=$(fn_kmodname "$pcname")
   fn_driver_clean "$pcname"
   fn_driver_make "$pcname" "$xtr"
   fn_driver_install "$pcname"
   # sharedir content and xmlfile are needed by the libraries, not driver.
   # parking these activities in sdkdev is the logical choice but
   #  1. they will be executed frequently during development using this script as is.
   #  2. current driver implementation requires loading of the corresponding driver
   #     for each model, rather than 1 universal driver supporting all models. This
   #     means that driver installation will always require the '-c' flag, which is
   #     required by the content, whereas sdkdev does not.
   # I opted to park them with drvdev for less typing.
   #
   # ** The ideal solution would be to remove the xmlfile since it's content can be
   #    retrieved elsewhere easily.
   local tag="gageDriver"
   local sharedir="/usr/local/share/Gage"
   local xmlfile="$sharedir/GageDriver.xml"
   [ ! -d "$sharedir" ] && sudo mkdir "$sharedir"
   [ ! -e "$xmlfile" ] && echo -e "<?xml version=\"1.0\"?>\n<$tag>\n</$tag>"|sudo tee "$xmlfile" >/dev/null
   grep -qi "$kdname" "$xmlfile" || eval "sudo sed -i 's|\(</$tag>\)|   <driver name=\"$kdname\"></driver>\n\1|' $xmlfile"
   if [ -d "Boards/Csi/$pcname" ]; then
      sudo cp -f Boards/Csi/"$pcname"/* "$sharedir" >/dev/null 2>&1
      sudo chmod 666 "$sharedir"/*.csi >/dev/null 2>&1
   fi
   [ "$persistent" = "yes" ] && fn_autoload_enable "$pcname" "$opsys"
}

fn_driver_clean()
{
   local pcname="$1"
   cd "Boards/KernelSpace/$pcname/Linux"
   make clean -ik >/dev/null 2>&1
   cd - >/dev/null
}

fn_autotools_init()
{
   local force="$1"
   local xtr="$2"
   local configurecmd="./configure"
   if [ ! -f "aclocal.m4" ] || [ "$force" = "yes" ]; then
      #
      # fix timestamp from repository
      #find . -name "Makefile.*" -o -name "configure.*" | xargs sudo touch
      #
      # add libtool support
      libtoolize --automake
      #
      # generate m4 environment for autotools
      aclocal
      #
      # parse configure.ac; generate configure script
      autoconf
      autoheader
      #
      # parse Makefile.am; generate Makefile.in
      automake --add-missing --foreign
      #
      # run it
      if [ "$force" = "yes" ] && [ ! -z "$xtr" ]; then configurecmd="$configurecmd $(echo "$xtr"|base64 -d)"; fi
      eval $configurecmd
      fn_exit_if_failed $?
   fi
}

fn_sdk_install()
{
   local opsys="$3"
   echo "- Installing libraries..."
   # how to suppress the useless installed message?
   sudo make install >/dev/null
   fn_exit_if_failed $?
   echo "$(lsb_release -si) $opsys"|grep -qi 'redhat' && {
      [ ! -e "/etc/ld.so.conf.d/gage.conf" ] && echo "/usr/local/lib"|sudo tee "/etc/ld.so.conf.d/gage.conf" >/dev/null;
   }
   sudo ldconfig
   fn_helper_install
}

fn_sdk_uninstall()
{
   local opsys="$3"
   echo "- Uninstalling libraries..."
   sudo make uninstall >/dev/null
   echo "$(lsb_release -si) $opsys"|grep -qi 'redhat' && {
      [ -e "/etc/ld.so.conf.d/gage.conf" ] && sudo rm -f "/etc/ld.so.conf.d/gage.conf";
   }
   sudo ldconfig
}

fn_sdk_make()
{
   local xtr="$1"
   local makecmd="make"
   [ ! -z "$xtr" ] && makecmd="$makecmd $(echo "$xtr"|base64 -d)"
   eval $makecmd
   fn_exit_if_failed $?
}

fn_sdk_build()
{
   local xtr="$1"
   local opsys="$3"
   fn_sdk_make "$xtr"
   fn_sdk_install "dummy" "dummy" "$opsys"
}

fn_sdk_clean()
{
   local subdirs="$2"
   [ -z "$subdirs" ] && make clean -ik >/dev/null 2>&1
   for dir in $subdirs; do make clean -C "$dir" -ik >/dev/null 2>&1; done
}

fn_gui_install()
{
   sudo make install -C "VersaScope/linux-gtk2-build"
   fn_exit_if_failed $?
}

fn_gui_uninstall()
{
   sudo make uninstall -C "VersaScope/linux-gtk2-build"
}

fn_gui_make()
{
   local xtr="$1"
   local makecmd="make -C VersaScope/linux-gtk2-build"
   [ ! -z "$xtr" ] && makecmd="$makecmd $(echo "$xtr"|base64 -d)"
   eval $makecmd
   fn_exit_if_failed $?
}

fn_gui_build()
{
   local xtr="$1"
   fn_gui_make "$xtr"
   fn_gui_install
}

fn_gui_clean()
{
   make clean -C "VersaScope/linux-gtk2-build" -ik >/dev/null 2>&1
}

fn_autoload_enable()
{
   local pcname="$1"
   local opsys="$2"
   local kdname=$(fn_kmodname "$pcname")
   local distro=$(lsb_release -si)
   local autoloadfile=""
   if echo "$distro $opsys"|grep -qi 'ubuntu'; then
      autoloadfile="/etc/modules"
      grep -qi "\<$kdname\>" "$autoloadfile" && return 0
      echo "$kdname"|sudo tee -a "$autoloadfile" >/dev/null
      sudo update-initramfs -u >/dev/null

   elif echo "$distro $opsys"|grep -qi 'redhat'; then
      autoloadfile="/etc/sysconfig/modules/gage.modules"
      [ ! -e "$autoloadfile" ] &&  {
         echo "#!/bin/sh"|sudo tee "$autoloadfile" >/dev/null;
         sudo chmod 777 "$autoloadfile";
      }
      grep -qi "\<$kdname\>" "$autoloadfile" && return 0
      echo -e "\n[ -e /lib/modules/$(uname -r)/extra/${kdname}.ko ] && modprobe $kdname"|sudo tee -a "$autoloadfile" >/dev/null

   else
     :
   fi
}

fn_autoload_disable()
{
   local pcname="$1"
   local opsys="$2"
   local kdname=$(fn_kmodname "$pcname")
   local distro=$(lsb_release -si)
   local autoloadfile=""
   if echo "$distro $opsys"|grep -qi 'ubuntu'; then
      autoloadfile="/etc/modules"
      grep -qi "\<$kdname\>" "$autoloadfile" || return 0
      eval "sudo sed -i '/\<$kdname\>/ d' $autoloadfile"
      sudo update-initramfs -u >/dev/null

   elif echo "$distro $opsys"|grep -qi 'redhat'; then
      autoloadfile="/etc/sysconfig/modules/gage.modules"
      [ ! -e "$autoloadfile" ] && return 0
      grep -qi "\<$kdname\>" "$autoloadfile" || return 0
      eval "sudo sed -i '/\<$kdname\>/ d' $autoloadfile"
      grep -qi 'modprobe' "$autoloadfile" || sudo rm -f "$autoloadfile"

   else
      :
   fi
}

fn_install()
{
   local pcname="$1"
   local xtr="$2"
   local persistent="$3"
   local opsys="$4"
   local force="$5"
   local count="$6"
   if [ "$count" = "1" ]; then
      fn_autotools_init "$force" "$xtr"
      fn_driver_build "$pcname" "$xtr" "$persistent" "$opsys"
      fn_sdk_build "$xtr" "dummy" "$opsys"
      #fn_gui_build "$xtr"
      fn_driver_load "$pcname"
      fn_daemon_load
      [ "$persistent" = "yes" ] && fn_autoload_enable "$pcname" "$opsys"
   else
      # only kernel driver(s) need to be installed
      fn_driver_build "$pcname" "$xtr" "$persistent" "$opsys"
      fn_driver_load "$pcname"
      fn_daemon_unload
      fn_daemon_load
      [ "$persistent" = "yes" ] && fn_autoload_enable "$pcname" "$opsys"
   fi
}

fn_uninstall()
{
   local pcname="$1"
   local opsys="$4"
   local count="$6"
   if [ "$count" = "1" ]; then
      #fn_gui_uninstall
      fn_daemon_unload
      fn_sdk_uninstall "dummy" "dummy" "$opsys"
      fn_driver_unload "$pcname"
      fn_driver_uninstall "$pcname" "dummy" "dummy" "$opsys"
      #fn_gui_clean
      fn_sdk_clean
      fn_driver_clean "$pcname"
   else
      fn_driver_unload "$pcname"
      fn_driver_uninstall "$pcname" "dummy" "dummy" "$opsys"
      fn_driver_clean "$pcname"
   fi
}

fn_install_deps()
{
   local opsys="$1"
   local distro=""
   local distrover=""
   # local distro=$(lsb_release -si)        ! lsb_release is not installed by default in redhat 7 !
   # local distrover=$(lsb_release -sr)
   local arch=$(uname -m|sed 's|x86_||;s|i[3-6]86|32|')
   local instlist=""
   local wver=""
   local jver=""
   local deplist=""
   local dpkglist=""
   local username=$(whoami)
   cat /etc/*release|grep -qi 'red[ ]*hat' && distro="redhat"
   cat /etc/*release|grep -qi 'ubuntu' && distro="ubuntu"
   if echo "$distro $opsys"|grep -qi 'ubuntu'; then
      deplist="build-essential default-jdk automake libtool devscripts debhelper dh-autoreconf"
      dpkglist=$(dpkg -l|tr -s ' '|cut -d' ' -f2)
      for dep in $deplist; do
         echo "$dpkglist"|grep -qi "$dep" || instlist="$instlist $dep"
      done
      [ ! -z "$instlist" ] && {
         sudo apt-get install $instlist -y;
         fn_exit_if_failed $?;
      }

   elif echo "$distro $opsys"|grep -qi 'redhat'; then
      su -c "grep -qi $username /etc/sudoers" || su -c "echo -e \"$username\tALL=(ALL)\tALL\" >>/etc/sudoers"
      [ ! -e "/usr/bin/lsb_release" ] && { sudo yum install redhat-lsb-core -y; fn_exit_if_failed $?; }
      distrover=$(lsb_release -sr)
      yum list installed "epel-release" || {
         [ ${distrover%%.*} = "7" ] && {
            wget -qN "https://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm";
            fn_exit_if_failed $?;
            sudo rpm -ivh epel-release-*7*.noarch.rpm;
         }
         [ ${distrover%%.*} = "6" ] && {
            wget -qN "https://dl.fedoraproject.org/pub/epel/epel-release-latest-6.noarch.rpm";
            fn_exit_if_failed $?;
            sudo rpm -ivh epel-release-*6*.noarch.rpm;
         }
      }
      # not enabling epel permanently seems to cause yum-update to fail ?!
      sudo yum-config-manager --enable epel
      jver=$(yum list java*|grep 'java-'|cut -d' ' -f1|sed 's|java-\([0-9]*[.0-9]*\).*|\1|g'|sort -nr|sed '1!d')
      deplist="kernel-devel-$(uname -r) gcc gcc-c++ automake dkms java-${jver}-openjdk"
      dpkglist=$(rpm -qa)
      for dep in $deplist; do
         echo "$dpkglist"|grep -qi "$dep" || instlist="$instlist $dep"
      done
      # need separate check for libtool because of other package naming
      echo "$dpkglist"|grep -qi 'libtool-[0-9]' || instlist="$instlist libtool"

      # Need separate check for rpm-build because of other package naming
      echo "$dpkglist" | grep -qvi 'libs' | grep -qi 'rpm-build' || instlist="$instlist rpm-build"

      [ ! -z "$instlist" ] && {
         # sudo yum --enablerepo=epel install $instlist -y;
         sudo yum install $instlist -y;
         fn_exit_if_failed $?;
      }

   else
      echo -e "\nUnknown distribution!\n"
      exit 1

   fi
}

fn_create_deb()
{
   local version=""
   local path=""
   local debPath="Package/deb"
   local outputPath="$debPath/output"
   local files_to_change=$(ls $debPath/debian_template/)
   local curTime=$(date +"%a, %d %b %Y %T %z")
   local buildstatus=0

   rm -rf $outputPath/
   mkdir -p $outputPath
   tar -zxf gage-driver-*.tar.gz -C $outputPath/

   version=$(ls -d $outputPath/* | sed "s/[a-zA-Z/\-]\+//")

   path="$outputPath/gage-driver-$version/"
   mkdir -p "$path/debian"

   for file in $files_to_change; do
      sed "s/#VERSION#/$version/g; s/#TIME#/$curTime/g;" "$debPath/debian_template/$file" > "$path/debian/$file"
   done
   chmod 755 "$path/debian/rules"

   cd $path >/dev/null
   ./configure

   # The options are to disable GPG signature
   debuild -us -uc
   buildstatus=$?
   cd - >/dev/null

   if [ $buildstatus -eq 0 ]; then
      rm -rf $path
   fi
}

fn_main()
{
   local mode=""
   local extra=""
   local module=""
   local target=""
   local pcname=""
   local subdirs=""
   local persistent="no"
   local depsinst="no"
   local opsys=""
   local force="no"
   local rmactive="no"
   local doinit=""
   local dobuildrpm=""
   local dobuilddeb=""
   local dorhwxgtk3=""
   local xmlfile="/usr/local/share/Gage/GageDriver.xml"
   local temp=""
   local i="1"
   while [ "$1" != "" ]; do
      case $1 in
         "install"  ) mode="install"   ;;
         "uninstall") mode="uninstall" ;;
         "drvdev"   ) module="driver"  ;;
         "sdkdev"   ) module="sdk"     ;;
         "guidev"   ) module="gui"     ;;
         "init"     ) doinit="yes"     ;;
         "buildrpm" ) dobuildrpm="yes" ;;
         "builddeb" ) dobuilddeb="yes" ;;
         "cleanst"  )
            temp=". $(fn_umodpath "all")"
            local kpath=("CsJohnDeere" "CsRabbit" "CsCobraMax" "CsSpider" "CsSplenda" "CsDecade12" "CsHexagon")
            make clean mostlyclean distclean maintainer-clean -ik >/dev/null 2>&1
            for dir in $temp; do rm -f "$dir"/Makefile.in; done
            for kDir in ${kpath[@]}; do
               fn_driver_clean $kDir
            done
            rm -fr Package/rpm/rpmb Package/deb/output aclocal.m4 ar-lib autom4te.cache compile config.* configure depcomp gage-driver-*.tar.gz install-sh ltmain.sh missing >/dev/null 2>&1
            exit
            ;;
         "-a"  | "--autoload") persistent="yes"                 ;;
         "-d"  | "--depsinst") depsinst="yes"                   ;;
         "-ad" | "-da"       ) persistent="yes"; depsinst="yes" ;;
         "-f"  | "--force"   ) force="yes"                      ;;
         "-p"  | "--patch"   )
            shift
            [ "$1" = "rh-wxgtk3" ] && dorhwxgtk3="yes"
            ;;
         "-o"  | "--os")
            shift
            case "$1" in
               "ubuntu") opsys="ubuntu" ;;
               "redhat") opsys="redhat" ;;
               *       ) echo -e "\nInvalid -o argument!\n"; exit 1
            esac
            ;;
         "-c"  | "--codename")
            shift
            temp=$(echo "$1"|sed 's|,| |g')
            for name in $temp; do
               case "$name" in
                  "johndeere") pcname="$pcname CsJohnDeere" ;;
                  "rabbit"   ) pcname="$pcname CsRabbit"    ;;
                  "cobramax" ) pcname="$pcname CsCobraMax"  ;;
                  "spider"   ) pcname="$pcname CsSpider"    ;;
                  "splenda"  ) pcname="$pcname CsSplenda"   ;;
                  "decade"   ) pcname="$pcname CsDecade12"  ;;
                  "hexagon"  ) pcname="$pcname CsHexagon"   ;;
                  *          ) echo -e "\nInvalid -c argument!\n"; exit 1
               esac
            done
            pcname=$(echo "$pcname"|sed 's|^ *||g')
            ;;
         "-t"  | "--target")
            shift
            case "$1" in
               "install"  ) target="build"     ;;
               "uninstall") target="uninstall" ;;
               "clean"    ) target="clean"     ;;
               *          ) echo -e "\nInvalid -t argument!\n"; exit 1
            esac
            if [ "$target" != "clean" ] && [ $(fn_is_running "VersaScope") = "yes" ]; then
               echo -e "\nGUI's running!\n"
               exit 1
            fi
            ;;
         "-e"  | "--extra")
            shift
            # it's non trivial to not mess up the content if it contains bash's special
            # characters or IFS; convert to base64 to avoid the issue.
            extra=$(echo "$1"|base64)
            ;;
         "-m"  | "--module")
            shift
            temp=$(echo "$1"|sed 's|,| |g')
            for mod in $temp; do subdirs="$subdirs $(fn_umodpath $mod)"; done
            subdirs=$(echo "$subdirs"|sed 's|^ *||g')
            ;;
         *)
            echo -e "\nUnknown option!\n"
            exit 1
      esac
      shift
   done

   [ "$doinit" = "yes" ] && { fn_autotools_init "$force" "$extra"; exit; }
   [ "$depsinst" = "yes" ] && fn_install_deps "$opsys"
   if [ "$dorhwxgtk3" = "yes" ]; then
      if [ ! -e "/usr/bin/wx-config" ] && [ "$mode" = "install" ]; then
         echo -e "\n- Adding rh-wxgtk3 patch...\n"
         temp=$(sudo find / -name 'wx-config')
         [ -z "$temp" ] && { echo "\nCould not find wxGTK3 wx-config!\n"; exit 1; }
         sudo ln -s "$temp" "/usr/bin/wx-config"
      elif [ -e "/usr/bin/wx-config" ] && [ "$mode" = "uninstall" ]; then
         echo -e "\n- Removing rh-wxgtk3 patch...\n"
         sudo rm -f "/usr/bin/wx-config"
      else
         :
      fi
   fi
   [ "$dobuildrpm" = "yes" ] && {
      fn_autotools_init "$force" "$extra";
      echo "- Creating tar.gz...";
      make distcheck >/dev/null;
      fn_exit_if_failed $?;
      local prefix="Package/rpm/rpmb"
      temp="$prefix $prefix/BUILD $prefix/BUILDROOT $prefix/SRPMS $prefix/RPMS $prefix/SPECS $prefix/SOURCES";
      for dir in $temp; do mkdir -p "$dir"; done;
      echo "- Creating rpm...";
      eval "rpmbuild --define '_topdir $(pwd)/$prefix' -ta $(ls gage-driver-*.tar.gz) --quiet";
      [ $? -eq 0 ] && echo -e "\nRPM created successfully in $prefix/RPMS.\n";
      exit;
   }
   [ "$dobuilddeb" = "yes" ] && {
      fn_autotools_init "$force" "$extra";
      echo "- Creating tar.gz...";
      make distcheck >/dev/null;
      fn_exit_if_failed $?;
      fn_create_deb
      exit;
   }
   if [ "$mode" = "install" ]; then
      fn_exit_if_null "$pcname" "-c"
      for name in $pcname; do
         fn_${mode} "$name" "$extra" "$persistent" "$opsys" "$force" "$i"
         ((i++))
      done

   elif [ "$mode" = "uninstall" ]; then
      [ ! -e "$xmlfile" ] && {
         # if gagedriver.xml is missing, it could only mean either fn_driver_make or
         # fn_driver_install failed, and there is nothing to be uninstall in either
         # case, nor for SDK or GUI since both are installed after the kernel driver.
         exit 0;
      }
      temp=$(grep 'name=' "$xmlfile"|sed 's|.*name="\(.*\)".*|\1|g'|tr '\n' ' ')
      for name in $temp; do
         fn_${mode} "$(fn_codename $name)" "$extra" "$persistent" "$opsys" "$force" "$i"
         ((i++))
      done

   elif [ "$module" = "driver" ]; then
      fn_exit_if_null "$pcname" "-c"
      fn_exit_if_null "$target" "-t"
      rmactive=$(fn_is_running "csrmd")
      if [ "$target" != "clean" ]; then
         [ "$rmactive" = "yes" ] && fn_daemon_unload
         for name in $pcname; do fn_driver_unload "$name"; done
      fi
      for name in $pcname; do fn_driver_${target} "$name" "$extra" "$persistent" "$opsys"; done

      if [ "$target" = "build" ]; then
         for name in $pcname; do fn_driver_load "$name"; done
         [ "$rmactive" = "yes" ] && fn_daemon_load
      fi

   elif [ "$module" = "sdk" ]; then
      fn_exit_if_null "$target" "-t"
      fn_autotools_init "$force" "$extra"
      [ "$target" != "clean" ] && fn_daemon_unload
      fn_sdk_${target} "$extra" "$subdirs" "$opsys"
      [ "$target" = "build" ] && fn_daemon_load

   elif [ "$module" = "gui" ]; then
      fn_exit_if_null "$target" "-t"
      fn_gui_${target} "$extra"

   else
      echo -e "\nHummm....?!\n"
      exit 1

   fi
}

fn_main "$@"

