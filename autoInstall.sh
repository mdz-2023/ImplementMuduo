#!/bin/bash

# 编译报错
set -e 

rm -rf `pwd`/build*

mkdir `pwd`/build

cd `pwd`/build &&
    cmake .. &&
    make

# 回到项目根目录
cd ..

# 把头文件拷贝到 /usr/include/mymuduo  so库拷贝到 /usr/lib
if [ ! -d /usr/include/mymuduo ]; then
    mkdir /usr/include/mymuduo
fi

for headers in `ls *.h`
do
    cp $headers /usr/include/mymuduo
done

cp `pwd`/lib/libmymuduo.so /usr/lib

# 刷新动态库缓存
ldconfig