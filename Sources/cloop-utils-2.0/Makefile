#!/usr/bin/make

CFLAGS:=-Wall -Wstrict-prototypes -Wno-trigraphs -O2 -s -I. -fno-strict-aliasing -fno-common -fomit-frame-pointer 

PROGRAMS = create_compressed_fs extract_compressed_fs cloop_suspend

utils: $(PROGRAMS)

create_compressed_fs: advancecomp-1.15/advfs
	ln -f $< $@

advancecomp-1.15/advfs:
	( cd advancecomp-1.15 ; ./configure && $(MAKE) advfs )

extract_compressed_fs: extract_compressed_fs.c
	$(CC) -Wall -O2 -s -o $@ $< -lz

cloop_suspend: cloop_suspend.c
	$(CC) -static -Wall -O2 -s -o $@ $<

install:
	mkdir -p "$(DESTDIR)/usr/bin"
	install $(PROGRAMS) "$(DESTDIR)/usr/bin/"

clean:
	rm -rf create_compressed_fs extract_compressed_fs cloop_suspend *.o *.ko Module.symvers .cloop* .compressed_loop.* .tmp*
	[ -f advancecomp-1.15/Makefile ] && $(MAKE) -C advancecomp-1.15 distclean || true
