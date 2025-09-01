#Create lib_simpleloader.so from loader.c

#Provide the command for cleanup

all: lib_simpleloader.so

lib_simpleloader.so: loader.c
	gcc -m32 -shared -fPIC -o lib_simpleloader.so loader.c

clean:
	rm -f lib_simpleloader.so

