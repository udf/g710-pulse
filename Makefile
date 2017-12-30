CXX=g++
CXXFLAGS=-g -std=c++1z -Wall -pedantic -lopenal -lfftw3
CXXFLAGS += -lhidapi-hidraw
CXXFLAGS += -Ofast

all:
	$(CXX) $(CXXFLAGS) ModularSpec/OpenALDataFetcher.cpp ModularSpec/Spectrum.cpp ModularSpec/util.cpp g710_pulse.cpp -o g710_pulse

clean:
	rm g710_pulse