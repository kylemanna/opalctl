
all: opalctl

opalctl: opalctl.c
	gcc opalctl.c -Wall -Wextra -O0 -o opalctl

.PHONY: all
