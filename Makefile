#INSTALL = install

#BINFILES = unipihostname unipicheck

PHONY := __all
__all:
	MAKEFLAGS="$(MAKEFLAGS)" $(MAKE) -C src

%:
	MAKEFLAGS="$(MAKEFLAGS)" $(MAKE) -C src $@

#install:
#	$(INSTALL) -D $(BINFILES:%=src/%) -t $(DESTDIR)/opt/unipi/tools


#clean:
#	cd src && make clean
