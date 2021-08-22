
AM_CXXFLAGS = \
	-fpermissive

AM_CPPFLAGS = \
	$(all_includes) \
	-I$(srcdir)/../../../Include/Private \
	-I$(srcdir)/../../../Include/Public \
	-I$(srcdir)/../US_Include \
	-I$(srcdir)/../../../Include/Linux \
	-I$(srcdir)/../../../Include \
	-I$(srcdir)/../../Bd_Include

VPATH += ../../../Common
VPATH += ../US_Common


