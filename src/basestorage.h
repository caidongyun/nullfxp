// basestorage.h --- 
// 
// Author: liuguangzhao
// Copyright (C) 2007-2010 liuguangzhao@users.sf.net
// URL: http://www.qtchina.net http://nullget.sourceforge.net
// Created: 2008-04-04 14:46:49 +0000
// Version: $Id$
// 

#include <QtCore>
#include <QDataStream>

class BaseStorage
{
public:
    static BaseStorage *instance();
    ~BaseStorage();

    // host session method
    bool addHost(const QMap<QString, QString> &host, const QString &catPath = QString::null);
    bool removeHost(QString show_name, QString catPath = QString::null);
    bool updateHost(QMap<QString,QString> host, QString newName = QString::null, QString catPath = QString::null);
    bool clearHost(QString catPath = QString::null);

    bool containsHost(QString show_name, QString catPath = QString::null);
    
    QMap<QString, QMap<QString,QString> > getAllHost(const QString &catPath = QString::null);
    QMap<QString,QString> getHost(const QString &show_name, const QString &catPath = QString::null);
    QStringList getNameList(const QString &catPath = QString::null);

    int hostCount(QString catPath = QString::null);

    QString getSessionPath();
    QString getConfigPath();
    QString getForwardConfigFilePath();

    // forward settings
    bool addForwarder(const QMap<QString, QString> &fwd);
    const QMap<QString, QString> getForwarder(const QString &name);
    const QStringList getForwarderNames();

    // global options method
    bool saveOptions(QMap<QString, QMap<QString, QString> > options);
    bool saveOptions(QString section, QMap<QString, QString> options);
  
signals:
    void hostListChanged(QString catPath = QString::null);
    void hostLIstChanged(QString show_name, QString catPath = QString::null);

private:
    BaseStorage();

    static BaseStorage * mInstance;
    bool init();

    QString storagePath;
};


/* basestorage.h ends here */
