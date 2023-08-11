INCLUDES = @DEPS_CFLAGS@  -DPKGDATADIR=\"$(pkgdatadir)\" -DSYSCONFDIR=\"$(sysconfdir)\"

bin_PROGRAMS = vault

vault_SOURCES = \
		      vault.c \
		      vault.h \
		      db.c \
		      db.h \
		      logic.c \
		      logic.h \
		      schema.h


vault_LDADD = @DEPS_LIBS@ 
