#!/bin/bash

#compile x264
build_dir=`pwd`
cd external_libs/x264*
#./config.ff
./configure \
    --prefix=${build_dir} \
    --disable-cli \
    --enable-static \
make
make install
make clean
cd -

#compile x265
cd external_libs/x265*
cmake -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX="${build_dir}" -DENABLE_SHARED:bool=off -DENABLE_CLI:bool=off ../../source
make
make install
make clean
cd -


cd fdk-aac*
./autogen.sh
./configure --prefix="${build_dir}" --disable-shared
make 
make install

cd -

cd faac*
./bootstrap
./configure --prefix="${build_dir}" --enable-shared=no
make
make install
cd -


#compile libvpx
cd ${build_dir}
../confiugre --prefix=`pwd`
make
make install



#compile SDL

cd ${build_dir}
../configure --prefix=`pwd` --build=i686-pc-mingw32
make
make install
