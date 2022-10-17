CFLAGS = `pkg-config --cflags opencv4`
LIBS = `pkg-config --libs opencv4`

% : %.cpp
		g++ -std=c++11 $(CFLAGS) $(LIBS) -o $@ $<