
EXTRA_PROGRAMS = aacNetPlayer
bin_PROGRAMS = opencore-aacdec $(NETPLAYER)

opencore_aacdec_SOURCES = decoder_aac.c wav.c wav.h
opencore_aacdec_LDADD = $(top_builddir)/src/libaacdec.la
opencore_aacdec_CFLAGS = -I$(top_srcdir)/include

aacNetPlayer_SOURCES = aacNetPlayer.c audio.c audio.h ringbuffer.c ringbuffer.h
aacNetPlayer_LDADD = $(top_builddir)/src/libaacdec.la $(CURL_LIBS)
aacNetPlayer_CFLAGS = -I$(top_srcdir)/include $(CURL_CFLAGS)
