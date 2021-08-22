
AM_CXXFLAGS = \
	-fpermissive \
	-Wno-write-strings

AM_CPPFLAGS = \
	$(all_includes) \
	-I$(srcdir)/../../Include/Public \
	-I$(srcdir)/../../Include/Private  \
	-I$(srcdir)/../../Include/Linux \
	-I$(srcdir)/../../Include

VPATH += ../../Common

