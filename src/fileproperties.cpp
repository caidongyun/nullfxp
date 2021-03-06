// fileproperties.cpp --- 
// 
// Author: liuguangzhao
// Copyright (C) 2007-2010 liuguangzhao@users.sf.net
// URL: http://www.qtchina.net http://nullget.sourceforge.net
// Created: 2008-07-19 14:43:08 +0800
// Version: $Id$
// 

#include <errno.h>

#include "utils.h"
#include "sshfileinfo.h"
#include "ftpfileinfo.h"
#include "globaloption.h"
#include "sshdirretriver.h"
#include "fileproperties.h"

#ifndef _MSC_VER
#warning "wrapper lower class, drop this include"
#endif
#include "rfsdirnode.h"

extern QHash<QString, QString> gMimeHash;

#ifndef _MSC_VER
#warning "maybe there is some way to drop this thread"
#endif

#include "connection.h"
#include "libftp/libftp.h"
#include "ui_fileproperties.h"

static int QUrlInfo2LIBSSH2_SFTP_ATTRIBUTES(QUrlInfo &ui, LIBSSH2_SFTP_ATTRIBUTES *attr)
{
    assert(attr != NULL);
    int perm = ui.permissions();

    memset(attr, 0, sizeof(*attr));
    attr->filesize = ui.size();
    attr->atime = ui.lastRead().toTime_t();
    attr->mtime = ui.lastModified().toTime_t();

    if (ui.isFile() && !ui.isSymLink()) {
        attr->permissions |= LIBSSH2_SFTP_S_IFREG;
    } else if (ui.isSymLink()) {
        attr->permissions |= LIBSSH2_SFTP_S_IFLNK;
    } else if (ui.isDir()) {
        attr->permissions |= LIBSSH2_SFTP_S_IFDIR;
    } else {
        qDebug()<<"unknown file type:"<<ui.name();
    }

    if (perm & QUrlInfo::ReadOwner) {
        attr->permissions |= LIBSSH2_SFTP_S_IRUSR;
    }
    if (perm & QUrlInfo::WriteOwner) {
        attr->permissions |= LIBSSH2_SFTP_S_IWUSR;
    }
    if (perm & QUrlInfo::ExeOwner) {
        attr->permissions |= LIBSSH2_SFTP_S_IXUSR;
    }

    if (perm & QUrlInfo::ReadGroup) {
        attr->permissions |= LIBSSH2_SFTP_S_IRGRP;
    }
    if (perm & QUrlInfo::WriteGroup) {
        attr->permissions |= LIBSSH2_SFTP_S_IWGRP;
    }
    if (perm & QFile::ExeGroup) {
        attr->permissions |= LIBSSH2_SFTP_S_IXGRP;
    }

    if (perm & QUrlInfo::ReadOther) {
        attr->permissions |= LIBSSH2_SFTP_S_IROTH;
    }
    if (perm & QUrlInfo::WriteOther) {
        attr->permissions |= LIBSSH2_SFTP_S_IWOTH;
    }
    if (perm & QUrlInfo::ExeOther) {
        attr->permissions |= LIBSSH2_SFTP_S_IXOTH;
    }

    // TODO how got uid and gid. ftp not given it?
    return 0;
}

FilePropertiesRetriver::FilePropertiesRetriver(Connection *conn, LIBSSH2_SFTP *ssh2_sftp, 
                                               QString file_path, QObject *parent)
  : QThread(parent)
{
    this->ssh2_sftp = ssh2_sftp;
    this->file_path = GlobalOption::instance()->remote_codec->fromUnicode(file_path);
    this->conn = conn;
}
FilePropertiesRetriver::~FilePropertiesRetriver()
{
    q_debug()<<"";
}

void FilePropertiesRetriver::run()
{
    assert(this->conn != NULL);
    if (this->conn->protocolType() == Connection::PROTO_SFTP) {
        this->run_sftp();
    } else if (this->conn->protocolType() == Connection::PROTO_FTP) {
        this->run_ftp();
    } else if (this->conn->protocolType() == Connection::PROTO_FTPS) {
        this->run_ftp();
    } else if (this->conn->protocolType() == Connection::PROTO_FTPES) {
        this->run_ftp();
    } else {
        assert(0);
    }
}

