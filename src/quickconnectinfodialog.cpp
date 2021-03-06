// quickconnectinfodialog.cpp --- 
// 
// Author: liuguangzhao
// Copyright (C) 2007-2010 liuguangzhao@users.sf.net
// URL: http://www.qtchina.net http://nullget.sourceforge.net
// Created: 2008-08-08 11:28:59 +0800
// Version: $Id: quickconnectinfodialog.cpp 475 2009-09-05 14:58:13Z liuguangzhao $
// 

#include  <QtGui>

#include "utils.h"
#include "ssh_info.h"

#include "ui_quickconnectinfodialog.h"
#include "quickconnectinfodialog.h"

QuickConnectInfoDialog::QuickConnectInfoDialog(QWidget *parent, Qt::WindowFlags f)
  : QDialog(parent, f)
  , uiw(new Ui::QuickConnectInfoDialog())
{
    this->uiw->setupUi(this);

    QObject::connect(this->uiw->checkBox_3, SIGNAL(stateChanged(int)),
                     this, SLOT(slot_pubkey_checked(int)));
    QObject::connect(this->uiw->toolButton, SIGNAL(clicked()),
                     this, SLOT(slot_select_pubkey()));
    QObject::connect(this->uiw->comboBox, SIGNAL(currentIndexChanged(int)),
                     this, SLOT(slot_protocol_changed(int)));

    this->pubkey_path = QString::null;
}


QuickConnectInfoDialog::~QuickConnectInfoDialog()
{
}

QString QuickConnectInfoDialog::get_protocol()
{
    QString protocol;
    switch (this->uiw->comboBox->currentIndex()) {
    case 0:
        protocol = "SFTP";
        break;
    case 1:
        protocol = "FTP";
        break;
    case 2:
        protocol = "FTPES";
        break;
    case 3:
        protocol = "FTPS";
        break;
    default:
        protocol = "UNKNOWN";
        Q_ASSERT(1 == 2);
        break;
    }
    return protocol;
    return this->uiw->comboBox->currentText().trimmed();
}
QString QuickConnectInfoDialog::get_user_name ()
{
    return this->uiw->lineEdit_3->text().trimmed();
}

QString QuickConnectInfoDialog::get_host_name ()
{
    return this->uiw->lineEdit->text().trimmed();
}

QString QuickConnectInfoDialog::get_password()
{
    QString passwd = this->uiw->lineEdit_4->text();
    passwd = QUrl::toPercentEncoding(passwd);
    return passwd;
}
short  QuickConnectInfoDialog::get_port()
{
    QString str_port = this->uiw->lineEdit_2->text().trimmed();
    int port = str_port.toShort();
    
    return port;
}
QString QuickConnectInfoDialog::get_pubkey()
{
    return this->pubkey_path;
}

void QuickConnectInfoDialog::set_active_host(QMap<QString,QString> host)
{
    QString show_name = host["show_name"];
    QString protocol = host["protocol"];
    QString host_name = host["host_name"];
    QString user_name = host["user_name"];
    QString password = host["password"];
    QString port = host["port"];
    QString pubkey_path ;//
    if (host.contains("pubkey")) {
        pubkey_path = host["pubkey"];
    }

    if (protocol == QString("FTPS")) {
        this->uiw->comboBox->setCurrentIndex(3);
    } else if (protocol == QString("FTPES")) {
        this->uiw->comboBox->setCurrentIndex(2);
    } else if (protocol == QString("FTP")) {
        this->uiw->comboBox->setCurrentIndex(1);
    } else if (protocol == QString("SFTP")) {
        this->uiw->comboBox->setCurrentIndex(0);
    } else {
        Q_ASSERT(1 == 2);
        // this->uiw->comboBox->setCurrentIndex(0);
    }

    this->uiw->lineEdit->setText(host_name);
    this->uiw->lineEdit_2->setText(port);
    this->uiw->lineEdit_3->setText(user_name);
    
    this->uiw->lineEdit_4->setFocus();
    this->uiw->lineEdit_4->setText(QUrl::fromPercentEncoding(password.toAscii()));
    
    this->uiw->groupBox->setTitle(QString(tr("Host Infomation: %1")).arg(show_name));
    this->show_name = show_name;
    this->pubkey_path = pubkey_path;

    if (host.contains("pubkey")) {
        this->uiw->toolButton->setToolTip(QString(tr("Current key: ")) 
                                          + this->pubkey_path);
    } else {
        this->uiw->toolButton->setToolTip(tr("No key supplied."));
    }
}

