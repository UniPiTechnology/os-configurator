INSTALL = install

BINFILES = unipihostname unipicheck

all:
	cd src; make

install:
	$(INSTALL) -D $(BINFILES:%=src/%) -t $(DESTDIR)/opt/unipi/tools


clean:
	cd src && make clean
