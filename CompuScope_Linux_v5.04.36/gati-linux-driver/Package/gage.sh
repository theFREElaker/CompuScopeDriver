
# Verifies if the LIBRARY_PATH environment variable is properly set.
val=""
gageLibPath="/usr/lib/gage"
if ! env | grep -q "^LIBRARY_PATH="; then
	val="$gageLibPath"
else
	val=$(echo $LIBRARY_PATH)
	if ! echo $val | grep -q "$gageLibPath"; then
		if [[ -z "${val// }" ]]; then
			val="$gageLibPath"
		else
			val="$gageLibPath:$val"
		fi
	fi
fi

export LIBRARY_PATH=$val

# Starts csrmd, if it exists and is not running
if [ -f /usr/bin/csrmd ] && ! ps ax | grep -v grep | grep -q csrmd; then
	csrmd &
fi
