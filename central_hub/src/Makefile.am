INCLUDES = @DEPS_CFLAGS@  -DPKGDATADIR=\"$(pkgdatadir)\" -DSYSCONFDIR=\"$(sysconfdir)\"

bin_PROGRAMS = central_hub

central_hub_SOURCES = \
		      central_hub.c \
		      sys.c \
		      sys.h


central_hub_LDADD = @DEPS_LIBS@


