# Auxiliary programs for shipping library
CXX = g++
CXXFLAGS = -Wall -Wextra -O2 -I../include
LDFLAGS = -L../build
LIBS = -l:libicl.a

genh: generate_header.cpp ../build/libicl.a
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS) $(LIBS)
