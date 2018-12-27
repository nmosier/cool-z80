SPASM_DIRECTIVES=-DBREAK="di \\ halt \\ ei"
SPASM_MODE=

%.s: %.cl
	../pa5/mycoolc $^

%.8xp: %.s z80_code/routines/*
	spasm $(SPASM_DIRECTIVES) $*.s -I z80_code/routines $@

%.8xk: %.s z80_code/routines/*
	spasm $(SPASM_MODE) $(SPASM_DIRECTIVES) $*.s -I z80_code/routines $@

.PHONY: clean
clean:
	rm -f *.8xp *.s *.8xk