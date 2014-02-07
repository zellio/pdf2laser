epilog: epilog.c
	gcc \
		-std=c99 \
		-W \
		-Wall \
		-O3 \
		-o $@ \
		$< \
		-lm \

ta10: ta10.c
	gcc \
		-W \
		-Wall \
		-O3 \
		-o $@ \
		$< \

epilog2: epilog2.c
	gcc \
		-W \
		-Wall \
		-O3 \
		-o $@ \
		$< \
		-lm \

live-laser: live-laser.c
	gcc \
		-W \
		-Wall \
		-O3 \
		-m32 \
		-I/usr/local/include/opencv \
		-o $@ \
		$< \
		-L/usr/local/lib \
		-lhighgui \
		-lcvaux \
		-lcxcore \
		-lcv \

cups-epilog: cups-epilog.c
	gcc \
		-o $@ \
		`cups-config --cflags` \
		$< \
		`cups-config --libs` \


test-print: epilog test.ps
	DEVICE_URI='epilog://192.168.3.4/Legend/rp=100/rs=100/vp=100/vs=10/vf=5000/rm=mono/r=300/af=0/debug' \
	./epilog  \
		123 \
		thudson \
		test \
		1 \
		/debug \
		< test.ps

test-gs: \
	/opt/local/bin/gs \
		-q \
		-dBATCH \
		-dNOPAUSE \
		-r600 \
		-g5100x6600 \
		-sDEVICE=bmpmono \
		-sOutputFile=/tmp/epilog-2823.bmp \
		/tmp/epilog-2823.eps \
		> /tmp/epilog-2823.vector

INSTALL_LOCATION=/usr/libexec/cups/backend/epilog

install: epilog
	sudo cp $< $(INSTALL_LOCATION)
	sudo chown root:wheel $(INSTALL_LOCATION)

