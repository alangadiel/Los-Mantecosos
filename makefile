all:
	-cd Consola/Debug && $(MAKE) all && cp Consola ../
	-cd Kernel/Debug && $(MAKE) all && cp Kernel ../
	-cd CPU/Debug && $(MAKE) all && cp CPU ../
	-cd FileSystem/Debug && $(MAKE) all && cp FileSystem ../
	-cd Memoria/Debug && $(MAKE) all && cp Memoria ../

clean:
	-cd Consola/Debug && $(MAKE) clean
	-cd Kernel/Debug && $(MAKE) clean
	-cd CPU/Debug && $(MAKE) clean
	-cd FileSystem/Debug && $(MAKE) clean
	-cd Memoria/Debug && $(MAKE) clean
