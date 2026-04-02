# Compiler
CC = g++

# Default file
FILE = Data/equilatero_norm.txt

# work according to ground truth
#FILE = Data/Lee_1.txt # OK! but a bit cheating bcs this one should just stay order-1
#FILE = Data/equilatero_norm.txt # OK, I obtain only regions of order 2!
#FILE = Data/example_3.txt # OK!
#FILE = Data/example_4.txt # OK!
#FILE = Data/four_points.txt # Ok, I obtain only regions of order 2!
#FILE = Data/hardest_one.txt # OK! The closed regions is completely partitioned in order-2 regions 
#FILE = Data/hardest_one_perturbated.txt 
#FILE = Data/Lee_fig2.txt # OK! Since I need exactly all points all regions get fused together and I get no region at all
FILE = Data/best_example.txt
#FILE = Data/easy_case.txt
#FILE = Data/Lee_fig2_easier.txt

# not working, why?
#FILE = Data/hard_case.txt

# I don't know the ground truth for these ones
#FILE = Data/example_1.txt 
#FILE = Data/example_2.txt
#FILE = Data/Lee_fig2_2.txt
#FILE = Data/Lee_2.txt

# Probably degenerates cases (like cocircular points or collinear points) that I don't know how to handle yet
#FILE = Data/degenerate_1.txt 
#FILE = Data/cocircular_degeneration.txt 
#FILE = Data/degenerate_2.txt 

# Generic flags that are always valid 
CFLAGS = -std=c++17 -Isrc/MyGAL/include
LIBS =  -lsfml-graphics -lsfml-window -lsfml-system

# The flags 
DEBUG_FLAGS = -g -fsanitize=address -fno-omit-frame-pointer
SRC = src/Main.cpp src/MinKnapsack.cpp src/Point2D.cpp src/UnionFind.cpp src/Diagram_visualization.cpp
OUT = myProgram

OUT_LPC = lpcProgram
SRC_LPC = src/LPC_problem.cpp src/Point2D.cpp src/Diagram_visualization.cpp

# Targets
all: $(OUT) $(OUT_LPC)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LIBS)

$(OUT_LPC): $(SRC_LPC)
	$(CC) $(CFLAGS) $(SRC_LPC) -o $(OUT_LPC) $(LIBS)

debug_build: CFLAGS += $(DEBUG_FLAGS)
debug_build: $(OUT)

run: $(OUT)
	./$(OUT) --file $(FILE) --visualize 1 --minKnapsack 1 --save_diagram 1 --save_image 0

lpc: $(OUT_LPC)
	./$(OUT_LPC) --file $(FILE)

base: $(OUT)
	./$(OUT) --file $(FILE) --visualize 1 --minKnapsack 0

valgrind: debug_build
	valgrind --track-origins=yes --leak-check=full --show-leak-kinds=all ./$(OUT) --file $(FILE) --visualize 0

debug: debug_build
	gdb --args ./myProgram --file $(FILE) --visualize 0

debug_lpc: debug_build
	gdb --args ./lpcProgram --file $(FILE)
	
clean:
	rm -f $(OUT) $(OUT_LPC)