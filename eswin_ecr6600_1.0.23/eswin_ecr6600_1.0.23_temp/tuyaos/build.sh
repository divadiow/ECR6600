#!/bin/sh
echo $1 $2 $3 $4 $5
APP_BIN_NAME=$1
APP_VERSION=$2
TARGET_PLATFORM=$3
APP_PATH=$4
USER_CMD=$5

echo APP_BIN_NAME=$APP_BIN_NAME
echo APP_VERSION=$APP_VERSION
echo TARGET_PLATFORM=$TARGET_PLATFORM
echo APP_PATH=$APP_PATH
echo USER_CMD=$USER_CMD

export TUYA_APP_NAME=$APP_BIN_NAME

USER_SW_VER=`echo $APP_VERSION | cut -d'-' -f1`

echo USER_SW_VER=$USER_SW_VER

echo "Start Compile"
set -e

MAKEFILE_DIR=$(pwd)
BULID_PATH=$MAKEFILE_DIR
ESWIN_TOP_DIR=$MAKEFILE_DIR/../eswin_ecr6600_os
ROOT_DIR=$ESWIN_TOP_DIR/../../..
APP_DIR=$ROOT_DIR/$APP_PATH

BRD_DIR=$ESWIN_TOP_DIR/Boards/ecr6600/tuya

if [ x"$USER_CMD" = x"" ];then

	USER_CMD=all

fi

DIR_INIT()
{
	ROOT_DIR=$(cd $ROOT_DIR; pwd)
	APP_DIR=$(cd $APP_DIR; pwd)
	BRD_DIR=$(cd $BRD_DIR; pwd)
	mkdir -p $APP_DIR/output
}
COMPILE_HEADER()
{
if [ x"$USER_CMD" = x"all" ]; then
	cd $BRD_DIR
	make pass1dep
fi
}
COMPILE_APP()
{
    echo "Start Compile $APP_DIR"

    cd $APP_DIR
    if [ x"$GCC_PATH" = x"" ]; then
        export GCC_PATH=$ROOT_DIR/vendor/eswin_ecr6600/toolchain/nds32le-elf-mculib-v3s/bin/
    fi
    rm -rf $APP_DIR/output/libs $APP_DIR/output/objs
    make APP_BIN_NAME=$APP_BIN_NAME USER_SW_VER=$APP_VERSION APP_VERSION=$APP_VERSION APP_PATH=$APP_PATH $USER_CMD
}

