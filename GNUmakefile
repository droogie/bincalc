.DEFAULT_GOAL := all

QMAKE_MAKEFILE := Makefile

.PHONY: all clean distclean tests

$(QMAKE_MAKEFILE):
	qmake bincalc.pro -o $(QMAKE_MAKEFILE)

all: $(QMAKE_MAKEFILE)
	$(MAKE) -f $(QMAKE_MAKEFILE)

tests:
	qmake tests.pro -o Makefile.tests
	$(MAKE) -f Makefile.tests

clean:
	@if [ -f $(QMAKE_MAKEFILE) ]; then $(MAKE) -f $(QMAKE_MAKEFILE) clean; fi
	./scripts/clean.sh

distclean: clean
	rm -f $(QMAKE_MAKEFILE) Makefile.tests
