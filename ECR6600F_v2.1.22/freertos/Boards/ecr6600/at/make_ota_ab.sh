#!/bin/bash

project_path=$(cd `dirname $0`; pwd)
BOARD_DIR="${project_path##*/}"
TOPDIR="../../.."
HOSTOS=$(shell uname -o 2>/dev/null || echo "Other")
HOSTEXEEXT=""
if [ $HOSTOS == "Cygwin" ]
then
	HOSTEXEEXT = ".exe"
fi


if [ $# -eq 1 ]
then
	rm -rf $TOPDIR/build/$BOARD_DIR_$1_A
	make clean 
	make REL_V=$1 OTA_CONFIG_TYPE="OTA_AB_UPDATE" all
	rm -rf $TOPDIR/build/$BOARD_DIR/libs $TOPDIR/build/$BOARD_DIR/objs
	cp -r $TOPDIR/build/$BOARD_DIR $TOPDIR/build/$BOARD_DIR"_AB_"$1"_A"
	cp -r ./generated $TOPDIR/build/$BOARD_DIR"_AB_"$1"_A"/.
	echo "* * * * * * * move A OK! * * * * * * * * *"

	if [ $1 != "1.0.0" ] 
	then
		grep_str=$(grep -az cpu ref_bin/*Partition*.bin)
		cut_head=${grep_str:4}
		cut_tail=${cut_head%?}
		start_addr_16=${cut_tail%,*}
		len_16=${cut_tail#*,}
		((start_addr=$start_addr_16))
		((len=$len_16))
		B_xip_addr=`expr $len / 2 + $start_addr`
		B_xip_addr_HEX=`printf "0x%x" $B_xip_addr`
		echo "B part XIP_ADDR=$B_xip_addr_HEX"

		rm -rf $TOPDIR/build/$BOARD_DIR_$1_B
		make clean 
		make REL_V=$1 OTA_CONFIG_TYPE="OTA_AB_UPDATE" XIP_ADDR=$B_xip_addr_HEX all
		rm -rf $TOPDIR/build/$BOARD_DIR/libs $TOPDIR/build/$BOARD_DIR/objs
		cp -r $TOPDIR/build/$BOARD_DIR $TOPDIR/build/$BOARD_DIR"_AB_"$1"_B"
		cp -r ./generated $TOPDIR/build/$BOARD_DIR"_AB_"$1"_B"/.
		echo "* * * * * * * move B OK! * * * * * * * * *"
		echo ""
		echo "* * * * * * Generate AB upgrade package * * * * * * *"
#		echo $TOPDIR/scripts/bin/ota_tool/ota_tool$HOSTEXEEXT
#		echo ref_bin/*Partition*.bin
#		echo $TOPDIR/build/$BOARD_DIR"_AB_"$1"_A"/ECR6600F_*_cpu_0x*.bin
#		echo $TOPDIR/build/$BOARD_DIR"_AB_"$1"_B"/ECR6600F_*_cpu_0x*.bin
#		echo $TOPDIR/build/ECR6600F_ab_ota_package_$1.bin
		$TOPDIR/scripts/bin/ota_tool/ota_tool$HOSTEXEEXT ab ref_bin/*Partition*.bin $TOPDIR/build/$BOARD_DIR"_AB_"$1"_A"/ECR6600F_*_cpu_0x*.bin $TOPDIR/build/$BOARD_DIR"_AB_"$1"_B"/ECR6600F_*_cpu_0x*.bin $TOPDIR/build/ECR6600F_AB_ota_package_$1.bin
	fi
else
    echo "input: ./make_ota_ab.sh 1.0.0"
fi
