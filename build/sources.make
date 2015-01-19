############ global stuff -- overridden by ../Makefile

ROOT_DIR	?= $(PWD)/..
SRC_DIR		?= $(ROOT_DIR)/src
BUILD_DIR 	?= $(ROOT_DIR)/build
DEST_DIR	?= $(ROOT_DIR)/build.out

ST_DIR		?= $(ROOT_DIR)/core-lib/Smalltalk
EX_DIR		?= $(ROOT_DIR)/core-lib/Examples
TEST_DIR	?= $(ROOT_DIR)/core-lib/TestSuite

############# "component" directories

COMPILER_DIR 	= $(SRC_DIR)/compiler
INTERPRETER_DIR = $(SRC_DIR)/interpreter
MEMORY_DIR 		= $(SRC_DIR)/memory
MISC_DIR 		= $(SRC_DIR)/misc
VM_DIR 			= $(SRC_DIR)/vm
VMOBJECTS_DIR 	= $(SRC_DIR)/vmobjects
UNITTEST_DIR 	= $(SRC_DIR)/unitTests

COMPILER_SRC	= $(wildcard $(COMPILER_DIR)/*.cpp)
COMPILER_OBJ	= $(COMPILER_SRC:.cpp=.o)
INTERPRETER_SRC	= $(wildcard $(INTERPRETER_DIR)/*.cpp)
INTERPRETER_OBJ	= $(INTERPRETER_SRC:.cpp=.o)
MEMORY_SRC		= $(wildcard $(MEMORY_DIR)/*.cpp)
MEMORY_OBJ		= $(MEMORY_SRC:.cpp=.o)
MISC_SRC		= $(wildcard $(MISC_DIR)/*.cpp)
MISC_OBJ		= $(MISC_SRC:.cpp=.o)
VM_SRC			= $(wildcard $(VM_DIR)/*.cpp)
VM_OBJ			= $(VM_SRC:.cpp=.o)
VMOBJECTS_SRC	= $(wildcard $(VMOBJECTS_DIR)/*.cpp)
VMOBJECTS_OBJ	= $(VMOBJECTS_SRC:.cpp=.o)
UNITTEST_SRC	= $(wildcard $(UNITTEST_DIR)/*.cpp)
UNITTEST_OBJ	= $(UNITTEST_SRC:.cpp=.o)


MAIN_SRC		= $(wildcard $(SRC_DIR)/*.cpp)
MAIN_OBJ		= $(MAIN_SRC:.cpp=.o)

############# primitives loading

PRIMITIVESCORE_DIR = $(SRC_DIR)/primitivesCore
PRIMITIVESCORE_SRC = $(wildcard $(PRIMITIVESCORE_DIR)/*.cpp)
PRIMITIVESCORE_OBJ = $(PRIMITIVESCORE_SRC:.cpp=.o)

############# primitives location etc.

PRIMITIVES_DIR	= $(SRC_DIR)/primitives
PRIMITIVES_SRC	= $(wildcard $(PRIMITIVES_DIR)/*.cpp)
PRIMITIVES_OBJ	= $(PRIMITIVES_SRC:.cpp=.o)

############# include path

INCLUDES		=-I$(SRC_DIR)

##############
############## Collections.

CSOM_OBJ		=  $(MEMORY_OBJ) $(MISC_OBJ) $(VMOBJECTS_OBJ) \
				$(COMPILER_OBJ) $(INTERPRETER_OBJ) $(VM_OBJ)

ALL_OBJ			= $(CSOM_OBJ) $(PRIMITIVESCORE_OBJ) $(PRIMITIVES_OBJ) $(MAIN_OBJ)
OBJECTS			= $(ALL_OBJ) $(UNITTEST_OBJ)
ALL_TEST_OBJ= $(CSOM_OBJ) $(PRIMITIVESCORE_OBJ) $(PRIMITIVES_OBJ) $(UNITTEST_OBJ)

SOURCES			=  $(COMPILER_SRC) $(INTERPRETER_SRC) $(MEMORY_SRC) \
				$(MISC_SRC) $(VM_SRC) $(VMOBJECTS_SRC)  \
				$(PRIMITIVES_SRC) $(PRIMITIVESCORE_SRC) $(MAIN_SRC)

############# Things to clean

CLEAN			= $(OBJECTS) $(DIST_DIR) $(DEST_DIR) $(CSOM_NAME)