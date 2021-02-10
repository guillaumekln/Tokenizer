#! /bin/bash

set -e
set -x

ROOT_DIR=$PWD
ICU_VERSION=${ICU_VERSION:-64.2}

curl -L -O https://github.com/unicode-org/icu/releases/download/release-${ICU_VERSION/./-}/icu4c-${ICU_VERSION/./_}-src.tgz
tar xf icu4c-*-src.tgz
cd icu/source
./runConfigureICU Cygwin/MSVC -prefix=$ROOT_DIR/icu/install -enable-static -disable-shared
make -j2 install
cd $ROOT_DIR

ls $ROOT_DIR/icu/install

# curl -L -O https://github.com/unicode-org/icu/releases/download/release-${ICU_VERSION/./-}/icu4c-${ICU_VERSION/./_}-Win64-MSVC2019.zip
# unzip icu4c-*.zip -d icu

rm -rf build
mkdir build
cd build
cmake -DLIB_ONLY=ON \
      -DCMAKE_INSTALL_PREFIX=$TOKENIZER_ROOT \
      -DICU_INCLUDE_DIRS=$ROOT_DIR/icu/install/include \
      -DICU_LIBRARIES=$ROOT_DIR/icu/install/lib64/icuuc.lib \
      ..
cmake --build . --config Release --target install

cp $ROOT_DIR/icu/bin64/icudt68.dll $ROOT_DIR/bindings/python/pyonmttok/icudt.dll
cp $ROOT_DIR/icu/bin64/icuuc68.dll $ROOT_DIR/bindings/python/pyonmttok/icuuc.dll
cp $TOKENIZER_ROOT/bin/OpenNMTTokenizer.dll $ROOT_DIR/bindings/python/pyonmttok/
