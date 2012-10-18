CC=g++
EXECUTABLE=minirt
SRC_DIR=src
INTERM_DIR=obj

INCLUDES=-I $(SRC_DIR)
LIBS=-lpng -stdc++ -fopenmp
CFLAGS_COMMON=$(INCLUDES)
CFLAGS=$(CFLAGS_COMMON) -O3 -DNDEBUG -fopenmp
ifeq ($(_NO_ACCELERATION),1) #check if acceleration structures should be used
CFLAGS+= -DNO_ACC=1 #disable acceleration
endif
#CFLAGS=$(CFLAGS_COMMON) -g -O0 -D_DEBUG -fopenmp

#WARNINGS=-Wall
#WARNINGS+= -Wno-pragmas
WARNINGS=-Waddress -Wc++0x-compat -Wchar-subscripts -Wcomment -Wformat -Wmissing-braces -Wparentheses -Wreorder -Wreturn-type -Wsequence-point -Wstrict-aliasing -Wstrict-overflow=1 -Wswitch -Wtrigraphs -Wuninitialized -Wunused-function -Wunused-label -Wunused-value -Wunused-variable -Wvolatile-register-var 
WARNINGS+= -pedantic

SOURCE_FILES=$(shell find $(SRC_DIR) -iname '*.cpp')
DEP_FILES=$(SOURCE_FILES:$(SRC_DIR)/%.cpp=./$(INTERM_DIR)/%.dep)
OBJ_FILES=$(SOURCE_FILES:$(SRC_DIR)/%.cpp=./$(INTERM_DIR)/%.o)

all: $(EXECUTABLE)

clean:
	rm -rf obj $(EXECUTABLE)

.PHONY: clean all

.SUFFIXES:
.SUFFIXES:.o .dep .cpp .h

$(INTERM_DIR)/%.dep: $(SRC_DIR)/%.cpp
	mkdir -p `dirname $@`
	echo -n `dirname $@`/ > $@
	$(CC) $(CFLAGS_COMMON) $< -MM | sed -r -e 's,^(.*)\.o\s*\:,\1.o $@ :,g' >> $@

ifneq ($(MAKECMDGOALS),clean)
-include $(DEP_FILES)
endif

$(INTERM_DIR)/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(INTERM_DIR)
	$(CC) $(CFLAGS) $(WARNINGS) -c $< -o $@

$(EXECUTABLE): $(OBJ_FILES)
	$(CC) $^ $(LIBS) -o $@
