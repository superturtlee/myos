all: extractbootfs mkbootfs

extractbootfs:
	gcc -o extractbootfs extractbootfs.c
mkbootfs: extractbootfs
	gcc -o mkbootfs mkbootfs.c

clean:
	rm -f extractbootfs mkbootfs