#!/bin/bash
g++ main.cpp -o conv
for file in sample/rgba/*.qoi
do
    filenamenoext=${file%.qoi}
    filenamenodir=${filenamenoext##*/}
    echo "Decoding "$filenamenoext.qoi"..."
    /usr/bin/time -f "Execute time: %E" ./conv -d -4 <$file >$filenamenodir.pam
    ans=$(diff -q $filenamenoext.pam $filenamenodir.pam)
    rm $filenamenodir.pam
    if [ "$ans" ]; then
        echo "The output is wrong."
    else
        echo "The output is right."
    fi
    echo
done
for file in sample/rgba/*.pam
do
    filenamenoext=${file%.pam}
    filenamenodir=${filenamenoext##*/}
    echo "Encoding "$filenamenoext.pam"..."
    /usr/bin/time -f "Execute time: %E" ./conv -e -4 <$file >$filenamenodir.qoi
    ans=$(diff -q $filenamenoext.qoi $filenamenodir.qoi)
    rm $filenamenodir.qoi
    if [ "$ans" ]; then
        echo "The output is wrong."
    else
        echo "The output is right."
    fi
    echo
done
for file in sample/rgb/*.qoi
do
    filenamenoext=${file%.qoi}
    filenamenodir=${filenamenoext##*/}
    echo "Decoding "$filenamenoext.qoi"..."
    /usr/bin/time -f "Execute time: %E" ./conv -d -3 <$file >$filenamenodir.ppm
    ans=$(diff -q $filenamenoext.ppm $filenamenodir.ppm)
    rm $filenamenodir.ppm
    if [ "$ans" ]; then
        echo "The output is wrong."
    else
        echo "The output is right."
    fi
    echo
done
for file in sample/rgb/*.ppm
do
    filenamenoext=${file%.ppm}
    filenamenodir=${filenamenoext##*/}
    echo "Encoding "$filenamenoext.ppm"..."
    /usr/bin/time -f "Execute time: %E" ./conv -e -3 <$file >$filenamenodir.qoi
    ans=$(diff -q $filenamenoext.qoi $filenamenodir.qoi)
    rm $filenamenodir.qoi
    if [ "$ans" ]; then
        echo "The output is wrong."
    else
        echo "The output is right."
    fi
    echo
done