CXX = g++
CXXFLAGS = -std=c++11 -Wall -Werror -pedantic
LDLIBS = -lncurses

SRC_FILES = $(wildcard *.cpp)
OBJ_FILES = $(patsubst %.cpp, %.o, $(SRC_FILES))

BINARY := file_manager

all: $(BINARY)

$(BINARY): $(OBJ_FILES)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o
	rm -f $(BINARY)

.PHONY: all clean