void FilePropertiesRetriver::run_sftp()
{
    //qDebug() <<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;
    int rv = 0;
    LIBSSH2_SFTP_ATTRIBUTES *sftp_attrib = (LIBSSH2_SFTP_ATTRIBUTES*)malloc(sizeof(LIBSSH2_SFTP_ATTRIBUTES));

    memset(sftp_attrib, 0, sizeof(LIBSSH2_SFTP_ATTRIBUTES));

	rv = libssh2_sftp_stat(ssh2_sftp, file_path.toAscii().data(), sftp_attrib);
    if (rv != 0) {
        qDebug()<<this->file_path;
        qDebug()<<"sftp stat error:"<<libssh2_sftp_last_error(ssh2_sftp);
    }
    emit file_attr_abtained(this->file_path, sftp_attrib, QString());
    // QString u8_file_path = GlobalOption::instance()->remote_codec->toUnicode(this->file_path.toAscii());
    // emit file_attr_abtained(u8_file_path, sftp_attrib);
}
void FilePropertiesRetriver::run_ftp()
{
    //qDebug() <<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;
    int rv = 0;
    LIBSSH2_SFTP_ATTRIBUTES *sftp_attrib = (LIBSSH2_SFTP_ATTRIBUTES*)malloc(sizeof(LIBSSH2_SFTP_ATTRIBUTES));
    memset(sftp_attrib, 0, sizeof(LIBSSH2_SFTP_ATTRIBUTES));
    LibFtp *ftp = this->conn->ftp;
    //QVector<QUrlInfo> fileList;
    QVector<FTPFileInfo> fileList;
    QString link_to;

    rv = ftp->chdir(this->file_path); // so we can known if it is directory
    if (rv == 0) {
        // it is dir
        // rv = ftp->passive();
        // assert(rv == 0);
        // rv = ftp->connectDataChannel();
        // assert(rv == 0);
        // rv = ftp->lista(this->file_path);
        // assert(rv == 0);
        // rv = ftp->closeDataChannel();
        // fileList = ftp->getDirList();
        // // lista的结果有时候是该目录下的文件和子目录，
        // // 有时候是这个目录本身
        // if (fileList.count() == 1) {
        //     QUrlInfo ui = fileList.at(0);
        //     rv = QUrlInfo2LIBSSH2_SFTP_ATTRIBUTES(ui, sftp_attrib);
        // } else {
        //     for (int i = 0; i < fileList.count(); ++i) {
        //         if (fileList.at(i).name() == ".") {
        //             QUrlInfo ui = fileList.at(i);
        //             rv = QUrlInfo2LIBSSH2_SFTP_ATTRIBUTES(ui, sftp_attrib);
        //             break;
        //         }
        //     }
        // }
        rv = ftp->mlst(this->file_path);
        fileList = ftp->getDirList2();
        if (fileList.count() == 1) {
            QUrlInfo ui = fileList.at(0);
            rv = QUrlInfo2LIBSSH2_SFTP_ATTRIBUTES(ui, sftp_attrib);
        } else {
            for (int i = 0; i < fileList.count(); ++i) {
                if (fileList.at(i).name() == ".") {
                    QUrlInfo ui = fileList.at(i);
                    rv = QUrlInfo2LIBSSH2_SFTP_ATTRIBUTES(ui, sftp_attrib);
                    break;
                }
            }
        }
    } else {
        // rv = ftp->passive();
        // assert(rv == 0);
        // rv = ftp->connectDataChannel();
        // assert(rv == 0);
        // rv = ftp->list(this->file_path);
        // assert(rv == 0);
        // rv = ftp->closeDataChannel();
        // fileList = ftp->getDirList();
        // if (fileList.count() == 1) {
        //     QUrlInfo ui = fileList.at(0);
        //     rv = QUrlInfo2LIBSSH2_SFTP_ATTRIBUTES(ui, sftp_attrib);
        // } else {
        //     assert(0);
        // }

        // FIXME: if this->file_path is soft symbol link, fileList.count() == 0
        // because this symlink link to a directory which can not access for permission resion.
        // how fix???
        // 也可能不是一个文件，而是一个链接。
        rv = ftp->mlst(this->file_path);
        assert(rv == 0);
        fileList = ftp->getDirList2();
        q_debug()<<fileList.count();
        if (fileList.count() == 0) {
            // q_debug()<<"FIXME:"<<this->file_path;
            if (this->file_path == "/") {
                // not possible
                Q_ASSERT(1==2);
            }
            QString file_name = this->file_path.right(this->file_path.length()
                                                      -this->file_path.lastIndexOf("/")-1);
            QString parent_path = this->file_path.left(this->file_path.lastIndexOf("/"));
            // q_debug()<<file_name<<parent_path;
            rv = ftp->mlst(parent_path);
            assert(rv == 0);
            fileList = ftp->getDirList2();
            FTPFileInfo ui;
            for (int i = 0; i < fileList.count(); ++i) {
                ui = fileList.at(i);
                // q_debug()<<ui.name()<<file_name<<i;
                if (ui.name() == file_name) {
                    rv = QUrlInfo2LIBSSH2_SFTP_ATTRIBUTES(ui, sftp_attrib);
                    if (ui.isSymLink()) {
                        link_to = ui.symlinkTarget();
                    }
                    break;
                }
            }
        } else if (fileList.count() == 1) {
            QUrlInfo ui = fileList.at(0);
            rv = QUrlInfo2LIBSSH2_SFTP_ATTRIBUTES(ui, sftp_attrib);
        } else {
            assert(0);
        }
    }

    emit file_attr_abtained(this->file_path, sftp_attrib, link_to);
}

