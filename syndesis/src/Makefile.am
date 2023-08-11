AM_CFLAGS = -Wall 
INCLUDES = @DEPS_CFLAGS@  -DPKGDATADIR=\"$(pkgdatadir)\" -DSYSCONFDIR=\"$(sysconfdir)\"

bin_PROGRAMS = syndownload synview synnet


syndownload_SOURCES = \
		      download.c \
		      common.c \
		      common.h


syndownload_LDADD = @DEPS_LIBS@ 

synview_SOURCES = \
		      view.c \
		      common.c \
		      common.h


synview_LDADD = @DEPS_LIBS@ 

synnet_SOURCES = \
		      netmonitor.c \
		      integrator.c \
		      integrator.h \
		      common.c \
		      common.h


synnet_LDADD = @DEPS_LIBS@ 
