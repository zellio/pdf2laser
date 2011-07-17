epilog: cups-epilog.c
	gcc \
		-o $@ \
		`cups-config --cflags` \
		$< \
		`cups-config --libs` \


test-print: epilog test.ps
	DEVICE_URI='epilog://192.168.3.4/queue/rp/debug' \
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

