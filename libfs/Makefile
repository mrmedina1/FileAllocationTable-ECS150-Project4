# Target library
lib := libfs.a

all: $(lib)

objs := fs.o disk.o

CFLAGS := -Wall -Werror

$(lib): $(objs)
	ar rcs $(lib) $(objs) 

fs.o: fs.c fs.h
	gcc $(CFLAGS) -c -o fs.o fs.c

disk.o: disk.c disk.h
	gcc $(CFLAGS) -c -o disk.o disk.c

clean:
	rm -rf $(objs) $(lib)

## TODO
