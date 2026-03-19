# 1. Promenljive - ovde definišeš kompajler i zastavice
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -I. # -I. kaže da traži headere u trenutnom folderu
LDFLAGS =

# 2. Ime tvog konačnog programa
TARGET = simulator

# 3. Automatsko pronalaženje svih .cpp fajlova
# Ovo će naći main.cpp, CPU.cpp, Cache.cpp, Bus.cpp itd.
SRCS = $(wildcard *.cpp)

# 4. Pretvaranje imena .cpp u .o (Object fajlovi)
# npr. CPU.cpp postaje CPU.o
OBJS = $(SRCS:.cpp=.o)

# --- PRAVILA ---

# Glavno pravilo: napravi TARGET (simulator) od svih OBJS (.o fajlova)
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

# Pravilo kako se svaki .cpp pretvara u .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Pravilo za čišćenje projekta (da obrišeš .o i simulator)
clean:
	rm -f $(OBJS) $(TARGET)

# Pravilo za ponovno kompajliranje od nule
rebuild: clean $(TARGET)
