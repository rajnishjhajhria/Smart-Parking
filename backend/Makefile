CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -Iinclude
TARGET   = parking_server

ifeq ($(OS),Windows_NT)
LDLIBS += -lws2_32
endif

.PHONY: all clean run

all: $(TARGET)

$(TARGET): main.cpp include/database.h include/http_server.h include/router.h
	$(CXX) $(CXXFLAGS) -o $(TARGET) main.cpp $(LDLIBS)
	@echo "\n✅ Build successful → ./$(TARGET)\n"

run: all
	./$(TARGET)

clean:
	rm -f $(TARGET)
