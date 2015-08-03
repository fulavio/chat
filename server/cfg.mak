SRC=src
INC=include
OBJ=obj
DIST=bin
EXE=sv.exe
SRCS=$(wildcard $(SRC)/*.c)
OBJS=$(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SRCS))
GCCARGS=-Wall -lws2_32
