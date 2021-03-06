# src.pro --- 
# 
# Author: liuguangzhao
# Copyright (C) 2007-2010 liuguangzhao@users.sf.net
# URL: http://www.qtchina.net http://nullget.sourceforge.net
# Created: 2009-05-18 22:03:43 +0800
# Version: $Id: src.pro 613 2010-04-14 04:11:45Z liuguangzhao $
# 

DESTDIR = .
TEMPLATE = lib
TARGET = kssh

#CONFIG += dll console #staticlib
CONFIG +=  console staticlib
CONFIG -= qt 
OBJECTS_DIR = obj

VERSION = 0.5.1

win32 {
	CONFIG += release
} else {
	CONFIG += debug release
}

DEFINES += HAVE_CONFIG_H

SOURCES +=   agent.c \
  auth.c \
  auth1.c \
  base64.c \
  bind.c \
  buffer.c \
  callbacks.c \
  channels.c \
  channels1.c \
  client.c \
  config.c \
  connect.c \
  crc32.c \
  crypt.c \
  dh.c \
  error.c \
  getpass.c \
  gcrypt_missing.c \
  gzip.c \
  init.c \
  kex.c \
  keyfiles.c \
  keys.c \
  known_hosts.c \
  legacy.c \
  libcrypto.c \
  libgcrypt.c \
  log.c \
  match.c \
  messages.c \
  misc.c \
  options.c \
  packet.c \
  packet1.c \
  pcap.c \
  pki.c \
  poll.c \
  session.c \
  scp.c \
  sftp.c \
  server.c \
  sftpserver.c \
  socket.c \
  string.c \
  threads.c \
  wrapper.c

HEADERS += 

win32 {
	#SOURCES += libgcrypt.c
	#SOURCES += openssl.c	
} else {
	#SOURCES += openssl.c
    SOURCES +=   threads/pthread.c
}


# controll if show libssh debug message, using this line
# DEFINES += LIBSSH2DEBUG=1 
DEFINES += WITH_SSH1 

win32 {
    !win32-g++ {
        DEFINES += LIBSSH_WIN32 LIBSSH_STATIC _CRT_SECURE_NO_DEPRECATE 
        ## check cl.exe, x64 or x86
        CLARCH=$$system(path)
        VAMD64=$$find(CLARCH,amd64)
        isEmpty(VAMD64) {
            INCLUDEPATH += Z:/librarys/vc-ssl-x86/include Z:/librarys/vc-zlib/include
             QMAKE_LIBDIR += Z:/librarys/vc-ssl-x86/lib Z:/librarys/vc-zlib/static32
        } else {
            INCLUDEPATH += Z:/librarys/vc-ssl-x64/include Z:/librarys/vc-zlib/include
             QMAKE_LIBDIR += Z:/librarys/vc-ssl-x64/lib Z:/librarys/vc-zlib/staticx64
        }
    }
    LIBS += -lzlibstat -llibeay32 -lssleay32 -ladvapi32 -luser32 -lws2_32 -lgdi32 -lshell32
} else {
     LIBS += -lssl -lcrypto -lz
}

macx-g++ {
    #QMAKE_CFLAGS_DEBUG += -arch i386
    #QMAKE_CFLAGS_RELEASE += -arch i386
    #QMAKE_CXXFLAGS_DEBUG += -arch i386
    #QMAKE_CXXFLAGS_RELEASE += -arch i386
    DEFINES += __DARWIN__ __APPLE__
}

QMAKE_CFLAGS_DEBUG += -g
QMAKE_CFLAGS_RELEASE += -g
QMAKE_CXXFLAGS_DEBUG += -g
QMAKE_CXXFLAGS_RELEASE += -g

INCLUDEPATH += ../include/ .