///////////////////////////////////////////////////
FileProperties::FileProperties(QWidget *parent)
    : QDialog(parent)
    , ui_file_prop_dialog(new Ui::FileProperties())
{
    this->ssh2_sftp = 0;
    this->conn = 0;

	this->ui_file_prop_dialog->setupUi(this);
    this->ui_file_prop_dialog->label_13->setPixmap(QPixmap(":/icons/nullget-1.png").scaledToHeight(50));
}

FileProperties::~FileProperties()
{
}
void FileProperties::set_ssh2_sftp(void *ssh2_sftp)
{
    this->ssh2_sftp = (LIBSSH2_SFTP*)ssh2_sftp;
}
void FileProperties::setConnection(Connection *conn)
{
    this->conn = conn;
}

void FileProperties::set_file_info_model_list(QModelIndexList &mil)
{
	if (mil.count() == 0) return;

	NetDirNode *item_node = static_cast<NetDirNode*>(mil.at(0).internalPointer());
	QString file_name = mil.at(0).data().toString();
	QString file_size = mil.at(1).data().toString();
	QString file_modify_time = mil.at(3).data().toString();
	QString file_perm = mil.at(2).data().toString() ;
    // QString file_location = GlobalOption::instance()->remote_codec->toUnicode(item_node->filePath().toAscii());
    QString file_location = item_node->filePath(); //GlobalOption::instance()->remote_codec->toUnicode(item_node->filePath().toAscii());
    file_location = file_location.left(file_location.length() - file_name.length() -1);
    if (file_location.length() == 0) {
        // it is in / path
        file_location = "/";
    }
    
	this->ui_file_prop_dialog->lineEdit->setText(file_name);
	this->ui_file_prop_dialog->lineEdit_2->setText (file_perm.left(1));
    this->ui_file_prop_dialog->lineEdit_3->setText (file_location);

    {
        long long int ifilesize = file_size.toLongLong();
        if (ifilesize >= 1024 * 1024 * 1024) {
            file_size = QString(tr("%1 %2 (%3 Bytes)")).arg(ifilesize/(1024*1024*1024.0))
                .arg("G").arg(ifilesize);
        } else if (ifilesize >= 1024 * 1024) {
            file_size = QString(tr("%1 %2 (%3 Bytes)")).arg(ifilesize/(1024*1024.0))
                .arg("M").arg(ifilesize);
        } else if (ifilesize >= 1024) {
            file_size = QString(tr("%1 %2 (%3 Bytes)")).arg(ifilesize/(1024.0))
                .arg("K").arg(ifilesize);
        } else {
            file_size = QString(tr("%1 %2 (%3 Bytes)")).arg(ifilesize/1)
                .arg("B").arg(ifilesize);
        }
    }
	this->ui_file_prop_dialog->lineEdit_4->setText(file_size);
	this->ui_file_prop_dialog->lineEdit_5->setText(file_modify_time);

	if (file_perm.length() < strlen("drwxr-xr-x")) {
		//qDebug() <<" Invalide perm string";
		//return ;
	} else {//perm format : drwxr-xr-x
        this->update_perm_table(file_perm);
    }
    
    QString file_path = this->ui_file_prop_dialog->lineEdit_3->text()
        + QString("/") + this->ui_file_prop_dialog->lineEdit->text();

    FilePropertiesRetriver *rt = 0;
    // rt = new FilePropertiesRetriver(this->ssh2_sftp, file_path, this);
    rt = new FilePropertiesRetriver(this->conn, this->ssh2_sftp, file_path, this);
    QObject::connect(rt, SIGNAL(file_attr_abtained(QString, void*, const QString &)), 
                     this, SLOT(slot_file_attr_abtained(QString, void*, const QString &)));
    rt->start();
}