COPY_LIB()
{
    echo "Start copy libs"

    if [ x"$USER_CMD" = x"all" ]; then
        cp $ROOT_DIR/libs/* -rf $ROOT_DIR/vendor/eswin_ecr6600/eswin_ecr6600_os/libs/
    fi
}

COMPILE_PLATFORM()
{
    echo "Start Compile $BRD_DIR"
    
    echo "**************************************************************************"
    cd $BRD_DIR
    if [ x"$GCC_PATH" = x"" ]; then
        export GCC_PATH=$ROOT_DIR/vendor/eswin_ecr6600/toolchain/nds32le-elf-mculib-v3s/bin/
    fi

    make SDK_V=V2.0.0B06 EXTRA_SDK_DIR=$ROOT_DIR APP_BIN_NAME=$APP_BIN_NAME $USER_CMD
    if [ x"$USER_CMD" = x"all" ]; then
        mkdir -p $APP_DIR/output/$APP_VERSION
        src=$ROOT_DIR/vendor/eswin_ecr6600/eswin_ecr6600_os/build/$APP_NAME/ECR6600F_${APP_NAME}_cpu_0x9000.bin
        dst=$APP_DIR/output/$APP_VERSION/${APP_BIN_NAME}_STU_${APP_VERSION}.bin
        python3 $BULID_PATH/format_up_bin.py $src $dst 1CC000 4000 0 1000
        cp $ROOT_DIR/vendor/eswin_ecr6600/eswin_ecr6600_os/build/$APP_NAME/ECR6600F_tuya_${APP_NAME}_stub_cpu_inone.bin $APP_DIR/output/$APP_VERSION/${APP_BIN_NAME}_UA_$APP_VERSION.bin
        cp $ROOT_DIR/vendor/eswin_ecr6600/eswin_ecr6600_os/build/$APP_NAME/ECR6600F_tuya_${APP_NAME}_stub_cpu_inone.bin $APP_DIR/output/$APP_VERSION/${APP_BIN_NAME}_UG_${APP_VERSION}.bin
        cp $ROOT_DIR/vendor/eswin_ecr6600/eswin_ecr6600_os/build/$APP_NAME/ECR6600F_tuya_${APP_NAME}_allinone.bin $APP_DIR/output/$APP_VERSION/${APP_BIN_NAME}_QIO_$APP_VERSION.bin
        cp $ROOT_DIR/vendor/eswin_ecr6600/eswin_ecr6600_os/build/$APP_NAME/${APP_NAME}.elf $APP_DIR/output/$APP_VERSION/${APP_BIN_NAME}_$APP_VERSION.elf
        cp $ROOT_DIR/vendor/eswin_ecr6600/eswin_ecr6600_os/build/$APP_NAME/${APP_NAME}.map $APP_DIR/output/$APP_VERSION/${APP_BIN_NAME}_$APP_VERSION.map
        cp $ROOT_DIR/vendor/eswin_ecr6600/eswin_ecr6600_os/build/$APP_NAME/${APP_NAME}.lst $APP_DIR/output/$APP_VERSION/${APP_BIN_NAME}_$APP_VERSION.lst

        FW_NAME=$APP_NAME
        if [ -n $CI_IDENTIFIER ]; then
            FW_NAME=$CI_IDENTIFIER
        fi

        if [ -z $CI_PACKAGE_PATH ]; then
            echo "not is ci build"
            exit
        else
            mkdir -p ${CI_PACKAGE_PATH}
           
           cp $ROOT_DIR/apps/$APP_BIN_NAME/output/$APP_VERSION/${APP_BIN_NAME}_STU_${APP_VERSION}.bin ${CI_PACKAGE_PATH}/$FW_NAME"_STU_"$APP_VERSION.bin
           cp $ROOT_DIR/apps/$APP_BIN_NAME/output/$APP_VERSION/${APP_BIN_NAME}_UG_${APP_VERSION}.bin ${CI_PACKAGE_PATH}/$FW_NAME"_UG_"$APP_VERSION.bin
           cp $ROOT_DIR/apps/$APP_BIN_NAME/output/$APP_VERSION/${APP_BIN_NAME}_UA_${APP_VERSION}.bin ${CI_PACKAGE_PATH}/$FW_NAME"_UA_"$APP_VERSION.bin
           cp $ROOT_DIR/apps/$APP_BIN_NAME/output/$APP_VERSION/${APP_BIN_NAME}_QIO_${APP_VERSION}.bin ${CI_PACKAGE_PATH}/$FW_NAME"_QIO_"$APP_VERSION.bin
           cp $ROOT_DIR/apps/$APP_BIN_NAME/output/$APP_VERSION/${APP_BIN_NAME}_${APP_VERSION}.elf ${CI_PACKAGE_PATH}/$FW_NAME"_"$APP_VERSION.elf
           cp $ROOT_DIR/apps/$APP_BIN_NAME/output/$APP_VERSION/${APP_BIN_NAME}_${APP_VERSION}.map ${CI_PACKAGE_PATH}/$FW_NAME"_"$APP_VERSION.map
           cp $ROOT_DIR/apps/$APP_BIN_NAME/output/$APP_VERSION/${APP_BIN_NAME}_${APP_VERSION}.lst ${CI_PACKAGE_PATH}/$FW_NAME"_"$APP_VERSION.lst
        fi
    fi
}

DIR_INIT
COMPILE_HEADER
#COMPILE_APP
#COPY_LIB
COMPILE_PLATFORM
