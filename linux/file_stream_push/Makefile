#/bin/sh

TOP_PATH = $(shell pwd)

SRCPTH = $(TOP_PATH)
INPTH = $(TOP_PATH)

CXX  = g++ 
INLPTH = -g -Wall -O3
INLPTH += -I$(TOP_PATH)  \
		  -I$(TOP_PATH)/../include
	   
LPTH := -L$(TOP_PATH)/../lib/ -lavcodec  -lavformat -lswscale -lavutil -lavdevice -lavfilter \
							  -lswresample  -lm -lz -lpthread  
							  
OBJ = $(SDK_PATH)obj  

TARG = file_push_ff
DIRS = $(SRCPTH)

FILES   = $(foreach dir, $(DIRS),  $(wildcard $(dir)/*.cpp))
OBJS 	= $(patsubst %.cpp, %.o, $(FILES))

all:$(TARG)

#链接
$(TARG):$(OBJS)
	$(CXX) -o $(TARG) $^ $(LPTH)
	
#编译
.cpp.o:
	$(CXX) -c $(INLPTH)  $<  -o $@    
	
clean:
	rm -rf $(OBJS) 
	rm -rf $(TARG)