void FileProperties::slot_prop_thread_finished()
{
	qDebug() <<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;
    // memory clean, put it here, more safe on windows
    FilePropertiesRetriver *rt = static_cast<FilePropertiesRetriver*>(sender());
    rt->deleteLater();
}
void FileProperties::slot_file_attr_abtained(QString file_name, void *attr, const QString &link_to)
{
    // qDebug() <<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;

	char file_perm[60] = {0};
    QString file_size ;
	LIBSSH2_SFTP_ATTRIBUTES *sftp_attrib = (LIBSSH2_SFTP_ATTRIBUTES *)attr;
    SSHFileInfo fi(sftp_attrib);

    if (fi.isDir()) {
        QIcon icon = qApp->style()->standardIcon(QStyle::SP_DirIcon);
        this->ui_file_prop_dialog->label_13->setPixmap(icon.pixmap(50, 50).scaledToHeight(50));
    } else {
        this->ui_file_prop_dialog->label_13->setPixmap(this->fileIcon(file_name).pixmap(50, 50).scaledToHeight(50));
    }
    
    this->ui_file_prop_dialog->label_15->setText(QString("%1").arg(sftp_attrib->uid));
	this->ui_file_prop_dialog->label_16->setText(QString("%1").arg(sftp_attrib->gid));

	QString mode = digit_mode(sftp_attrib->permissions);
	this->ui_file_prop_dialog->lineEdit_7->setText(mode);
	
    this->ui_file_prop_dialog->lineEdit_6->setText(fi.lastModified().toString("yyyy/MM/dd HH:mm:ss"));
    this->ui_file_prop_dialog->lineEdit_5->setText(fi.lastRead().toString("yyyy/MM/dd HH:mm:ss"));
    
	if (this->ui_file_prop_dialog->lineEdit_2->text() == "D") {
         file_size = QString("%1").arg(sftp_attrib->filesize);
        this->ui_file_prop_dialog->lineEdit_4->setText(file_size);
        this->ui_file_prop_dialog->lineEdit_2->setText(tr("Folder"));
	} else if (this->ui_file_prop_dialog->lineEdit_2->text() == "d") {
        this->ui_file_prop_dialog->lineEdit_2->setText(tr("Folder"));
	} else if (this->ui_file_prop_dialog->lineEdit_2->text() == "l") {
		//qDebug() <<" open link , not process now";
        //TODO 写一个更好的，根据文件后缀判断文件类型的类库
        this->ui_file_prop_dialog->lineEdit_2->setText(tr("Symlink"));
        this->ui_file_prop_dialog->lineEdit->setText(QString("%1 --> %2")
                                                    .arg(file_name).arg(link_to));
	} else {
		// reg file??
        this->ui_file_prop_dialog->lineEdit_2->setText(this->type(file_name));
	}
	strmode(sftp_attrib->permissions,file_perm);
	this->update_perm_table(file_perm);
    free(sftp_attrib);
}

