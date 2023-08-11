INCLUDES = @DEPS_CFLAGS@ -DPKGDATADIR=\"$(pkgdatadir)\"

bin_PROGRAMS = elevate 

elevate_SOURCES = \
		      elevate.c \
		      gfx.c \
		      gfx.h \
		      files.c \
		      files.h \
		      app.c \
		      app.h \
		      modapp.c \
		      modapp.h \
		      engine.c \
		      engine.h \
		      parser.c \
		      parser.h \
		      lang.c \
		      lang.h \
		      integrator.c \
		      integrator.h \
		      mod_strings.h 

elevate_LDADD = @DEPS_LIBS@ 
