# libssh2.pro --- 
# 
# Author: liuguangzhao
# Copyright (C) 2007-2010 liuguangzhao@users.sf.net
# URL: http://www.qtchina.net http://nullget.sourceforge.net
# Created: 2009-05-18 22:03:33 +0800
# Version: $Id: libssh2.pro 455 2009-08-29 09:23:35Z liuguangzhao $
# 

TARGET = libssh.so

DESTDIR = .

SUBDIRS += libssh
TEMPLATE = subdirs

CONFIG += ordered

