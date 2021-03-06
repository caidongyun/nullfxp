/* taskpackage.h --- 
 * 
 * Author: liuguangzhao
 * Copyright (C) 2007-2010 liuguangzhao@users.sf.net
 * URL: http://www.qtchina.net http://nullget.sourceforge.net
 * Created: 2009-05-07 23:14:42 +0800
 * Version: $Id$
 */

#ifndef _TASKPACKAGE_H_
#define _TASKPACKAGE_H_
        
#include <QtCore>

enum { PROTO_MIN, 
       PROTO_FILE, 
       PROTO_SFTP, 
       PROTO_FTP,
       PROTO_FTPS, 
       PROTO_HTTP, 
       PROTO_HTTPS,
       PROTO_RSTP,
       PROTO_MMS , 
       PROTO_MAX 
} ;

/** 
 * 这个类存储的值都为unicode编码的值。
 */
class TaskPackage 
{
 public:
    TaskPackage();
    TaskPackage(int scheme);
    virtual ~TaskPackage();
    
    bool setProtocol(int proto);
    int setFile(QString file);
    static void dump(const TaskPackage &pkg);
    static bool isValid(const TaskPackage &kg);
    static QString getProtocolNameById(int protocol_id);
    QByteArray toRawData();
    static TaskPackage fromRawData(QByteArray ba);
    QMap<QString, QString> hostInfo();

    int scheme;
    QStringList files;
    QString host;
    QString port;
    QString username;
    QString password;
    QString pubkey;
};

#ifndef QT_NO_DEBUG_STREAM
Q_CORE_EXPORT QDebug operator<<(QDebug, const TaskPackage &);
#endif
        
#endif /* _TASKPACKAGE_H_ */
