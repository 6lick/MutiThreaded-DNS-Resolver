DNS: multi-lookup.c util.c
	gcc multi-lookup.c -o multi-lookup util.c -pthread -g -w
clean:
	rm *.txt multi-lookup