void FileProperties::update_perm_table(QString file_perm)
{
    //在一个线程中操作UI元素很不安全，容易导致程序死锁
    this->ui_file_prop_dialog->label_17->setText(file_perm);
	//perm format : drwxr-xr-x
	{
		QChar rp = file_perm.at(1);
		QChar wp = file_perm.at(1+1);
		QChar xp = file_perm.at(1+2);
        QChar sp = file_perm.at(1+2);

		this->ui_file_prop_dialog->checkBox->setChecked(rp=='r');
		this->ui_file_prop_dialog->checkBox_2->setChecked(wp=='w');
		this->ui_file_prop_dialog->checkBox_3->setChecked(xp=='x'  || sp == 's');
        this->ui_file_prop_dialog->checkBox_10->setChecked(sp == 'S' || sp == 's');
	}
	{
		QChar rp = file_perm.at(4);
		QChar wp = file_perm.at(4+1);
		QChar xp = file_perm.at(4+2);
        QChar sp = file_perm.at(4+2);

		this->ui_file_prop_dialog->checkBox_4->setChecked(rp=='r');
		this->ui_file_prop_dialog->checkBox_5->setChecked(wp=='w');
		this->ui_file_prop_dialog->checkBox_6->setChecked(xp=='x' || sp == 's');
        this->ui_file_prop_dialog->checkBox_11->setChecked(sp == 'S' || sp == 's');
	}
	{
		QChar rp = file_perm.at(7);
		QChar wp = file_perm.at(7+1);
		QChar xp = file_perm.at(7+2);

		this->ui_file_prop_dialog->checkBox_7->setChecked(rp=='r');
		this->ui_file_prop_dialog->checkBox_8->setChecked(wp=='w');
		this->ui_file_prop_dialog->checkBox_9->setChecked(xp=='x' || xp == 'T');
        this->ui_file_prop_dialog->checkBox_12->setChecked(xp=='t'  || xp == 'T');
	}
}


QString FileProperties::type(QString file_name)
{
    QFileInfo info(file_name);
    
    if (file_name == "/")
        return QApplication::translate("QFileDialog", "Drive");
    //     if (info.isFile()) {
    if (1) {
        if (!info.suffix().isEmpty())
            return info.suffix() + QLatin1Char(' ') + QApplication::translate("QFileDialog", "File");
        return QApplication::translate("QFileDialog", "File");
    }

    if (info.isDir())
        return QApplication::translate("QFileDialog",
#ifdef Q_WS_WIN
                                       "File Folder", "Match Windows Explorer"
#else
                                       "Folder", "All other platforms"
#endif
                                      );
    // Windows   - "File Folder"
    // OS X      - "Folder"
    // Konqueror - "Folder"
    // Nautilus  - "folder"

    if (info.isSymLink())
        return QApplication::translate("QFileDialog",
#ifdef Q_OS_MAC
                                       "Alias", "Mac OS X Finder"
#else
                                       "Shortcut", "All other platforms"
#endif
                                      );
    // OS X      - "Alias"
    // Windows   - "Shortcut"
    // Konqueror - "Folder" or "TXT File" i.e. what it is pointing to
    // Nautilus  - "link to folder" or "link to object file", same as Konqueror

    return QApplication::translate("QFileDialog", "Unknown");
}

QIcon FileProperties::fileIcon(QString file_name)
{
    int lastSplashPos = file_name.lastIndexOf(QChar('/'));
    int lastDotPos = file_name.lastIndexOf(QChar('.'));
    QString suffix;
    if (lastDotPos > lastSplashPos) {
        suffix = file_name.right(file_name.length() - lastDotPos - 1);
    }

    if (suffix.isEmpty()) {
        return qApp->style()->standardIcon(QStyle::SP_FileIcon);
    } else {
        if (suffix.right(1) == "~" || suffix.right(1) == "#") {
            suffix = suffix.left(suffix.length() - 1);
        }
        suffix = suffix.toLower();
        // qDebug()<<suffix<<gMimeHash.value(suffix);
        QIcon icon = QIcon(qApp->applicationDirPath() + "/icons/mimetypes/" + gMimeHash.value(suffix) + ".png");
        if (icon.actualSize(QSize(32,32)).isEmpty() || gMimeHash.value(suffix).isEmpty()) {
            icon = qApp->style()->standardIcon(QStyle::SP_FileIcon);
        }
        return icon;
    }

    return QIcon();
}

