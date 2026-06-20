CXX = g++

CXXFLAGS = -O3 -Wall -std=c++17 -g

OPENCV_CFLAGS = $(shell pkg-config --cflags opencv4)

OPENCV_LIBS = $(shell pkg-config --libs opencv4)

CXXFLAGS += $(OPENCV_CFLAGS)
LDFLAGS = $(OPENCV_LIBS)

TARGET = SpaceDefender

SRCS = main.cpp Iscrtavanje.cpp NitAsteroida.cpp NitBroda.cpp  Merac.cpp NitMetaka.cpp

ARGS = 5 standard

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)

run:
	./$(TARGET) $(ARGS)
