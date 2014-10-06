#!/bin/bash

if [ $# -lt 2 ]
	then
		echo "build-tool.sh <version-num> <conter> [type I|O]"
		echo "Sample: ./build-tool.sh 13 000 I"
		exit 1
fi

if [ $# -eq 3 ]
	then
		type=-$3
	else
		typ3=
fi

curPath=`pwd`
date=`date +%y%m%d`
versionMajor=0
versionMinor=$1
update=_update_V$versionMajor\_$versionMinor\_$2\_$date

outFile=HB-UW-Sen-THPL$type$update
outPath=../Firmware-Release
fullFile=$outPath/$outFile

if [ $# -lt 3 ]
	then
		./hex2eq3.php --inFile ../Firmware-Src/Release/WetterSensor.hex --spmPageSize 128 --outFormat eq3 --outFile $fullFile.eq3 --withCrcCheck
		./hex2eq3.php --inFile ../Firmware-Src/Release/WetterSensor.hex --spmPageSize 128 --outFormat hex --outFile $fullFile.hex
		cp $fullFile.eq3 ./

		rm -rf ../Firmware-Release/tmp
		mkdir ../Firmware-Release/tmp
		cp $fullFile.eq3 ../Firmware-Release/tmp
		cp ../Firmware-Src/ccu-changelog.txt ../Firmware-Release/tmp/changelog.txt
		
		echo "TypeCode=61698"                               > ../Firmware-Release/tmp/info
		echo "Name=HB-UW-Sen-THPL-O"                       >> ../Firmware-Release/tmp/info
		echo "CCUFirmwareVersionMin=2.9.0"                 >> ../Firmware-Release/tmp/info
		echo "FirmwareVersion=$versionMajor.$versionMinor" >> ../Firmware-Release/tmp/info
		cd $outPath/tmp
		tar -H gnu -zcvf $outFile-O.tgz ./*
		cd $curPath
		mv $outPath/tmp/$outFile-O.tgz $outPath
		
		rm $outPath/tmp/info
		echo "TypeCode=61697"                               > ../Firmware-Release/tmp/info
		echo "Name=HB-UW-Sen-THPL-I"                       >> ../Firmware-Release/tmp/info
		echo "CCUFirmwareVersionMin=2.9.0"                 >> ../Firmware-Release/tmp/info
		echo "FirmwareVersion=$versionMajor.$versionMinor" >> ../Firmware-Release/tmp/info
		cd $outPath/tmp
		tar -H gnu -zcvf $outFile-I.tgz ./*
		cd $curPath
		mv $outPath/tmp/$outFile-I.tgz $outPath

		rm -rf ../Firmware-Release/tmp

	else
		cp ../Firmware-Src/Release/WetterSensor.hex $outFile.hex
fi

