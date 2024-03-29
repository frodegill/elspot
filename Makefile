# Based on Makefile from <URL: http://hak5.org/forums/index.php?showtopic=2077&p=27959 >

PROGRAM = elspot

############# Main application #################
all:    $(PROGRAM)
.PHONY: all

# source files
#DEBUG_INFO = YES
SOURCES = $(shell find -L . -name '*.cpp'|grep -v "/example/"|grep -v "/tests/"|sort)
OBJECTS = $(SOURCES:.cpp=.o)
DEPS = $(OBJECTS:.o=.dep)

######## compiler- and linker settings #########
WX_CONFIG := wx-config
#ifdef WXWIDGETS_VERSION
 WX_CONFIG += --version=$(WXWIDGETS_VERSION)
#endif
CXX = g++
CXXFLAGS = -I/usr/local/include -I/usr/include -W -Wall -Werror -Wextra -Wconversion -pipe -std=c++20
LIBSFLAGS = -L/usr/local/library -L/usr/local/lib -L/usr/local/lib/x86_64 -lpthread -lpaho-mqtt3as -lpaho-mqttpp3 -lfmt

ifdef DEBUG_INFO
 CXXFLAGS += -g
 LIBSFLAGS +=  -lPocoFoundationd -lPocoJSONd -lPocoXMLd -lPocoNetd -lPocoNetSSLd -lPocoCryptod -lPocoUtild
else
 CXXFLAGS += -O3
 LIBSFLAGS +=  -lPocoFoundation -lPocoJSON -lPocoXML -lPocoNet -lPocoNetSSL -lPocoCrypto -lPocoUtil
endif

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

%.dep: %.cpp
	$(CXX) $(CXXFLAGS) -MM $< -MT $(<:.cpp=.o) > $@


############# Main application #################
$(PROGRAM):	$(OBJECTS) $(DEPS)
	$(CXX) -o $@ $(OBJECTS) $(LIBSFLAGS)

################ Dependencies ##################
ifneq ($(MAKECMDGOALS),clean)
include $(DEPS)
endif

################### Clean ######################
clean:
	find . -name '*~' -delete
	find . -name '*.gcda' -delete
	find . -name '*.gcno' -delete
	-rm -rf $(PROGRAM) $(OBJECTS) $(DEPS) coverage.info coverage/

install:
	strip -s $(PROGRAM)
