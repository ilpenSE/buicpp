CXXFLAGS = -std=c++23 -I../../include
LDFLAGS = -L../../build
LIBS = -l:libicl.a

main: main.cpp test.cpp
	g++ $(CXXFLAGS) main.cpp test.cpp $(LDFLAGS) $(LIBS) -o main