///////////////////////////////////////////
////    
///////////////////////////////////////////

LocalFileProperties::LocalFileProperties(QWidget *parent)
    : QDialog(parent)
  , ui_file_prop_dialog(new Ui::FileProperties())
{
    this->ui_file_prop_dialog->setupUi(this);
    this->ui_file_prop_dialog->label_13->setPixmap(QPixmap(":/icons/nullget-1.png").scaledToHeight(50));
}

LocalFileProperties::~LocalFileProperties()
{

}

void LocalFileProperties::set_file_info_model_list(QString file_name)
{
    q_debug()<<"";
    this->ui_file_prop_dialog->label_13->setPixmap(this->fileIcon(file_name).pixmap(50, 50).scaledToHeight(50));

    QFileInfo fi(file_name);
    
    this->file_name = file_name;
    QString file_size = QString("%1").arg(fi.size());
    QString file_modify_time = "";

    this->ui_file_prop_dialog->lineEdit->setText(fi.fileName());
    this->ui_file_prop_dialog->lineEdit_2->setText(this->type(file_name));
    this->ui_file_prop_dialog->lineEdit_3->setText(fi.path());

    {
        long long int ifilesize = file_size.toLongLong();
        if (ifilesize >= 1024 * 1024 * 1024) {
            file_size = QString(tr("%1 %2 (%3 Bytes)")).arg(ifilesize/(1024*1024*1024.0))
                .arg("G").arg(ifilesize);
        } else if (ifilesize >= 1024 * 1024) {
            file_size = QString(tr("%1 %2 (%3 Bytes)")).arg(ifilesize/(1024*1024.0))
                .arg("M").arg(ifilesize);
        } else if (ifilesize >= 1024) {
            file_size = QString(tr("%1 %2 (%3 Bytes)")).arg(ifilesize/(1024.0))
                .arg("K").arg(ifilesize);
        } else {
            file_size = QString(tr("%1 %2 (%3 Bytes)")).arg(ifilesize/1)
                .arg("B").arg(ifilesize);
        }
    }
    this->ui_file_prop_dialog->lineEdit_4->setText(file_size);

    this->ui_file_prop_dialog->lineEdit_5->setText(fi.lastModified().toString("yyyy/MM/dd hh:mm:ss"));
    this->ui_file_prop_dialog->lineEdit_6->setText(fi.lastRead().toString("yyyy/MM/dd hh:mm:ss")); //2007/11/28 06:53:45

    QString mode = this->digit_mode(fi.permissions());
    this->ui_file_prop_dialog->lineEdit_7->setText(mode);
    this->ui_file_prop_dialog->label_15->setText(fi.owner());
    this->ui_file_prop_dialog->label_16->setText(fi.group());

    this->update_perm_table(file_name);
}

QString LocalFileProperties::digit_mode(int mode)
{
    int keys[] = {
        QFile::ReadOwner,
        QFile::WriteOwner,
        QFile::ExeOwner,
        QFile::ReadUser,
        QFile::WriteUser,
        QFile::ExeUser,
        QFile::ReadGroup,
        QFile::WriteGroup,
        QFile::ExeGroup,
        QFile::ReadOther,
        QFile::WriteOther,
        QFile::ExeOther
    };
    char dmode[5] = {0};
    int i = 0, v = 0;;
    for (i =0 ;i < 4 ; i++) {
        v = 0;
        if (mode & keys[i*3]) v += 4;
        if (mode & keys[i*3+1]) v += 2;
        if (mode & keys[i*3+2]) v += 1;
        sprintf(dmode+strlen(dmode), "%d", v);
    }
    dmode[0] = '0';

    return QString(dmode);
}

