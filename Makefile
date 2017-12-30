CXX=g++
CXXFLAGS=-g -std=c++1z -Wall -pedantic -lopenal -lfftw3
CXXFLAGS += -lhidapi-hidraw
CXXFLAGS += -Ofast

all:
	$(CXX) $(CXXFLAGS) ModularViz/OpenALDataFetcher.cpp ModularViz/Spectrum.cpp ModularViz/util.cpp g710_pulse.cpp -o g710_pulse

clean:
	rm example