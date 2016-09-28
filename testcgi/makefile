all:
	gcc testcgi.c -L. -lcgic -o testcgi
	gcc basecgi.c ipc.c parson.c -L. -lcgic -o basecgi
	gcc netcgi.c ipc.c parson.c -L. -lcgic -o netcgi
	gcc appcgi.c ipc.c parson.c -L. -lcgic -o appcgi
	gcc devcgi.c ipc.c parson.c -L. -lcgic -o devcgi
	gcc uploadcgi.c -L. -lcgic -o uploadcgi
	cp *cgi /var/www/cgi-bin/
clean:
	rm *cgi
