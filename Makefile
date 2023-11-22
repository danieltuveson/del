FLAGS = -g -Weverything -Wno-padded -Wno-poison-system-directories
objects = main.o

bytecode : $(objects)
	cc $(FLAGS) -o bytecode $(objects)

clean :
	rm bytecode $(objects)

