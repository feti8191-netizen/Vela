CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -pedantic
TARGET = vela

all: $(TARGET)

$(TARGET): vela.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) vela.cpp

clean:
	rm -f $(TARGET)

.PHONY: all clean
