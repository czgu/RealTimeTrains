LIB_SRC = src
CODE_DIR = term

all: lib_make copy code_make copy_ftp

.PHONY:
	lib_code

lib_make:
	$(MAKE) -C $(LIB_SRC)

code_make:
	$(MAKE) -C $(CODE_DIR)

copy:
	cp -f $(LIB_SRC)/io.a lib/libio.a
	cp -f $(LIB_SRC)/structs.a lib/libstructs.a
	cp -f $(LIB_SRC)/string.a lib/libstring.a

copy_ftp:
	cp $(CODE_DIR)/a0.elf /u/cs452/tftp/ARM/czgu/a0.elf

clean:
	$(MAKE) -C $(LIB_SRC) clean
	$(MAKE) -C $(CODE_DIR) clean
