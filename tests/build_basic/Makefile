# Bootstrap the build system
# With REBUILD_URSELF, you just compile it once with hand like this:
buicpp: build.cpp
	g++ -std=c++20 -o buic build.cpp -I../../include -L../../build -l:libicl.a
# And you're good to go!
# Just run ./buic directly and it'll compile itself, no need to call this Makefile
# But if you lost the binary you always can bootstrap again
