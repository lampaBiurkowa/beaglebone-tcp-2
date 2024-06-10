CXX = g++
CXXFLAGS = -pthread
TARGET = main
SRC = main.cpp

main:
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)

