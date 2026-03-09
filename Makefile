THISFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
THISFILE_DIR  := $(realpath $(dir ${THISFILE_PATH}))

MAINGOAL := $(firstword $(MAKECMDGOALS))

# setup default configuration (iff do_configure is the main goal)
ifeq ($(MAINGOAL),do_configure)
ifeq (${CFG_GENERATOR},) # set default generator
	CFG_GENERATOR="Ninja"
endif
ifeq (${CFG_BUILD_TYPE},) # set default build type
	CFG_BUILD_TYPE="RelWithDebInfo"
endif
ifeq (${CFG_BUILD_NAME},) # set default build name
	CFG_BUILD_NAME=default
endif
endif

GCC_VER?=-15
CLANG_VER?=-22

SOURCE_ROOT_DIR = .
BUILD_ROOT_DIR ?= .build
BUILD_LAST_DIR ?= ${BUILD_ROOT_DIR}/the-latest

# locate build dir by looking for the 'last'
BUILD_DIR := $(shell realpath --canonicalize-existing --quiet ${BUILD_LAST_DIR})

# locate compile_commands.json if any
COMPILE_COMMANDS_FILE := $(shell realpath --canonicalize-existing --quiet compile_commands.json)

all: has-build-dir
	cmake --build ${BUILD_DIR}

.PHONY=test
test: has-build-dir
	cmake --build ${BUILD_DIR} --target test

.PHONY=clean
clean: has-build-dir
	cmake --build ${BUILD_DIR} --target clean

.PHONY=has-build-dir
has-build-dir:
ifeq ($(strip $(BUILD_DIR)),)
	@echo "No build dir detected. Run with 'configure-*' first."
	@exit 1
else
	@if [ ! -d "${BUILD_DIR}" ]; then echo "No build dir found [BUILD_DIR=${BUILD_DIR}]"; exit 1; fi
endif

.PHONY=do_configure
do_configure:
	@echo "Running configuration [${CFG_BUILD_NAME}]"
	cmake -G ${CFG_GENERATOR} -B ${BUILD_ROOT_DIR}/${CFG_BUILD_NAME} -S ${SOURCE_ROOT_DIR} \
		-D FETCHCONTENT_BASE_DIR="${BUILD_ROOT_DIR}/extern-deps/" \
		-D CMAKE_EXPORT_COMPILE_COMMANDS=YES \
		-D CMAKE_COLOR_DIAGNOSTICS=YES \
		-D CMAKE_BUILD_TYPE=${CFG_BUILD_TYPE}
	@# remove the 'last' symlink if any, and ensure there is no real dir named 'last'
	$(shell test -L ${BUILD_LAST_DIR} && rm -f ${BUILD_LAST_DIR})
	$(shell test -d ${BUILD_LAST_DIR} && echo "Unexpected directory [${BUILD_LAST_DIR}]." && exit -1)
	@# symlink the 'last'
	ln -s -f -d -r ${BUILD_ROOT_DIR}/${CFG_BUILD_NAME} ${BUILD_LAST_DIR}
	ln -s -f ${BUILD_ROOT_DIR}/${CFG_BUILD_NAME}/compile_commands.json ${SOURCE_ROOT_DIR}/compile_commands.json
	@echo "Done."

#
# configuration presets
#

.PHONY=configure-gcc
configure-gcc:
	@make -f ${THISFILE_PATH} do_configure \
		CC=gcc${GCC_VER} CXX=g++${GCC_VER} \
		CFG_BUILD_NAME="gcc${GCC_VER}" 

.PHONY=configure-clang
configure-clang:
	@make -f ${THISFILE_PATH} do_configure \
		CC=clang${CLANG_VER} CXX=clang++${CLANG_VER} \
		CFG_BUILD_NAME="clang${CLANG_VER}" \


.PHONY=configure-gcc-debug
configure-gcc-debug:
	@make -f ${THISFILE_PATH} do_configure \
		CC=gcc${GCC_VER} CXX=g++${GCC_VER} \
		CFG_BUILD_TYPE="Debug" \
		CFG_BUILD_NAME="gcc${GCC_VER}-debug" 

.PHONY=configure-clang-debug
configure-clang-debug:
	@make -f ${THISFILE_PATH} do_configure \
		CC=clang${CLANG_VER} CXX=clang++${CLANG_VER} \
		CFG_BUILD_TYPE="Debug" \
		CFG_BUILD_NAME="clang${CLANG_VER}-debug" \


.PHONY=configure-gcc-release
configure-gcc-release:
	@make -f ${THISFILE_PATH} do_configure \
		CC=gcc${GCC_VER} CXX=g++${GCC_VER} \
		CFG_BUILD_TYPE="Release" \
		CFG_BUILD_NAME="gcc${GCC_VER}-release" 

.PHONY=configure-clang-release
configure-clang-release:
	@make -f ${THISFILE_PATH} do_configure \
		CC=clang${CLANG_VER} CXX=clang++${CLANG_VER} \
		CFG_BUILD_TYPE="Release" \
		CFG_BUILD_NAME="clang${CLANG_VER}-release" \

#
#
#

.PHONY = coverage
coverage: has-build-dir
	mkdir -p ${BUILD_DIR}/coverage-report
	# running lcov
	lcov -c -d ${SOURCE_ROOT_DIR} -o ${BUILD_DIR}/coverage-report/report.info --rc branch_coverage=1 \
        --no-external \
        --ignore-errors=version,version \
        --ignore-errors=empty,unused \
        --ignore-errors=unused,unused \
        --exclude "${BUILD_ROOT_DIR}/*" \
	# generating HTML report
	genhtml -o ${BUILD_DIR}/coverage-report/ ${BUILD_DIR}/coverage-report/report.info --rc branch_coverage=1 --dark-mode --flat

