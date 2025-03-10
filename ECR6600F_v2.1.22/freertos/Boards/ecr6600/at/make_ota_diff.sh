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


if [ $# == 3 -a $2 == "to" ] #1.0.0 to 1.0.1
then
	# -f 参数判断 $file 是否存在 1.0.0
#	echo $BOARD_DIR"_diff_"$1
	if [ -f $TOPDIR/build/$BOARD_DIR"_diff_"$1/ECR6600F_*_cpu_0x*.bin ]
	then # 存在
		echo "have file"
		echo $TOPDIR/build/$BOARD_DIR"_diff_"$1/ECR6600F_*_cpu_0x*.bin
	else # 不存在
		rm -rf $TOPDIR/build/$BOARD_DIR"_diff_"$1
		make clean
		make REL_V=$1 OTA_CONFIG_TYPE="OTA_DIFFERENTIAL_UPDATE" all
		rm -rf $TOPDIR/build/$BOARD_DIR/libs $TOPDIR/build/$BOARD_DIR/objs
		cp -r $TOPDIR/build/$BOARD_DIR $TOPDIR/build/$BOARD_DIR"_diff_"$1
		cp -r ./generated ../../../build/$BOARD_DIR"_diff_"$1/.
		echo "* * * * * * * move $1 OK! * * * * * * * * *"
	fi
	
	# -f 参数判断 $file 是否存在 1.0.1
#	echo $BOARD_DIR"_diff_"$3
	if [ -f $TOPDIR/build/$BOARD_DIR"_diff_"$3/ECR6600F_*_cpu_0x*.bin ]
	then # 存在
		echo "have file"
		echo $TOPDIR/build/$BOARD_DIR"_diff_"$3/ECR6600F_*_cpu_0x*.bin
	else # 不存在
		rm -rf $TOPDIR/build/$BOARD_DIR"_diff_"$3
		make clean
		make REL_V=$3 OTA_CONFIG_TYPE="OTA_DIFFERENTIAL_UPDATE" all
		rm -rf $TOPDIR/build/$BOARD_DIR/libs $TOPDIR/build/$BOARD_DIR/objs
		cp -r $TOPDIR/build/$BOARD_DIR $TOPDIR/build/$BOARD_DIR"_diff_"$3
		cp -r ./generated ../../../build/$BOARD_DIR"_diff_"$3/.
		echo "* * * * * * * move $3 OK! * * * * * * * * *"
	fi
	
	#生成压缩包
	echo ""
	echo "* * * * * * Generate AB upgrade package * * * * * * *"
	$TOPDIR/scripts/bin/ota_tool/ota_tool$HOSTEXEEXT diff ref_bin/*Partition*.bin $TOPDIR/build/$BOARD_DIR"_diff_"$1/ECR6600F_*_cpu_0x*.bin $TOPDIR/build/$BOARD_DIR"_diff_"$3/ECR6600F_*_cpu_0x*.bin $TOPDIR/build/ECR6600F_Diff_ota_package_$1"_to_"$3.bin
else
	if [ $# == 1 ]
	then
		rm -rf $TOPDIR/build/$BOARD_DIR"_diff_"$1
		make clean
		make REL_V=$1 OTA_CONFIG_TYPE="OTA_DIFFERENTIAL_UPDATE" all
		rm -rf $TOPDIR/build/$BOARD_DIR/libs $TOPDIR/build/$BOARD_DIR/objs
		cp -r $TOPDIR/build/$BOARD_DIR $TOPDIR/build/$BOARD_DIR"_diff_"$1
		cp -r ./generated ../../../build/$BOARD_DIR"_diff_"$1/.
		echo "* * * * * * * move $1 OK! * * * * * * * * *"
	else
		echo "input: ./make_ota_diff.sh 1.0.0    or    ./make_ota_diff.sh 1.0.0 to 1.0.1"
	fi
fi

