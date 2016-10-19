LOCAL_PATH := $(call my-dir) 
HOME_PATH := $(call my-dir)

## Lib GMP
include $(CLEAR_VARS) 
LOCAL_MODULE := libgmp 
LOCAL_SRC_FILES := lib/libgmp.a 
include $(PREBUILT_STATIC_LIBRARY) 

## Lib PBC
include $(CLEAR_VARS) 
LOCAL_MODULE := libpbc 
LOCAL_SRC_FILES := lib/libpbc.a 
LOCAL_STATIC_LIBRARIES := libgmp 
include $(PREBUILT_STATIC_LIBRARY) 

# OpenSSL
include $(CLEAR_VARS) 
LOCAL_MODULE := openssl
LOCAL_SRC_FILES := lib/libcrypto.a
include $(PREBUILT_STATIC_LIBRARY) 




include $(CLEAR_VARS) 
GLIB_TOP := $(LOCAL_PATH)

GLIB_BUILD_STATIC := $(BUILD_STATIC)
$(info $(GLIB_BUILD_STATIC))

GLIB_C_INCLUDES :=			\
	$(GLIB_TOP)			\
	$(GLIB_TOP)/glib		\
	$(GLIB_TOP)/android

GLIB_SHARED_LIBRARIES :=		\
	libglib-2.0

GLIB_STATIC_LIBRARIES :=		\
	$(GLIB_SHARED_LIBRARIES)	\
	libpcre

include $(CLEAR_VARS)


include $(GLIB_TOP)/glib/Android.mk
TARGET_FORMAT_STRING_CFLAGS := -Wformat -Wno-error
#include $(BUILD_STATIC_LIBRARY)


#    LIBBSWABE

# Re-establish local path
LOCAL_PATH := $(HOME_PATH)

include $(CLEAR_VARS) 

# Include glib files
LOCAL_C_INCLUDES := 			\
	$(GLIB_TOP)			\
	$(GLIB_TOP)/android		\
	$(GLIB_TOP)/android-internal	\

LOCAL_MODULE    := libbswabe

LOCAL_SRC_FILES := \
	libbswabe/core.c \
	libbswabe/misc.c

LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog 
LOCAL_SHARED_LIBRARIES := libglib-2.0
LOCAL_WHOLE_STATIC_LIBRARIES:= openssl libpbc
include $(BUILD_SHARED_LIBRARY) 


#    MAIN - CPABE


include $(CLEAR_VARS) 

LOCAL_C_INCLUDES := 			\
	$(GLIB_TOP)			\
	$(GLIB_TOP)/android		\
	$(GLIB_TOP)/android-internal	\

LOCAL_MODULE    := cpabe
#LOCAL_EXPORT_LDLIBS := -llog
LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog -lrt
LOCAL_SRC_FILES := \
	cpabe/common.c \
	cpabe/cpabe.c \
	cpabe/policy_lang.c
	
LOCAL_SHARED_LIBRARIES := libbswabe libglib-2.0 #android-ndk-profiler
#LOCAL_CFLAGS := -pg


LOCAL_LDLIBS := -llog
include $(BUILD_SHARED_LIBRARY) 

#
# #################################################################### KP-ABE #########################################################################
#


#    LIBCELIA

# Re-establish local path
LOCAL_PATH := $(HOME_PATH)

include $(CLEAR_VARS) 

# Include glib files
LOCAL_C_INCLUDES := 			\
	$(GLIB_TOP)			\
	$(GLIB_TOP)/android		\
	$(GLIB_TOP)/android-internal	\

LOCAL_MODULE    := libcelia
LOCAL_SRC_FILES := \
	libcelia/core.c \
	libcelia/misc.c
LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog 	
LOCAL_SHARED_LIBRARIES := libglib-2.0
LOCAL_WHOLE_STATIC_LIBRARIES:= openssl libpbc
include $(BUILD_SHARED_LIBRARY) 


# MAIN - KPABE


include $(CLEAR_VARS) 

LOCAL_C_INCLUDES := 			\
	$(GLIB_TOP)			\
	$(GLIB_TOP)/android		\
	$(GLIB_TOP)/android-internal	\

LOCAL_MODULE    := kpabe
LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog -lrt
LOCAL_SRC_FILES := \
	kpabe/common.c \
	kpabe/kpabe.c \
	kpabe/policy_lang.c
	
LOCAL_SHARED_LIBRARIES := libcelia libglib-2.0

LOCAL_LDLIBS := -llog 
include $(BUILD_SHARED_LIBRARY) 

#$(call import-module,android-ndk-profiler)

