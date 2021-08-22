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
            if [ ! -x "/usr/local/bin/csrmd" ] && [ ! -x "/usr/bin/csrmd" ]; then
               echo -e "\ncsrmd's missing or access denied!\n"
            else
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
            fi
            ;;
         "-d" | "--driver")
            shift
            arg="$1"
            if [ ! -e "$xmlfile" ]; then
               echo -e "\nGageDriver.xml not found!\n"
            else
               drivers=$(grep 'name=' "$xmlfile"|sed 's|.*name="\(.*\)".*|\1|g'|tr '\n' ' ')
               lsmodlist=$(lsmod)
               for drv in $drivers; do
                  found=$(echo "$lsmodlist"|grep -i "$drv")
                  case "$arg" in
                     "restart") [ ! -z "$found" ] && sudo modprobe -r "$drv"; sudo modprobe "$drv" > /dev/null 2>&1 ;;
                     "start"  ) [   -z "$found" ] && sudo modprobe    "$drv" > /dev/null 2>&1 ;;
                     "stop"   ) [ ! -z "$found" ] && sudo modprobe -r "$drv" > /dev/null 2>&1 ;;
                     *        ) echo -e "\nInvalid -d argument!\n"; exit 1
                  esac
               done
            fi
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