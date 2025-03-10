#!/bin/bash

project_path=$(cd `dirname $0`; pwd)
BOARD_DIR="${project_path##*/}"
TOPDIR="../../.."


if [ $# -eq 1 ]
then
    rm -rf $TOPDIR/build/standalone_comp_$1
    make clean
    make REL_V=$1 OTA_CONFIG_TYPE="OTA_COMPRESSION_UPDATE" all
    rm -rf $TOPDIR/build/$BOARD_DIR/libs $TOPDIR/build/$BOARD_DIR/objs
    cp -r $TOPDIR/build/$BOARD_DIR $TOPDIR/build/$BOARD_DIR"_comp_"$1
    echo "* * * * * * rename $BOARD_DIR_$1 OK! * * * * * * * *"
    cp -r ./generated $TOPDIR/build/$BOARD_DIR"_comp_"$1/.
    echo "* * * * * * * move generated $1 OK! * * * * * * * * *"

else
    echo "input: ./make_ota_comp.sh 1.0.0"
fi