QMap<QString,QString> QuickConnectInfoDialog::get_host_map()
{
    QMap<QString,QString> host;

    // switch (this->uiw->comboBox->currentIndex()) {
    // case 0:
    //     host["protocol"] = "SFTP";
    //     break;
    // case 1:
    //     host["protocol"] = "FTP";
    //     break;
    // case 2:
    //     host["protocol"] = "FTPES";
    //     break;
    // case 3:
    //     host["protocol"] = "FTPS";
    //     break;
    // default:
    //     Q_ASSERT(1 == 2);
    //     break;
    // }
    host["show_name"] = this->show_name;
    host["protocol"] = this->get_protocol();
    host["host_name"] = this->uiw->lineEdit->text().trimmed();
    host["user_name"] = this->uiw->lineEdit_3->text().trimmed();
    host["password"] = this->uiw->lineEdit_4->text();
    host["password"] = QUrl::toPercentEncoding(host["password"]);
    host["port"] = this->uiw->lineEdit_2->text().trimmed();

    if (this->pubkey_path != QString::null && this->pubkey_path.length() > 0) {
        host["pubkey"] = this->pubkey_path;
    }

  return host;
}

void QuickConnectInfoDialog::slot_pubkey_checked(int state)
{
    if (state == Qt::Checked || state == Qt::PartiallyChecked) {
        this->uiw->toolButton->setEnabled(true);
    } else {
        this->uiw->toolButton->setEnabled(false);
    }
}

void QuickConnectInfoDialog::slot_select_pubkey()
{
    QString path = QString::null;
    QString init_dir;

#ifdef WIN32
    init_dir = ".";
#else
    init_dir = QDir::homePath();
#endif

    path = QFileDialog::getOpenFileName(this, tr("Publickey Select Dialog"), init_dir,
                                        tr("Public Key Files (*.pub);;All Files (*)"));
    qDebug()<<path;    
    if (path == QString::null) {
        //cancel
    } else if(path.length() == 0) {
        qDebug()<<"select null";
    } else {
        //TODO 检查文件格式, 
        bool is_priv_key = libssh2_publickey_is_privatekey(path.toAscii().data(), NULL) < 0 ? false : true;
        //TODO 简单检查是否是一个有效的public key 
        //TODO 检查key中的用户名及主机是否与上面输入的一致
        //检查对应的私钥文件;
        if (is_priv_key == false) {
            QString privkey = path.left(path.length() - 4);
            if (!QFile::exists(privkey)) {
                QMessageBox::warning(this, tr("Warning"), tr("Can not find related private key file."));
            }
        }
        this->pubkey_path = path;
        this->uiw->toolButton->setToolTip(QString(tr("Current key: ")) 
                                          + this->pubkey_path);
    }
}

void QuickConnectInfoDialog::slot_protocol_changed(int index)
{
    switch (index) {
    case 0:
        this->uiw->lineEdit_2->setText("22");
        break;
    case 1:
        this->uiw->lineEdit_2->setText("21");
        break;
    case 2:
        this->uiw->lineEdit_2->setText("21");
        break;
    case 3:
        // ctrl 990, data 989
        this->uiw->lineEdit_2->setText("990");
        break;
    default:
        qDebug()<<"Unkown protocl:"<<index;
        break;
    }
}


