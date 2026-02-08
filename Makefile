build:
	make clean
	gcc -o ckpt ckpt.c
	gcc -o counter counter.c
	gcc -o readckpt readckpt.c
	gcc -static -Wl,-Ttext-segment=5000000 -Wl,-Tdata=5100000 -Wl,-Tbss=5200000 -g3 -o restart restart.c
	gcc -shared -fPIC -Wno-implicit-function-declaration -o libckpt.so libckpt.c

check:
	rm -f myckpt.img
	./ckpt ./counter &
	sleep 5
	echo Checkpointing...
	kill -12 `pgrep -n counter`
	sleep 3
	echo Restarting...
	./restart myckpt.img

clean:
	rm -f ckpt counter libckpt.so readckpt restart myckpt.img

dist:
	make clean
	tar -czvf hw2-submission.tar.gz ckpt.c counter.c libckpt.c readckpt.c restart.c README.md Makefile
