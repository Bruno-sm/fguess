CXX=g++
FLAGS=-g -O3 -std=c++11 -pg
HEAD_DIR=./include
SRC_DIR=./src
OBJ_DIR=./obj
LIB_DIR=./libs
BIN_DIR=./bin
DOXYFILE=./doc/doxys/Doxyfile



all: training

training: ${BIN_DIR}/training

${BIN_DIR}/training: ${OBJ_DIR}/training.o ${LIB_DIR}/libboost_filesystem.a ${LIB_DIR}/libboost_system.a
	${CXX} ${FLAGS} -I ${HEAD_DIR} $^ -o $@

${OBJ_DIR}/training.o: ${HEAD_DIR}/regex.hpp ${HEAD_DIR}/file_templates.hpp ${HEAD_DIR}/boost ${SRC_DIR}/training.cpp
	${CXX} ${FLAGS} -I ${HEAD_DIR} -c ${SRC_DIR}/training.cpp -o $@

doc: ${HEAD_DIR}/* ${SRC_DIR}/* ${DOXYFILE}
	doxygen ${DOXYFILE}
