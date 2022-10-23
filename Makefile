default:
	rm src.tar
	cp conf.h contrib include input.c layers.c LICENSE main.c Makefile meson.build output.c protocols README.md view.c -r src/
	tar cvf src.tar src/
	nix build -f default.nix
