CXX = clang-cl
CXXFLAGS += -utf-8 -std:c++latest -EHsc -GR- -W4 -Werror=gnu -Wmicrosoft -Wno-missing-field-initializers -Wpedantic

reflink: reflink.cpp main.cpp
clean:
	rm reflink.exe