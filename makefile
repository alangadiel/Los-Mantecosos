all:
	-sudo chmod -R 777 /mnt
	-cd Consola/bin && $(MAKE) all && cp Consola ../
	-cd Kernel/bin && $(MAKE) all && cp Kernel ../
	-cd CPU/bin && $(MAKE) all && cp CPU ../
	-cd FileSystem/bin && $(MAKE) all && cp FileSystem ../
	-cd Memoria/bin && $(MAKE) all && cp Memoria ../

clean:
	-cd Consola/bin && $(MAKE) clean
	-cd Kernel/bin && $(MAKE) clean
	-cd CPU/bin && $(MAKE) clean
	-cd FileSystem/bin && $(MAKE) clean
	-cd Memoria/bin && $(MAKE) clean
