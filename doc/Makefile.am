man_MANS =
dist_man1_MANS = pdf2laser.1
dist_man_MANS = pdf2laser.preset.5
CLEANFILES = pdf2laser.preset.5
MAINTAINERCLEANFILES = Makefile.in

pdf2laser.preset.5: pdf2laser.preset.man
	sed -e "s|@DATAROOTDIR@|@datarootdir@|g" -e "s|@SYSCONFDIR@|@sysconfdir@|g" <$< >$@
