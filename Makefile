all:
	@gcc send.c logs.c utils.c main.c
	@ctags -R *
	@echo "-- OK! --"
