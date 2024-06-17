

PHONY := __all
__all:
	MAKEFLAGS="$(MAKEFLAGS)" $(MAKE) -C src

%:
	MAKEFLAGS="$(MAKEFLAGS)" $(MAKE) -C src $@

