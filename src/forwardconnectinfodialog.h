// forwardconnectinfodialog.h --- 
// 
// Author: liuguangzhao
// Copyright (C) 2007-2012 liuguangzhao@users.sf.net
// URL: http://www.qtchina.net http://nullget.sourceforge.net
// Created: 2007-08-29 16:40:49 +0800
// Version: $Id$
// 

#ifndef FORWARDCONNECTINFODIALOG_H
#define FORWARDCONNECTINFODIALOG_H

#include <QtCore>
#include <QtGui>

namespace Ui {
    class ForwardConnectInfoDialog;
};

/**
	@author liuguangzhao <liuguangzhao@users.sf.net>
*/
class ForwardConnectInfoDialog : public QDialog
{
    Q_OBJECT;
public:
    ForwardConnectInfoDialog(QWidget *parent = 0);
    virtual ~ForwardConnectInfoDialog();
    
    void get_forward_info(QString &host, QString &user_name, QString &passwd, 
                          QString &listen_port, QString &local_port);
    
private:
    Ui::ForwardConnectInfoDialog *uiw;
};

#endif
