# 1. Promenljive - kompajler, flegovi
CXX = g++
CXXFLAGS = -Wall -Wextra -g -std=c++17 -I. -MMD -MP # -I. kaže da traži headere u trenutnom folderu
LDFLAGS = -g

# 2. Ime konačnog programa
TARGET = simulator

# 3. Automatsko pronalaženje svih .cpp fajlova
SRCS = $(wildcard *.cpp)

# 4. Pretvaranje imena .cpp u .o (objektni fajlovi)
OBJS = $(SRCS:.cpp=.o)

# --- PRAVILA ---

# Glavno pravilo: napravi TARGET (simulator) od svih OBJS (.o fajlova)
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

# Pravilo kako se svaki .cpp pretvara u .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Pravilo za čišćenje projekta (briše sve .o fajlove, .d fajlove i izvšnu datoteku)
clean:
	rm -f $(OBJS) $(TARGET) *.d

# Pravilo za ponovno kompajliranje
rebuild: clean $(TARGET)

-include $(OBJS:.o=.d)
