
extract_symtab_from_exec: extract_symtab_from_exec.c
	$(CC) -g2 -m64 -o $@ $^ -lproc -ldemangle

libelftest: libelftest.c
	$(CC) -g2 -m64 -o $@ $^ -lelf

grab_file_test: grab_file_test.c
	$(CC) -g2 -m64 -o $@ $^ -lproc

proctest: proctest.c
	$(CC) -g2 -m64 -o $@ $^ -lproc

gordon: gordon.c
	$(CC) -g2 -m64 -o $@ $^ -lproc
