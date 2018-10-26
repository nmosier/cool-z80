%.s: %.cl
	../pa5/mycoolc $^

%.8xp: %.s z80_code/routines/*
	spasm -DBREAK="di \\ halt \\ ei" $*.s -I z80_code/routines $@

.PHONY: %
%: %.8xp
	# done

.PHONY: clean
clean:
	rm *.8xp *.s