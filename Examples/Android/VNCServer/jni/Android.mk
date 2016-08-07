LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	fbvncserver.c \
	LibVNCServer-0.9.7/libvncserver/main.c \
	LibVNCServer-0.9.7/libvncserver/rfbserver.c \
	LibVNCServer-0.9.7/libvncserver/rfbregion.c \
	LibVNCServer-0.9.7/libvncserver/auth.c \
	LibVNCServer-0.9.7/libvncserver/sockets.c \
	LibVNCServer-0.9.7/libvncserver/stats.c \
	LibVNCServer-0.9.7/libvncserver/corre.c \
	LibVNCServer-0.9.7/libvncserver/hextile.c \
	LibVNCServer-0.9.7/libvncserver/rre.c \
	LibVNCServer-0.9.7/libvncserver/translate.c \
	LibVNCServer-0.9.7/libvncserver/cutpaste.c \
	LibVNCServer-0.9.7/libvncserver/httpd.c \
	LibVNCServer-0.9.7/libvncserver/cursor.c \
	LibVNCServer-0.9.7/libvncserver/font.c \
	LibVNCServer-0.9.7/libvncserver/draw.c \
	LibVNCServer-0.9.7/libvncserver/selbox.c \
	LibVNCServer-0.9.7/libvncserver/d3des.c \
	LibVNCServer-0.9.7/libvncserver/vncauth.c \
	LibVNCServer-0.9.7/libvncserver/cargs.c \
	LibVNCServer-0.9.7/libvncserver/minilzo.c \
	LibVNCServer-0.9.7/libvncserver/ultra.c \
	LibVNCServer-0.9.7/libvncserver/scale.c \
	LibVNCServer-0.9.7/libvncserver/zlib.c \
	LibVNCServer-0.9.7/libvncserver/zrle.c \
	LibVNCServer-0.9.7/libvncserver/zrleoutstream.c \
	LibVNCServer-0.9.7/libvncserver/zrlepalettehelper.c \
	LibVNCServer-0.9.7/libvncserver/zywrletemplate.c \
	LibVNCServer-0.9.7/libvncserver/tight.c \
	jpeg8d/jcapimin.c \
	jpeg8d/jcapistd.c \
	jpeg8d/jccoefct.c \
	jpeg8d/jccolor.c \
	jpeg8d/jcdctmgr.c \
	jpeg8d/jchuff.c \
	jpeg8d/jcinit.c \
	jpeg8d/jcmainct.c \
	jpeg8d/jcmarker.c \
	jpeg8d/jcmaster.c \
	jpeg8d/jcomapi.c \
	jpeg8d/jcparam.c \
	jpeg8d/jcprepct.c \
	jpeg8d/jcsample.c \
	jpeg8d/jctrans.c \
	jpeg8d/jdapimin.c \
	jpeg8d/jdapistd.c \
	jpeg8d/jdatadst.c \
	jpeg8d/jdatasrc.c \
	jpeg8d/jdcoefct.c \
	jpeg8d/jdcolor.c \
	jpeg8d/jddctmgr.c \
	jpeg8d/jdhuff.c \
	jpeg8d/jdinput.c \
	jpeg8d/jdmainct.c \
	jpeg8d/jdmarker.c \
	jpeg8d/jdmaster.c \
	jpeg8d/jdmerge.c \
	jpeg8d/jdpostct.c \
	jpeg8d/jdsample.c \
	jpeg8d/jdtrans.c \
	jpeg8d/jerror.c \
	jpeg8d/jfdctflt.c \
	jpeg8d/jfdctfst.c \
	jpeg8d/jfdctint.c \
	jpeg8d/jidctflt.c \
	jpeg8d/jidctfst.c \
	jpeg8d/jidctint.c \
	jpeg8d/jquant1.c \
	jpeg8d/jquant2.c \
	jpeg8d/jutils.c \
	jpeg8d/jmemmgr.c \
	jpeg8d/jcarith.c \
	jpeg8d/jdarith.c \
	jpeg8d/jaricom.c \
	jpeg8d/jmemnobs.c \
	zlib/adler32.c \
	zlib/crc32.c \
	zlib/deflate.c \
	zlib/infback.c \
	zlib/inffast.c \
	zlib/inflate.c \
	zlib/inftrees.c \
	zlib/trees.c \
	zlib/zutil.c \
	zlib/compress.c \
	zlib/uncompr.c \
	zlib/gzclose.c \
	zlib/gzlib.c \
	zlib/gzread.c \
	zlib/gzwrite.c



LOCAL_C_INCLUDES := \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/LibVNCServer-0.9.7/libvncserver \
	$(LOCAL_PATH)/LibVNCServer-0.9.7 \
	$(LOCAL_PATH)/jpeg8d \
	$(LOCAL_PATH)/zlib 


LOCAL_MODULE:= androidvncserver

include $(BUILD_EXECUTABLE)
