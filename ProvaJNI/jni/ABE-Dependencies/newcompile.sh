TOOLCHAIN_DIR=$(pwd)/android-toolchain
BASE_DIR=$(pwd)
NDK_PATH=~/Android/android-ndk-r11b
# ~/Android/android-ndk-r11b/build/tools/make-standalone-toolchain.sh --platform=android-14 --install-dir=$TOOLCHAIN_DIR --arch=arm

export PATH=$PATH:$TOOLCHAIN_DIR/bin
export CC=$TOOLCHAIN_DIR/bin/arm-linux-androideabi-gcc-4.9
#export CXX="/home/moreno/android-ndk-r9d/toolchains/arm-linux-androideabi-4.6/prebuilt/linux-x86_64/bin/arm-linux-androideabi-g++"

echo $PATH


#
# Compile gmp-6.0.0
#
# cd gmp-6.1.0
# rm -rf build
# mkdir build
# ./configure --prefix=$(pwd)/build --host=arm-linux-androideabi --disable-shared
# make && make install
# cd ..

#
# Compile pbc-0.5.14
#
cd pbc-0.5.14
rm -f -r build
mkdir build
export CPPFLAGS=-I$BASE_DIR/gmp-6.0.0/build/include
export LDFLAGS=-L$BASE_DIR/gmp-6.0.0/build/lib 
./configure --prefix=$(pwd)/build --host=arm-linux-androideabi --disable-shared
make
make install
cd ..


#
# Compile Openssl
#

# cd openssl-1.0.2g
# rm -rf build
# mkdir build
# CC="$TOOLCHAIN_DIR/bin/arm-linux-androideabi-gcc-4.9 -march=armv7-a -mfloat-abi=softfp --sysroot=$NDK_PATH/platforms/android-9/arch-arm"
# ./Configure --prefix=$(pwd)/build android-armv7 no-asm
# make && make install_sw
# cd ..



