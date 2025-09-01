#invoke make inside following directories and in this order: loader, launch, fib
#move the lib_simpleloader.so and launch binaries inside bin directory
#Provide the command for cleanup
all: bin/lib_simpleloader.so bin/launch bin/fib

bin/lib_simpleloader.so:
	$(MAKE) -C loader lib_simpleloader.so
	mkdir -p bin
	mv loader/lib_simpleloader.so bin/

bin/launch: bin/lib_simpleloader.so
	$(MAKE) -C launcher launch
	mkdir -p bin
	mv launcher/launch bin/

bin/fib: 
	$(MAKE) -C test fib

clean:
	$(MAKE) -C loader clean
	$(MAKE) -C launcher clean
	$(MAKE) -C test clean
	rm -rf bin
