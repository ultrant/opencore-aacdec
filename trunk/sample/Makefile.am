
bin_PROGRAMS = opencore-aacdec @NETPLAYER@
EXTRA_PROGRAMS = aacNetPlayer

opencore_aacdec_SOURCES = decoder_aac.c wav.c wav.h
opencore_aacdec_LDADD = $(top_builddir)/src/libaacdec.la
opencore_aacdec_CFLAGS = -I$(top_srcdir)/include

aacNetPlayer_SOURCES = aacNetPlayer.c wav.c wav.h
aacNetPlayer_LDADD = $(top_builddir)/src/libaacdec.la @NETPLAYER_LIBS@
aacNetPlayer_CFLAGS = -I$(top_srcdir)/include @NETPLAYER_CFLAGS@
