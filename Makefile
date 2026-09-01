CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
CPPFLAGS := -Isrc
LDLIBS   := -lsfml-graphics -lsfml-window -lsfml-audio -lsfml-system

BIN := space_breakers
SRC := $(shell find src -name '*.cpp')
OBJ := $(SRC:.cpp=.o)
DEP := $(OBJ:.o=.d)

$(BIN): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ) $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c -o $@ $<

-include $(DEP)

run: $(BIN)
	./$(BIN)

clean:
	rm -f $(OBJ) $(DEP) $(BIN)

.PHONY: run clean
