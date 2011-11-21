LIBMOPEDDIR := ../..
NAME := files

# Compiler flags 
CXX = g++

CXXFLAGS = -O3 -Wall -march=native -mfpmath=sse -fopenmp 
CXXFLAGS += -ffast-math -fno-trapping-math -fsingle-precision-constant -funroll-loops

INCLUDES = -I$(LIBMOPEDDIR)/include/ 
DLIBS = -lgomp -lGLEW -lglut -lIL -lcv -lhighgui -lcvaux

# Targets
.PHONY : all clean

all : $(NAME)

$(NAME) : $(NAME).cpp 
	@cd $(LIBMOPEDDIR); $(MAKE) --no-print-directory
	$(CXX) $(CXXFLAGS) $(INCLUDES) -Wl,--start-group $(DLIBS) $(LIBMOPEDDIR)/lib/libmoped.a $< -o $@

clean:
	$(RM) $(NAME)