void LocalFileProperties::update_perm_table(QString file_name)
{
    QFileInfo fi(file_name);
    QFile::Permissions fp = fi.permissions();

    {
        this->ui_file_prop_dialog->checkBox->setChecked(fp & QFile::ReadOwner/*rp=='r'*/);
        this->ui_file_prop_dialog->checkBox_2->setChecked(fp & QFile::WriteOwner/*wp=='w'*/);
        this->ui_file_prop_dialog->checkBox_3->setChecked(fp & QFile::ExeOwner/*xp=='x'*/);
    }
    {
        this->ui_file_prop_dialog->checkBox_4->setChecked(fp & QFile::ReadGroup /*rp=='r'*/);
        this->ui_file_prop_dialog->checkBox_5->setChecked(fp & QFile::WriteGroup /*wp=='w'*/);
        this->ui_file_prop_dialog->checkBox_6->setChecked(fp & QFile::ExeGroup /*xp=='x'*/);
    }
    {

        this->ui_file_prop_dialog->checkBox_7->setChecked(fp & QFile::ReadOther/*rp=='r'*/);
        this->ui_file_prop_dialog->checkBox_8->setChecked(fp & QFile::WriteOther /*wp=='w'*/);
        this->ui_file_prop_dialog->checkBox_9->setChecked(fp & QFile::ExeOther /*xp=='x'*/);
    }
}


QString LocalFileProperties::type(QString file_name)
{
    QFileInfo info(file_name);
    
    if (file_name == "/")
        return QApplication::translate("QFileDialog", "Drive");
    if (info.isFile()) {
        //     if (1) {
        if (!info.suffix().isEmpty())
            return info.suffix() + QLatin1Char(' ') + QApplication::translate("QFileDialog", "File");
        return QApplication::translate("QFileDialog", "File");
    }

    if (info.isDir())
        return QApplication::translate("QFileDialog",
#ifdef Q_WS_WIN
                                       "File Folder", "Match Windows Explorer"
#else
                                       "Folder", "All other platforms"
#endif
                                      );
    // Windows   - "File Folder"
    // OS X      - "Folder"
    // Konqueror - "Folder"
    // Nautilus  - "folder"

    if (info.isSymLink())
        return QApplication::translate("QFileDialog",
#ifdef Q_OS_MAC
                                       "Alias", "Mac OS X Finder"
#else
                                       "Shortcut", "All other platforms"
#endif
                                      );
    // OS X      - "Alias"
    // Windows   - "Shortcut"
    // Konqueror - "Folder" or "TXT File" i.e. what it is pointing to
    // Nautilus  - "link to folder" or "link to object file", same as Konqueror

    return QApplication::translate("QFileDialog", "Unknown");
}

QIcon LocalFileProperties::fileIcon(QString file_name)
{
    int lastSplashPos = file_name.lastIndexOf(QChar('/'));
    int lastDotPos = file_name.lastIndexOf(QChar('.'));
    QString suffix;
    if (lastDotPos > lastSplashPos) {
        suffix = file_name.right(file_name.length() - lastDotPos - 1);
    }

    if (QFileInfo(file_name).isDir()) {
        return qApp->style()->standardIcon(QStyle::SP_DirIcon);
    } else if (suffix.isEmpty()) {
        return qApp->style()->standardIcon(QStyle::SP_FileIcon);
    } else {
        if (suffix.right(1) == "~" || suffix.right(1) == "#") {
            suffix = suffix.left(suffix.length() - 1);
        }
        suffix = suffix.toLower();
        // qDebug()<<suffix<<gMimeHash.value(suffix);
        QIcon icon = QIcon(qApp->applicationDirPath() + "/icons/mimetypes/" + gMimeHash.value(suffix) + ".png");
        if (icon.actualSize(QSize(32,32)).isEmpty() || gMimeHash.value(suffix).isEmpty()) {
            icon = qApp->style()->standardIcon(QStyle::SP_FileIcon);
        }
        return icon;
    }

    return QIcon();
}

