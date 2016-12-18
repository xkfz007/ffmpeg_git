#!/bin/bash
#set -e
set -x
usage(){
    cat <<EOF
    Usage:$0 conf=<configurations> ver=<versions> enabled=<features> disabled=<features>
    Options:
           configurations:release,debug
           versions:online, offline
           features:x264,x265,sdl,sdl2,libass,fdk-aac
           default: release, online
    Exampels:
          $0 conf=debug enabled=x264,x265,sdl2
    
EOF
    exit
}

add_x(){
    local file=$1
    if [ ! -x $file ];then
        chmod u+x $file
    fi
}

to_upper(){
    local t
    t=$(echo $1|tr '[a-z]' '[A-Z]')
    echo $t
}
to_lower(){
    local t
    t=$(echo $1|tr '[A-Z]' '[a-z]')
    echo $t
}

check_arg(){
    local arg=$1
    shift
    local FOUND=0
    while [ $# -gt 0 ]
    do
        if [ "$1" == "$arg" ];then
            FOUND=1
            break
        fi
        shift
    done

    if [ $FOUND -eq 0 ];then
        echo "Invalid option value $arg"
        exit
    fi
}

switch_feature(){
    local feature=$1
    local sw=$2

    local list=${feature//,/ }
    for i in ${list[@]}
    do
        check_arg $i "${!fea_list[@]}"
        fea_list[$i]=$sw
    done
}
turn_on_feature(){
    switch_feature $1 "on"
}
turn_off_feature(){
    switch_feature $1 "off"
}

pkgconfig_generate(){
    local name=$1
    local pkg_dir=$2
    local pkg_internal_dir=$3
    local prefix=$4
    local pkg_internal_file="${pkg_internal_dir}/${name}.ipc"
    local pkg_file="${pkg_dir}/${name}.pc"
    if [ ! -e $pkg_internal_file ];then
        echo "ERROR: $pkg_internal_file does not exist"
        exit
    fi
    cat <<EOF  >$pkg_file
prefix=$prefix
exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib
includedir=\${prefix}/include

EOF
   cat $pkg_internal_file >>$pkg_file
}

#check system
sys=$(uname -o)
if [[ $sys =~ "Cygwin" ]];then
    echo "sys="$sys
    sys_name="cygwin"
elif [[ $sys =~ "Linux" ]];then
    echo "sys="$sys
    sys_name="linux"
fi

conf_list=("release" "debug")
ver_list=("online" "offline")
declare -A fea_list=(
['x264']='on'
['x265']='on'
['sdl']='off'
['sdl2']='off'
['fdk-aac']='off'
['libass']='off'
['freetype2']='off'
['fontconfig']='off'
['fribidi']='off'
)
declare -A extlib_list=(
['x264']='libx264'
['x265']='libx265'
['sdl']='sdl'
['sdl2']='sdl2'
['fdk-aac']='libfdk_aac'
['libass']='libass'
['vpx']='libvpx'
['xvid']='libxvid'
['freetype2']='libfreetype'
['fontconfig']='fontconfig'
['fribidi']='libfribidi'
)

conf='release'
ver='online'
enabled_feature=''
disabled_feature=''
for i in $*
do
    echo $i
    case $i in
         help|h|-h|--help)
             usage
             exit
             ;;
    esac
    
    opt=$(echo $i|awk -F= '{print $1}')
    arg=$(echo $i|awk -F= '{print $2}')
    case "$opt" in 
        [cC][oO][nN][fF])
        #"conf")
            conf=$(to_lower $arg)
            check_arg $conf "${conf_list[@]}"
            ;;
        [vV][eE][rR])
        #"ver")
            ver=$(to_lower $arg)
            check_arg $ver  "${ver_list[@]}"
            ;;
        [eE][nN][aA][bB][lL][eE][dD])
        #"enabled")
            enabled_feature=$(to_lower $arg)
            if [ "$enabled_feature" != "" ];then
                turn_on_feature $enabled_feature 
            fi
            ;;
        [dD][iI][sS][aA][bB][lL][eE][dD])
        #"disabled")
            disabled_feature=$(to_lower $arg)
            if [ "$disabled_feature" != "" ];then
                turn_off_feature $disabled_feature
            fi
            ;;
        *)
            echo "Undefined option $opt"
            exit
    esac
done

#check directories
current_dir=`pwd`
root_dir=`readlink -f "$current_dir/../"`
source_dir="$root_dir/"
external_libs_dir=`readlink -f "$root_dir/../external_libs"`
if [ ! -e $external_libs_dir ];then
    echo "ERROR: ${external_libs_dir} does not exist"
    exit
fi

pkg_dir="${root_dir}/build/pkgconfig"
if [ ! -e $pkg_dir ];then
    mkdir $pkg_dir
