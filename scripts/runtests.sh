#!/bin/bash -e

verbose=1

# Convenience function to return a string that
# is a reverse list of the incoming arguments:
#
reverse()
{
    args=($*)
    for (( i=((${#args[*]} - 1)); i >= 0; i-- )); do
	echo ${args[$i]}
    done
}

# Set paths to a particular module - if no path is set to a module, use modprobe:
#
declare -A a_mpath
mpath()
{
    local module="$1"
    local mpath="$2"
    [[ $mpath != "" ]] || fail "Usage: mpath module path"

    a_mpath[$module]="$BUILD/$mpath"
}

# Set parameters to load a given module with for test purposes:
declare -A a_params
params()
{
    local module="$1"
    shift
    a_params[$module]="$*"
}

log()
{
    (( $verbose )) && echo $*
}

mod_probe()
{
    local fm=""
    local name="$1"
    shift

    mp=${a_mpath[$name]}
    if [[ $mp != "" ]]; then
	fm="$mp"
    fi

    is_loaded=$(lsmod | egrep "^$name" || true)
    if [[ $is_loaded != "" ]]; then
	echo "Module \"$name\" is already loaded!" 1>&2
	return 0
    fi

    if [[ $fm == "" ]]; then
	log "Modprobing $name"
	$sudo modprobe $name ${a_params[$name]}
    else
	fm=${a_mpath[$name]}
        log "Insmod'ing module \"$name\"" 1>&2
	$sudo insmod $fm ${a_params[$name]}
    fi
}

# If/when more modules are to be loaded, this could go in a config file
# but for the purpose of this example, just do it inline:
#
mpath ktf 	ktf/kernel/ktf.ko
mpath selftest	ktf/selftest/selftest.ko

load_modules="ktf selftest"

unload_modules=$(reverse $load_modules)

sudo=""
if [[ $USER != "root" ]]; then
    sudo="sudo"
fi

for m in $load_modules; do
    mod_probe $m
done

if [[ $GTEST_PATH == "" ]];then
    echo "Set environment variable GTEST_PATH to point to your googletest build!"
    exit 1
fi

export LD_LIBRARY_PATH="$BUILD/ktf/lib:$GTEST_PATH/lib64:$GTEST_PATH/lib"
$BUILD/ktf/user/ktftest || stat=$?

for m in $unload_modules; do
    $sudo rmmod $m
done

exit $stat
