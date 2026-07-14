CC = g++
CFLAGS = #-std=c++17 -Wall -g -O3 -march=native -ffast-math
LIBS = -lX11 -lXext -pthread
TARGET = SpaceDefender

ARGS = 10 standard

SRCS = main.cpp Iscrtavanje.cpp NitBroda.cpp NitAsteroida.cpp NitMetaka.cpp Merac.cpp
OBJS = $(SRCS:.cpp=.o)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LIBS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@ $(LIBS)

clean:
	rm -f $(OBJS) $(TARGET)

run:
	./$(TARGET) $(ARGS)