fi
export PKG_CONFIG_PATH="$PKG_GONFIG_PATH:${pkg_dir}"

lib_flags=''
inc_flags=''
extlib_cmd=''

for key in ${!fea_list[@]}
do
    extlib_dir="${external_libs_dir}/$key-${sys_name}"
    lib_dir="$extlib_dir/"
    inc_dir="$extlib_dir/"
    pkg_prefix="$extlib_dir"
    if [ ${fea_list[$key]} = "on" ];then
        pkgconfig_generate $key "$pkg_dir" "$extlib_dir" "$pkg_prefix" 
        lib_flags=${lib_flags}" -L'"$lib_dir"'"
        inc_flags=${inc_flags}" -I'"$inc_dir"'"
        #name=$(to_lower $key)
        extlib_cmd=${extlib_cmd}" --enable-${extlib_list[$key]}"
        if [[ "$key" == "sdl"* ]];then
            extlib_cmd=${extlib_cmd}" --enable-outdev=$key"
        fi
    fi
done

add_x ${source_dir}/configure
add_x ${source_dir}/version.sh

builds_dir="${root_dir}/build/${conf}_${ver}"
if [ ! -e $builds_dir ];then
    mkdir $builds_dir
fi

cd ${builds_dir}

cmd="${source_dir}/configure        \
    --prefix=${builds_dir}          \
    --pkg-config-flags=\"--static\" \
    --extra-ldflags=\"${lib_flags}\"\
    --extra-cflags=\"${inc_flags}\" \
"
cmd=${cmd}"\
    --disable-doc              \
    --disable-htmlpages        \
    --disable-manpages         \
    --disable-podpages         \
    --disable-txtpages         \
    --disable-ffserver         \
    --disable-shared           \
    --disable-d3d11va          \
    --disable-dxva2            \
    --disable-vaapi            \
    --disable-vda              \
    --disable-vdpau            \
    --disable-videotoolbox     \
    --disable-bzlib            \
    --disable-iconv            \
    --disable-libxcb           \
    --disable-libxcb-shm       \
    --disable-libxcb-xfixes    \
    --disable-libxcb-shape     \
    --disable-lzma             \
    --disable-xlib             \
    --disable-zlib             \
    --disable-schannel         \
    --disable-securetransport  \
    --disable-outdevs          \
    --disable-indevs           \
"
cmd=${cmd}" --enable-gpl --enable-nonfree --enable-static --enable-indev=lavfi"

cmd=$cmd" $extlib_cmd"
cmd=$cmd" --extra-cflags=''"
cmd=$cmd" --extra-libs=''"

if [ ${fea_list["sdl"]} = "off" ] && [ ${fea_list["sdl2"]} = "off" ];then
    cmd=$cmd" --disable-ffplay"
fi

if [ "$conf" = "debug"  ];then
    cmd="$cmd --disable-optimizations --disable-stripping --disable-asm --disable-pthreads"
fi

echo $cmd
eval $cmd

config_file="config.h"
if [ ! -e $config_file  ];then
    echo "ERROR: ${config_file} does not exist"
    exit
fi

# modify the config.h for cygwin
function replace_macro
{
    local def=$1
    local f=$2
    sed -i "s/.*$def.*/#define $def 0/g" $f
}

#sed -i "s/.*HAVE_ARC4RANDOM.*/#define HAVE_ARC4RANDOM 0/g" config.h

if [ "$sys_name" = "cygwin" ];then
    replace_macro "HAVE_ARC4RANDOM"     "$config_file"
    replace_macro "HAVE_GETTIMEOFDAY"   "$config_file"
#    replace_macro "HAVE_GLOB"           "$config_file"
#    replace_macro "CONFIG_DOC"          "$config_file"
#    replace_macro "HAVE_TERMIOS_H"      "$config_file"
fi

def_file=${current_dir}/defines.h
if [ $ver = "offline" ];then
    new_def_file=${current_dir}/defines_offline.h
    sed 's/\(#define\)\(.*\)[0-1]/\1\20/g' $def_file > $new_def_file
    def_file=$new_def_file
fi

#add defines.h to config.h
if [ -e "$def_file" ];then
    line="#include \"$def_file\""
    cnt=$(grep -n "$line" $config_file|wc -l)
    #echo $cnt
    #cmd1="grep -n \"$line\" $config_file|wc -l"
    #echo $cmd1
    #eval $cmd1
    if [ $cnt -eq 0 ];then
        #echo "defines.h will be included by config.h"
        sed -i '$i'"$line" $config_file 
    elif [ $cnt -eq 1 ];then
        echo "defines.h has already been included by config.h"
    else
        echo "WARNING:Too many including of defines.h in config.h, please check"
    fi
fi

make
#make install
cd -
