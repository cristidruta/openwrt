target:=cpp11-demo
objects=$(subst .cpp,.o,$(wildcard *.cpp))

CXXFLAGS += -std=gnu++11

$(target):$(objects)
	$(CXX) -o $@ $^

clean:
	-@rm $(target)
	-@rm $(objects)
