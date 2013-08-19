
BOOST_HOME      = /opt/boost
CPP             = g++ -std=c++11
CPP_INCLUDES    = -I$(SRC)/c++11 -I$(BOOST_HOME)/include
CPP_DEFINES     = -D_REENTRANT -DBOOST_NO_DEPRECATED -D_REENTRANT
CPP_FLAGS       = $(GLOBALCCOPTIONS) $(CPP_DEFINES) $(CPP_INCLUDES) -fPIC
LINKER          = ar cru 
SHLINKER        = g++ -shared -fPIC
LINKER_FLAGS    = -L$(BOOST_HOME)/lib
LINKER_ENTRY    = -lboost_system -lboost_chrono -lboost_exception -lboost_thread


libname=http_server
lib=$(libname).so

.SUFFIXES: .cc .class .java .cxx .C .cpp .o .c .l .y
objs=connection.o mime_types.o reply.o request_handler.o request_parser.o server.o

all: $(objs) http_server $(lib)
 
clean:
	-rm $(objs) http_server $(libname).so

.cpp.o: 
	$(CPP) -c $< $(CPP_FLAGS) $(CPP_DEFINES) $(CPP_INCLUDES)

$(lib): $(objs)
	$(SHLINKER) $(LINKER_FLAGS) $(LINKER_ENTRY) -o $(lib) $(objs)

http_server: asio_http.cpp $(objs)
	$(CPP) $< -o http_server $(CPP_FLAGS) $(CPP_DEFINES) $(CPP_INCLUDES) $(objs) $(LINKER_FLAGS) $(LINKER_ENTRY) 
