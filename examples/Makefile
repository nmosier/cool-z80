%.s: %.cl
	../pa5/mycoolc $^

%.8xp: %.s z80_code/routines/*
	spasm -DBREAK="di \\ halt \\ ei" $*.s -I z80_code/routines $@

.PHONY: clean
clean:
	rm -f *.8xp *.s