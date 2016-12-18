#!/bin/bash
current_dir=`pwd`

pkgconfig_generate(){
    name=$1
    comment=$2
    version=$3
    libs=$4
    pkg_dir=$5
    cat <<EOF  >"${pkg_dir}/${name}.pc"
prefix=$prefix
exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib
includedir=\${prefix}/include

Name: $name
Description: $comment
Version: $version
Libs: -L\${libdir} -l${name} ${libs}
Cflags: -I\${includedir}
EOF

}

pkg_dir="${current_dir}/../lib/pkgconfig"
test -e ${pkg_dir} || mkdir -p ${pkg_dir}
pkgconfig_generate x264    "H.264 (MPEG4 AVC) encoder library" "0.148.x" "-lpthread -lm -ldl"      "${pkg_dir}"
pkgconfig_generate x265    "H.265/HEVC video encoder"          "1.9"     "-lstdc++ -lm -lrt -ldl"  "${pkg_dir}"
pkgconfig_generate fdk-aac "AAC Codec Library"                 "0.1.4"   "-lm"                     "${pkg_dir}"

