# CSE 4403 (Algorithms) -- Assignment 2
# Emergency Blood Supply Distribution and Hospital Allocation System

CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -MMD -MP
TARGET   = bloodflow
SRC      = $(wildcard src/*.cpp)
OBJ      = $(SRC:.cpp=.o)
DEP      = $(OBJ:.o=.d)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

sample: $(TARGET)
	./$(TARGET) data/sample_scenario.txt

clean:
	rm -f $(OBJ) $(DEP) $(TARGET) run_tests

-include $(DEP)

.PHONY: all run sample clean test

LIBSRC = src/graph.cpp src/maxflow.cpp src/knapsack.cpp src/priority.cpp src/model.cpp

test: tests/test_algorithms.cpp $(LIBSRC)
	$(CXX) $(CXXFLAGS) -o run_tests $^ && ./run_tests
