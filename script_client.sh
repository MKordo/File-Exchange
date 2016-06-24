#!/bin/bash

ip="localhost"
#ip="linux18.di.uoa.gr"

mkdir "Client1"
mkdir "Client2"
mkdir "Client3"
mkdir "Client4"

cp remoteClient Client1/
cp remoteClient Client2/
cp remoteClient Client3/
cp remoteClient Client4/

cd Client1/
./remoteClient -i $ip -p 3060 -d folder1
cd ..

cd Client2/
./remoteClient -i $ip -p 3060 -d folder1/folder2
cd ..

cd Client3/
./remoteClient -i $ip -p 3060 -d folder1/folder2
cd ..

cd Client4/
./remoteClient -i $ip -p 3060 -d folder1
cd ..

echo "///////   SCRIPT   ///////"
echo "Lets see what was copied"
echo

echo "1st client:"
ls Client1/folder1
ls Client1/folder1/folder2

echo
echo "2st client:"
ls Client2/folder1
ls Client2/folder1/folder2

echo
echo "3st client:"
ls Client3/folder1
ls Client3/folder1/folder2

echo
echo "4st client:"
ls Client4/folder1
ls Client4/folder1/folder2

echo
echo
echo

rm -rf Client1/
rm -rf Client2/
rm -rf Client3/
rm -rf Client4/


