#INSTALL = install

#BINFILES = unipihostname unipicheck


%:
	MAKEFLAGS="$(MAKEFLAGS)" $(MAKE) -C src $@

#install:
#	$(INSTALL) -D $(BINFILES:%=src/%) -t $(DESTDIR)/opt/unipi/tools


#clean:
#	cd src && make clean
