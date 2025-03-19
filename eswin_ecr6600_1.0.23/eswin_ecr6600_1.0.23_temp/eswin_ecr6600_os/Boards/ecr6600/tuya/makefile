.PHONY: ${MAKECMDGOALS}
BOARD_DIR := ${shell echo $(CURDIR) | sed -e 's/ /\\ /g'}
TOPDIR := $(realpath ${BOARD_DIR}/../../..)


APP_NAME ?= $(BOARD_DIR)

${MAKECMDGOALS}:
	$(info info text)
	echo ${MAKEFLAGS}
	make -C ${TOPDIR} BOARD_DIR=${BOARD_DIR} APP_NAME=${APP_NAME} -${MAKEFLAGS} ${MAKECMDGOALS} 
	


