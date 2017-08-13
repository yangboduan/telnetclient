CPP      = g++
CC       = gcc
CFLAGS   = -std=gnu++0x
OBJ      = main.o telnetclient.o
LINKOBJ  = main.o telnetclient.o
BIN      = telnetclient
RM       = rm -rf
LIB	 = 
$(BIN): $(OBJ)
	$(CPP) $(LINKOBJ) -o $(BIN)   
clean: 
	${RM} $(OBJ) $(BIN)

cleanobj:
	${RM} *.o
