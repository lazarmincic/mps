CC = g++
CFLAGS = -std=c++17 -Wall
LIBS = -lsfml-graphics -lsfml-window -lsfml-system -pthread
TARGET = SpaceDefender

ARGS = 2 hard

SRCS = main.cpp Iscrtavanje.cpp NitBroda.cpp NitAsteroida.cpp NitMetaka.cpp Merac.cpp

OBJS = $(SRCS:.cpp=.o)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LIBS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

run:
	DISPLAY=:0 ./$(TARGET) $(ARGS)
