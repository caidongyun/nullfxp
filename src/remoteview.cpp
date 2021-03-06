// remoteview.cpp --- 
// 
// Author: liuguangzhao
// Copyright (C) 2007-2010 liuguangzhao@users.sf.net
// URL: http://www.qtchina.net http://nullget.sourceforge.net
// Created: 2008-05-05 21:49:36 +0800
// Version: $Id$
// 

#include <QtCore>
#include <QtGui>

#include "ssh_info.h"

#include "globaloption.h"
#include "utils.h"

#include "progressdialog.h"
#include "localview.h"
#include "remoteview.h"
#include "netdirsortfiltermodel.h"
#include "ui_remoteview.h"

#include "fileproperties.h"
#include "encryptiondetailfocuslabel.h"
#include "encryptiondetaildialog.h"

#ifndef _MSC_VER
#warning "wrapper lower class, drop this include"
#endif

#include "rfsdirnode.h"
#include "completelineeditdelegate.h"
#include "connection.h"

RemoteView::RemoteView(QMdiArea *main_mdi_area, LocalView *local_view, QWidget *parent)
    : QWidget(parent)
    , uiw(new Ui::RemoteView())
{
    this->uiw->setupUi(this);
    this->local_view = local_view;
    this->main_mdi_area = main_mdi_area;
    this->setObjectName("NetDirView");

    ///////
    this->status_bar = new QStatusBar();
    this->entriesLabel = new QLabel(tr("Entries label"), this);
    this->entriesLabel->setTextInteractionFlags(this->entriesLabel->textInteractionFlags() 
                                                | Qt::TextSelectableByMouse);
    this->status_bar->addPermanentWidget(this->entriesLabel);
    this->status_bar->addPermanentWidget(this->enc_label = new EncryptionDetailFocusLabel("ENC", this));
    QObject::connect(this->enc_label, SIGNAL(mouseDoubleClick()),
                     this, SLOT(encryption_focus_label_double_clicked()));
    HostInfoDetailFocusLabel *hi_label = new HostInfoDetailFocusLabel("HI", this);
    this->status_bar->addPermanentWidget(hi_label);
    QObject::connect(hi_label, SIGNAL(mouseDoubleClick()), 
                     this, SLOT(host_info_focus_label_double_clicked()));
    this->layout()->addWidget(this->status_bar);

    ////////////
    //     this->uiw->treeView->setAcceptDrops(false);
    //     this->uiw->treeView->setDragEnabled(false);
    //     this->uiw->treeView->setDropIndicatorShown(false);
    //     this->uiw->treeView->setDragDropMode(QAbstractItemView::NoDragDrop);
    //this->uiw->treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    
    QObject::connect(this->uiw->treeView, SIGNAL(customContextMenuRequested(const QPoint &)),
                     this, SLOT(slot_dir_tree_customContextMenuRequested (const QPoint &)));
    QObject::connect(this->uiw->tableView,SIGNAL(customContextMenuRequested(const QPoint &)),
                     this, SLOT(slot_dir_tree_customContextMenuRequested (const QPoint & )));
    QObject::connect(this->uiw->listView_2, SIGNAL(customContextMenuRequested(const QPoint &)),
                     this, SLOT(slot_dir_tree_customContextMenuRequested (const QPoint &)));
    // this->init_popup_context_menu();
    
    this->in_remote_dir_retrive_loop = false;
    this->uiw->tableView->test_use_qt_designer_prompt = 0;
    CompleteLineEditDelegate *delegate = new CompleteLineEditDelegate();
    this->uiw->tableView->setItemDelegate(delegate);
    this->uiw->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    this->uiw->treeView->setItemDelegate(delegate);
    this->uiw->treeView->setAnimated(true);

    // something about dir nav
    this->is_dir_complete_request = false;
    // this->dir_complete_request_prefix = "";
    QObject::connect(this->uiw->widget, SIGNAL(goHome()),
                     this, SLOT(slot_dir_nav_go_home()));
    QObject::connect(this->uiw->widget, SIGNAL(dirPrefixChanged(const QString&)),
                     this, SLOT(slot_dir_nav_prefix_changed(const QString&)));
    QObject::connect(this->uiw->widget, SIGNAL(dirInputConfirmed(const QString&)),
                     this, SLOT(slot_dir_nav_input_comfirmed(const QString&)));
    QObject::connect(this->uiw->widget, SIGNAL(iconSizeChanged(int)),
                     this, SLOT(slot_icon_size_changed(int)));
    // this->uiw->widget->onSetHome(QDir::homePath()); // call from set_user_home_path
    // this->setFileListViewMode(GlobalOption::FLV_BOTH_VIEW);
    this->setFileListViewMode(GlobalOption::FLV_DETAIL);
    this->m_operationLogModel = NULL;
}

RemoteView::~RemoteView()
{
    this->uiw->treeView->setModel(0);
    delete this->remote_dir_model;
}

void RemoteView::init_popup_context_menu()
{
    this->dir_tree_context_menu = new QMenu();
    QAction *action;
    action  = new QAction(tr("Download"), 0);
    this->dir_tree_context_menu->addAction(action);
    QObject::connect(action, SIGNAL(triggered()), this, SLOT(slot_new_transfer()));
    
    action = new QAction("", 0);
    action->setSeparator(true);
    this->dir_tree_context_menu->addAction(action);
    
    action = new QAction(tr("Refresh"), 0);
    this->dir_tree_context_menu->addAction(action);
    QObject::connect(action, SIGNAL(triggered()), this, SLOT(slot_refresh_directory_tree()));
    
    action = new QAction(tr("Properties..."), 0);
    this->dir_tree_context_menu->addAction(action);
    QObject::connect(action, SIGNAL(triggered()), this, SLOT(slot_show_properties()));
    attr_action = action ;
    
    action = new QAction(tr("Show Hidden"), 0);
    action->setCheckable(true);
    this->dir_tree_context_menu->addAction(action);
    QObject::connect(action, SIGNAL(toggled(bool)), this, SLOT(slot_show_hidden(bool)));
    
    action = new QAction("", 0);
    action->setSeparator(true);
    this->dir_tree_context_menu->addAction(action);

    //TODO  CUT, COPY, PASTE, ||set initial directory,||open,open with    
    action = new QAction(tr("Copy &Path"), 0);
    this->dir_tree_context_menu->addAction(action);
    QObject::connect(action, SIGNAL(triggered()), this, SLOT(slot_copy_path()));

    action = new QAction(tr("Copy &URL"), 0);
    this->dir_tree_context_menu->addAction(action);
    QObject::connect(action, SIGNAL(triggered()), this, SLOT(slot_copy_url()));
        
    action = new QAction(tr("Create directory..."), 0);
    this->dir_tree_context_menu->addAction(action);
    QObject::connect(action, SIGNAL(triggered()), this, SLOT(slot_mkdir()));
    
    action = new QAction(tr("Delete directory"), 0);
    this->dir_tree_context_menu->addAction(action);
    QObject::connect(action, SIGNAL(triggered()), this, SLOT(slot_rmdir()));

    action = new QAction(tr("Rename..."),0);
    this->dir_tree_context_menu->addAction(action);
    QObject::connect(action, SIGNAL(triggered()), this, SLOT(slot_rename()));
    
    action = new QAction("", 0);
    action->setSeparator(true);
    this->dir_tree_context_menu->addAction(action);
        
    //递归删除目录功能，删除文件的用户按钮
    action = new QAction(tr("Remove recursively !!!"), 0);
    this->dir_tree_context_menu->addAction(action);
    QObject::connect(action, SIGNAL(triggered()), this, SLOT(rm_file_or_directory_recursively()));

    // q_debug()<<"aaaaaaaaaaaaaaaaaaa";
    // // 编码设置菜单
    // QMenu *emenu = this->encodingMenu();
    // this->dir_tree_context_menu->addMenu(emenu);

}

QMenu *RemoteView::encodingMenu()
{
    return NULL;
}

void RemoteView::slot_show_fxp_command_log(bool show)
{
    this->uiw->listView->setVisible(show);    
}

void RemoteView::i_init_dir_view()
{
    qDebug()<<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;
    Q_ASSERT(1 == 2);

    this->remote_dir_model = new RemoteDirModel();
    // this->remote_dir_model->set_ssh2_handler(this->ssh2_sess);
    this->remote_dir_model->setConnection(this->conn);
    
    // this->remote_dir_model->set_user_home_path(this->user_home_path);
    // this->m_tableProxyModel = new DirTableSortFilterModel();
    // this->m_tableProxyModel->setSourceModel(this->remote_dir_model);
    // this->m_treeProxyModel = new DirTreeSortFilterModelEX();
    // this->m_treeProxyModel->setSourceModel(this->remote_dir_model);
    
    this->uiw->treeView->setModel(m_treeProxyModel);
    // this->uiw->treeView->setModel(this->remote_dir_model);
    this->uiw->treeView->setAcceptDrops(true);
    this->uiw->treeView->setDragEnabled(false);
    this->uiw->treeView->setDropIndicatorShown(true);
    this->uiw->treeView->setDragDropMode(QAbstractItemView::DropOnly);

    QObject::connect(this->remote_dir_model,
                     SIGNAL(sig_drop_mime_data(const QMimeData *, Qt::DropAction, int, int, const QModelIndex &)),
                     this, SLOT(slot_drop_mime_data(const QMimeData *, Qt::DropAction, int, int, const QModelIndex &)));
    
    QObject::connect(this->remote_dir_model, SIGNAL(enter_remote_dir_retrive_loop()),
                     this, SLOT(slot_enter_remote_dir_retrive_loop()));
    QObject::connect(this->remote_dir_model, SIGNAL(leave_remote_dir_retrive_loop()),
                     this, SLOT(slot_leave_remote_dir_retrive_loop()));
    
    this->uiw->treeView->expandAll();
    this->uiw->treeView->setColumnWidth(0, this->uiw->treeView->columnWidth(0)*2);
    
    //这里设置为true时，导致这个treeView不能正确显示滚动条了，为什么呢?
    //this->uiw->treeView->setColumnHidden( 1, false);
    this->uiw->treeView->setColumnWidth(1, 0);//使用这种方法隐藏看上去就正常了。
    this->uiw->treeView->setColumnHidden(2, true);
    this->uiw->treeView->setColumnHidden(3, true);
    
    /////tableView
    this->uiw->tableView->setModel(this->m_tableProxyModel);
    // this->uiw->tableView->setRootIndex(this->m_tableProxyModel->index(this->user_home_path));

    //change row height of table 
    // if (this->m_tableProxyModel->rowCount(this->m_tableProxyModel->index(this->user_home_path)) > 0) {
    //     this->table_row_height = this->uiw->tableView->rowHeight(0)*2/3;
    // } else {
    //     this->table_row_height = 20 ;
    // }
    // for (int i = 0; i < this->m_tableProxyModel->rowCount(this->m_tableProxyModel->index(this->user_home_path)); i ++) {
    //     this->uiw->tableView->setRowHeight(i, this->table_row_height);
    // }
    this->uiw->tableView->resizeColumnToContents(0);
    
    /////
    QObject::connect(this->uiw->treeView, SIGNAL(clicked(const QModelIndex &)),
                     this, SLOT(slot_dir_tree_item_clicked(const QModelIndex &)));
    QObject::connect(this->uiw->tableView, SIGNAL(doubleClicked(const QModelIndex &)),
                     this, SLOT(slot_dir_file_view_double_clicked(const QModelIndex &)));
    QObject::connect(this->uiw->tableView, SIGNAL(drag_ready()),
                     this, SLOT(slot_drag_ready()));

    //TODO 连接remoteview.treeView 的drag信号
    
    //显示SSH服务器信息
    QString ssh_server_version = libssh2_session_get_remote_version(this->conn->sess);
    int ssh_sftp_version = libssh2_sftp_get_version(this->ssh2_sftp);
    QString status_msg = QString("Ready. (%1  SFTP: V%2)").arg(ssh_server_version).arg(ssh_sftp_version); 
    this->status_bar->showMessage(status_msg);
}

void RemoteView::expand_to_home_directory(QModelIndex parent_model, int level)
{
    Q_UNUSED(parent_model);
    Q_UNUSED(level);
}

void RemoteView::expand_to_directory(QString path, int level)
{
    Q_UNUSED(path);
    Q_UNUSED(level);
}

void RemoteView::slot_disconnect_from_remote_host()
{
    this->uiw->treeView->setModel(0);
    delete this->remote_dir_model;
    this->remote_dir_model = 0;
}

void RemoteView::slot_dir_tree_customContextMenuRequested(const QPoint & pos)
{
    this->curr_item_view = static_cast<QAbstractItemView*>(sender());
    QPoint real_pos = this->curr_item_view->mapToGlobal(pos);
    real_pos = QPoint(real_pos.x() + 12, real_pos.y() + 36);
    attr_action->setEnabled(!this->in_remote_dir_retrive_loop);
    this->dir_tree_context_menu->popup(real_pos);
}

void RemoteView::slot_new_transfer()
{
    qDebug()<<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;
     
    // QString file_path;
    // TaskPackage remote_pkg(PROTO_SFTP);
    
    // if (this->in_remote_dir_retrive_loop) {
    //     QMessageBox::warning(this, tr("Notes:"), tr("Retriving remote directory tree,wait a minute please."));
    //     return ;
    // }
    
    // QItemSelectionModel *ism = this->curr_item_view->selectionModel();
    // QModelIndex cidx, idx;

    // if (ism == 0) {
    //     QMessageBox::critical(this, tr("Warning..."), tr("Maybe you haven't connected"));
    //     return;
    // }
    
    // QModelIndexList mil = ism->selectedIndexes();
    
    // for(int i = 0 ; i < mil.size(); i +=4 ) {
    //     QModelIndex midx = mil.at(i);
    //     NetDirNode *dti = (NetDirNode*)
    //         (this->curr_item_view!=this->uiw->treeView 
    //          ? this->m_tableProxyModel->mapToSource(midx).internalPointer() 
    //          : (this->m_treeProxyModel->mapToSource(midx ).internalPointer()));
    //     qDebug()<<dti->fileName()<<" "<<" "<<dti->fullPath;
    //     file_path = dti->fullPath;
    //     remote_pkg.files<<file_path;
    // }

    // remote_pkg.host = this->conn->hostName;
    // remote_pkg.port = QString("%1").arg(this->conn->port);
    // remote_pkg.username = this->conn->userName;
    // remote_pkg.password = this->conn->password;
    // remote_pkg.pubkey = this->conn->pubkey;

    // this->slot_new_download_requested(remote_pkg);
}

QString RemoteView::get_selected_directory()
{
    QString file_path;
    
    // QItemSelectionModel *ism = this->uiw->treeView->selectionModel();
    
    // if (ism == 0) {
    //     QMessageBox::critical(this, tr("Warning..."), tr("Maybe you haven't connected"));                
    //     return file_path;
    // }
    
    // QModelIndexList mil = ism->selectedIndexes();

    // for (int i = 0 ; i < mil.size(); i +=4) {
    //     QModelIndex midx = mil.at(i);
    //     QModelIndex aim_midx =  this->m_treeProxyModel->mapToSource(midx);
    //     NetDirNode *dti = (NetDirNode*) aim_midx.internalPointer();
    //     qDebug()<<dti->fileName()<<" "<< dti->fullPath;
    //     // if (this->m_treeProxyModel->isDir(midx)) {
    //     // 	  file_path = dti->fullPath;
    //     // } else {
    //     // 	  file_path = "";
    //     // }
    // }
    
    return file_path;
}

void RemoteView::set_user_home_path(const QString &user_home_path)
{
    this->user_home_path = user_home_path;
}
void RemoteView::setConnection(Connection *conn)
{
    this->conn = conn;
    this->ssh2_sftp = libssh2_sftp_init(this->conn->sess);
    assert(this->ssh2_sftp != 0);    
    this->setWindowTitle(this->windowTitle() + ": " + this->conn->userName + "@" + this->conn->hostName);
}

void RemoteView::closeEvent(QCloseEvent *event)
{
    qDebug()<<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;
    event->ignore();
    if (this->in_remote_dir_retrive_loop) {
        //TODO 怎么能友好的结束
        //QMessageBox::warning(this,tr("Attentions:"),tr("Retriving remote directory tree, wait a minute please.") );
        //return ;
        //如果说是在上传或者下载,则强烈建议用户先关闭传输窗口，再关闭连接
        if (this->own_progress_dialog != 0) {
            QMessageBox::warning(this, tr("Attentions:"), tr("You can't close connection when transfering file."));
            return ;
        }
    }
    //this->setVisible(false);
    if (QMessageBox::question(this, tr("Attemp to close this window?"),tr("Are you sure disconnect from %1?").arg(this->windowTitle()), QMessageBox::Ok|QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Ok) {
        this->setVisible(false);
        qDebug()<<"delete remote view";
        delete this ;
    }
}

void RemoteView::keyPressEvent(QKeyEvent * e)
{
    switch(e->key()) {
#ifdef Q_WS_MAC
    case Qt::Key_Enter:
#else
    case Qt::Key_F2:
#endif
        QTimer::singleShot(1, this, SLOT(slot_rename()));
        // QTimer::singleShot(1, this, SLOT(slot_edit_selected_host()));
        break;
    default:
        QWidget::keyPressEvent(e);
        break;
    }
}

void RemoteView::slot_custom_ui_area()
{
    qDebug()<<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;
    
    QSizePolicy sp;
    sp.setVerticalPolicy(QSizePolicy::Ignored);
    this->uiw->listView->setSizePolicy( sp ) ;
    //这个设置必须在show之前设置才有效果
    this->uiw->splitter->setStretchFactor(0,1);
    this->uiw->splitter->setStretchFactor(1,2);

    this->uiw->splitter_2->setStretchFactor(0,6);
    this->uiw->splitter_2->setStretchFactor(1,1);
    this->uiw->listView->setVisible(false);//暂时没有功能在里面先隐藏掉
    //this->uiw->tableView->setVisible(false);
    qDebug()<<this->geometry();
    this->setGeometry(this->x(),this->y(),this->width(),this->height()*2);
    qDebug()<<this->geometry();
}

void RemoteView::slot_enter_remote_dir_retrive_loop()
{
    qDebug()<<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;
    this->in_remote_dir_retrive_loop = true ;
    this->remote_dir_model->set_keep_alive(false);
    this->orginal_cursor = this->uiw->splitter->cursor();
    this->uiw->splitter->setCursor(Qt::BusyCursor);
}

void RemoteView::slot_leave_remote_dir_retrive_loop()
{
    qDebug()<<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;

    this->uiw->splitter->setCursor(this->orginal_cursor);
    this->remote_dir_model->set_keep_alive(true);
    this->in_remote_dir_retrive_loop = false ;
    for (int i = 0 ; i < this->m_tableProxyModel->rowCount(this->uiw->tableView->rootIndex()); i ++) {
        this->uiw->tableView->setRowHeight(i, this->table_row_height);
    }
    this->uiw->tableView->resizeColumnToContents ( 0 );
}

void RemoteView::update_layout()
{
    qDebug() <<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;
    
    // QString file_path ;
    
    // QItemSelectionModel *ism = this->uiw->treeView->selectionModel();
    
    // if (ism == 0) {
    //     //QMessageBox::critical(this,tr("waring..."),tr("maybe you haven't connected"));                
    //     //return file_path ;
    //     qDebug()<<" why???? no QItemSelectionModel??";        
    //     return ;
    // }
    
    // QModelIndexList mil = ism->selectedIndexes();
    
    // if (mil.count() == 0) {
    //     qDebug()<<" selectedIndexes count :"<< mil.count() << " why no item selected????";
    // }
    
    // for (int i = 0 ; i < mil.size(); i += 4) {
    //     QModelIndex midx = mil.at(i);
    //     qDebug()<<midx ;
    //     //这个地方为什么不使用mapToSource会崩溃呢？
    //     NetDirNode *dti = static_cast<NetDirNode*>
    //         (this->m_treeProxyModel->mapToSource(midx).internalPointer());
    //     qDebug()<<dti->fileName()<<" "<< dti->fullPath;
    //     file_path = dti->fullPath;
    //     dti->retrFlag = 1;
    //     dti->prevFlag=9;
    //     this->remote_dir_model->slot_remote_dir_node_clicked(this->m_treeProxyModel->mapToSource(midx));
    // }
}

void RemoteView::slot_refresh_directory_tree()
{
    this->update_layout();
}

void RemoteView::slot_show_properties()
{
    // QItemSelectionModel *ism = this->curr_item_view->selectionModel();
    
    // if (ism == 0) {
    //     qDebug()<<" why???? no QItemSelectionModel??";        
    //     return;
    // }
    
    // QModelIndexList mil = ism->selectedIndexes();
    // QModelIndexList aim_mil;
    // if (this->curr_item_view == this->uiw->treeView) {
    //     for (int i = 0 ; i < mil.count() ; i ++) {
    //         aim_mil << this->m_treeProxyModel->mapToSource(mil.at(i));
    //     }
    // } else {
    //     for (int i = 0 ; i < mil.count() ; i ++) {
    //         aim_mil << this->m_tableProxyModel->mapToSource(mil.at(i));
    //     }
    // }
    // if (aim_mil.count() == 0) {
    //     qDebug()<<" why???? no QItemSelectionModel??";
    //     return;
    // }
    // //  文件类型，大小，几个时间，文件权限
    // //TODO 从模型中取到这些数据并显示在属性对话框中。
    // FileProperties *fp = new FileProperties(this);
    // fp->set_ssh2_sftp(this->ssh2_sftp);
    // fp->setConnection(this->conn);
    // fp->set_file_info_model_list(aim_mil);
    // fp->exec();
    // delete fp;
}

void RemoteView::slot_mkdir()
{
    // QString dir_name;
    
    // QItemSelectionModel *ism = this->curr_item_view->selectionModel();
    
    // if (ism == 0) {
    //     qDebug()<<" why???? no QItemSelectionModel??";
    //     QMessageBox::critical(this, tr("Warning..."), tr("Maybe you haven't connected"));                
    //     return ;
    // }
    
    // QModelIndexList mil = ism->selectedIndexes();
    
    // if (mil.count() == 0) {
    //     qDebug()<<" selectedIndexes count :"<< mil.count() << " why no item selected????";
    //     QMessageBox::critical(this, tr("Warning..."), tr("No item selected"));
    //     return ;
    // }
    
    // QModelIndex midx = mil.at(0);
    // QModelIndex aim_midx = (this->curr_item_view == this->uiw->treeView) ? this->m_treeProxyModel->mapToSource(midx): this->m_tableProxyModel->mapToSource(midx) ;
    // NetDirNode *dti = (NetDirNode*)(aim_midx.internalPointer());
    
    // //检查所选择的项是不是目录
    // if (!this->remote_dir_model->isDir(aim_midx)) {
    //     QMessageBox::critical(this, tr("waring..."), tr("The selected item is not a directory."));
    //     return ;
    // }
    
    // dir_name = QInputDialog::getText(this, tr("Create directory:"),
    //                                  tr("Input directory name:").leftJustified(100, ' '),
    //                                  QLineEdit::Normal,
    //                                  tr("new_direcotry"));
    // if (dir_name == QString::null) {
    //     return ;
    // } 
    // if (dir_name.length() == 0) {
    //     qDebug()<<" selectedIndexes count :"<< mil.count() << " why no item selected????";
    //     QMessageBox::critical(this, tr("waring..."), tr("no directory name supplyed "));
    //     return;
    // }
    // //TODO 将 file_path 转换编码再执行下面的操作
    
    // this->remote_dir_model->slot_execute_command(dti, aim_midx.internalPointer(), SSH2_FXP_MKDIR, dir_name);
    
}

void RemoteView::slot_rmdir()
{
    qDebug() <<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;
    
    QItemSelectionModel *ism = this->curr_item_view->selectionModel();
    QModelIndex cidx, idx, pidx;
    
    if (ism == 0) {
        qDebug()<<" why???? no QItemSelectionModel??";
        QMessageBox::critical(this, tr("Warning..."), tr("Maybe you haven't connected"));                
        return ;
    }

    if (!ism->hasSelection()) {
        qDebug()<<"selectedIndexes count :"<<ism->hasSelection()<<"why no item selected????";
        QMessageBox::critical(this, tr("Warning..."), tr("No item selected"));
        return;
    }
    
    cidx = ism->currentIndex();
    idx = ism->model()->index(cidx.row(), 0, cidx.parent());
    
    // QModelIndexList mil = ism->selectedIndexes();
    
    // if (mil.count() == 0) {
    //     qDebug()<<"selectedIndexes count:"<<mil.count()<<"why no item selected????";
    //     QMessageBox::critical(this, tr("Warning..."), tr("No item selected").leftJustified(50, ' '));
    //     return;
    // }
    
    QModelIndex midx = idx;
    QModelIndex proxyIndex = midx;
    QModelIndex sourceIndex = (this->curr_item_view == this->uiw->treeView) 
        ? this->m_treeProxyModel->mapToSource(midx)
        : this->m_tableProxyModel->mapToSource(midx);    

    QModelIndex useIndex = sourceIndex;
    QModelIndex parent_model = useIndex.parent();
    NetDirNode *parent_item = (NetDirNode*)parent_model.internalPointer();
    
    // check if the selected item is a directory
    if (this->remote_dir_model->isDir(useIndex)
        || this->remote_dir_model->isSymLinkToDir(useIndex)) {
        QPersistentModelIndex *persisIndex = new QPersistentModelIndex(parent_model);
        this->remote_dir_model->slot_execute_command(parent_item, persisIndex, SSH2_FXP_RMDIR,
                                                     this->remote_dir_model->fileName(useIndex));
    } else {
        q_debug()<<"selected item is not a directory";
        int btn = QMessageBox::critical(this, tr("Warning..."), 
                                        tr("Selected item is not a directory.\n\t%1\n\n%2")
                                        .arg(this->remote_dir_model->filePath(useIndex))
                                        .arg(tr("Still remove it?"))
                                        .leftJustified(50, ' '), 
                                        QMessageBox::Yes, QMessageBox::Cancel);
        if (btn == QMessageBox::Yes) {
            QPersistentModelIndex *persisIndex = new QPersistentModelIndex(parent_model);
            this->remote_dir_model->slot_execute_command(parent_item, persisIndex,
                                                         SSH2_FXP_REMOVE,
                                                         this->remote_dir_model->fileName(useIndex));
        } else {
            q_debug()<<"Cancel remove directory, it's really a file";
        }
    }
}

void RemoteView::rm_file_or_directory_recursively()
{
    qDebug()<<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;
    
    // QItemSelectionModel *ism = this->curr_item_view->selectionModel();
    
    // if (ism == 0) {
    //     qDebug()<<"why???? no QItemSelectionModel??";
    //     QMessageBox::critical(this, tr("Warning..."), tr("Maybe you haven't connected"));                
    //     return;
    // }
    
    // QModelIndexList mil = ism->selectedIndexes();
    
    // if (mil.count() == 0) {
    //     qDebug()<<" selectedIndexes count :"<< mil.count() << " why no item selected????";
    //     QMessageBox::critical(this, tr("Warning..."), tr("No item selected"));
    //     return;
    // }
    // //TODO 处理多选的情况
    // QModelIndex midx = mil.at(0);
    // QModelIndex aim_midx = (this->curr_item_view == this->uiw->treeView) 
    //     ? this->m_treeProxyModel->mapToSource(midx)
    //     : this->m_tableProxyModel->mapToSource(midx);
    // NetDirNode *dti = (NetDirNode*) aim_midx.internalPointer();
    // if (QMessageBox::warning(this, tr("Warning:"), 
    //                          tr("Are you sure remove this directory and it's subnodes"),
    //                          QMessageBox::Yes, QMessageBox::Cancel) == QMessageBox::Yes) {
    //     QModelIndex parent_model =  aim_midx.parent() ;
    //     NetDirNode *parent_item = (NetDirNode*)parent_model.internalPointer();
        
    //     this->remote_dir_model->slot_execute_command(parent_item, parent_model.internalPointer(),
    //                                                  SSH2_FXP_REMOVE, dti->_fileName);
    // }
}

void RemoteView::slot_rename()
{
    qDebug()<<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;
    
    // QItemSelectionModel *ism = this->curr_item_view->selectionModel();
    
    // if (ism == 0) {
    //     qDebug()<<" why???? no QItemSelectionModel??";
    //     QMessageBox::critical(this, tr("waring..."), tr("maybe you haven't connected"));                
    //     return;
    // }
    
    // QModelIndexList mil = ism->selectedIndexes();
    
    // if (mil.count() == 0 ) {
    //     qDebug()<<"selectedIndexes count :"<< mil.count() << " why no item selected????";
    //     QMessageBox::critical(this, tr("waring..."), tr("no item selected"));
    //     return;
    // }
    // this->curr_item_view->edit(mil.at(0));
}
void RemoteView::slot_copy_path()
{
    // QItemSelectionModel *ism = this->curr_item_view->selectionModel();
    
    // if (ism == 0) {
    //     qDebug()<<"why???? no QItemSelectionModel??";
    //     QMessageBox::critical(this, tr("Warning..."), tr("Maybe you haven't connected"));
    //     return;
    // }
    
    // QModelIndexList mil = ism->selectedIndexes();
    
    // if (mil.count() == 0) {
    //     qDebug()<<"selectedIndexes count :"<< mil.count() << " why no item selected????";
    //     QMessageBox::critical(this, tr("Warning..."), tr("No item selected").leftJustified(50, ' '));
    //     return;
    // }
    
    // QModelIndex midx = mil.at(0);
    // QModelIndex aim_midx = (this->curr_item_view == this->uiw->treeView) 
    //     ? this->m_treeProxyModel->mapToSource(midx)
    //     : this->m_tableProxyModel->mapToSource(midx);    
    // NetDirNode *dti = (NetDirNode*)aim_midx.internalPointer();

    // QApplication::clipboard()->setText(dti->fullPath);
}

void RemoteView::slot_copy_url()
{
    // QItemSelectionModel *ism = this->curr_item_view->selectionModel();
    
    // if (ism == 0) {
    //     qDebug()<<"why???? no QItemSelectionModel??";
    //     QMessageBox::critical(this, tr("Warning..."), tr("Maybe you haven't connected"));
    //     return;
    // }
    
    // QModelIndexList mil = ism->selectedIndexes();
    
    // if (mil.count() == 0) {
    //     qDebug()<<" selectedIndexes count :"<< mil.count() << " why no item selected????";
    //     QMessageBox::critical(this, tr("Warning..."), tr("No item selected").leftJustified(50, ' '));
    //     return;
    // }
    
    // QModelIndex midx = mil.at(0);
    // QModelIndex aim_midx = (this->curr_item_view == this->uiw->treeView) 
    //     ? this->m_treeProxyModel->mapToSource(midx)
    //     : this->m_tableProxyModel->mapToSource(midx);    
    // NetDirNode *dti = (NetDirNode*) aim_midx.internalPointer();

    // QString url = QString("sftp://%1@%2:%3%4").arg(this->conn->userName)
    //     .arg(this->conn->hostName).arg(this->conn->port).arg(dti->fullPath);
    // QApplication::clipboard()->setText(url);
}


void RemoteView::slot_new_upload_requested(TaskPackage local_pkg, TaskPackage remote_pkg)
{
    RemoteView *remote_view = this;
    ProgressDialog *pdlg = new ProgressDialog();

    pdlg->set_transfer_info(local_pkg, remote_pkg);

    QObject::connect(pdlg, SIGNAL(transfer_finished(int, QString)),
                     remote_view, SLOT(slot_transfer_finished (int, QString)));

    this->main_mdi_area->addSubWindow(pdlg);
    pdlg->show();
    this->own_progress_dialog = pdlg;

}
void RemoteView::slot_new_upload_requested(TaskPackage local_pkg)
{
    QString remote_file_name ;
    RemoteView *remote_view = this ;
    TaskPackage remote_pkg(PROTO_SFTP);

    qDebug()<<" window title :" << remote_view->windowTitle();

    remote_file_name = remote_view->get_selected_directory();    

    if (remote_file_name.length() == 0) {
        QMessageBox::critical(this, tr("Warning..."), tr("you should selecte a remote file directory."));
    } else {
        remote_pkg.files<<remote_file_name;

        // remote_pkg.host = this->host_name;
        // remote_pkg.username = this->user_name;
        // remote_pkg.password = this->password;
        // remote_pkg.port = QString("%1").arg(this->port);
        // remote_pkg.pubkey = this->pubkey;

        remote_pkg.host = this->conn->hostName;
        remote_pkg.port = QString("%1").arg(this->conn->port);
        remote_pkg.username = this->conn->userName;
        remote_pkg.password = this->conn->password;
        remote_pkg.pubkey = this->conn->pubkey;

        this->slot_new_upload_requested(local_pkg, remote_pkg);
    }
}

void RemoteView::slot_new_download_requested(TaskPackage local_pkg, TaskPackage remote_pkg)
{
    qDebug() <<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;
    // RemoteView *remote_view = this ;
        
    ProgressDialog *pdlg = new ProgressDialog(0);
    // src is remote file , dest if localfile 
    pdlg->set_transfer_info(remote_pkg, local_pkg);
    QObject::connect(pdlg, SIGNAL(transfer_finished(int, QString)),
                     this, SLOT(slot_transfer_finished(int, QString)));
    this->main_mdi_area->addSubWindow(pdlg);
    pdlg->show();
    this->own_progress_dialog = pdlg;
}
void RemoteView::slot_new_download_requested(TaskPackage remote_pkg) 
{
    qDebug() <<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;
    
    QString local_file_path;
    // RemoteView *remote_view = this;
    TaskPackage local_pkg(PROTO_FILE);
    
    local_file_path = this->local_view->get_selected_directory();
    
    qDebug()<<local_file_path;
    if (local_file_path.length() == 0 
        || !QFileInfo(local_file_path).isDir()) {
        //        || !is_dir(GlobalOption::instance()->locale_codec->fromUnicode(local_file_path).data())) {
        qDebug()<<" selected a local file directory  please";
        QMessageBox::critical(this, tr("waring..."), tr("you should selecte a local file directory."));
    } else {
        local_pkg.files<<local_file_path;
        this->slot_new_download_requested(local_pkg, remote_pkg);
    }
}

void RemoteView::slot_transfer_finished(int status, QString errorString)
{
    qDebug() <<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__; 
    RemoteView *remote_view = this ;
    
    ProgressDialog *pdlg = (ProgressDialog*)sender();

    if (status == 0 || status ==3) {
        //TODO 通知UI更新目录结构,在某些情况下会导致左侧树目录变空。
        //int transfer_type = pdlg->get_transfer_type();
        //if ( transfer_type == TransferThread::TRANSFER_GET )
        {
            this->local_view->update_layout();
        }
        //else if ( transfer_type == TransferThread::TRANSFER_PUT )
        {
            remote_view->update_layout();
        }
        //else
        {
            // xxxxx: 没有预期到的错误
            //assert ( 1== 2 );
        }
    } else if (status != 0 && status != 3) {
        QString errmsg = QString(errorString).arg(status);
        if (errmsg.length() < 50) errmsg = errmsg.leftJustified(50);
        QMessageBox::critical(this, QString(tr("Error: ")), errmsg);
    }
    this->main_mdi_area->removeSubWindow(pdlg->parentWidget());

    delete pdlg;
    this->own_progress_dialog = 0;
    //     remote_view->slot_leave_remote_dir_retrive_loop();
}

/**
 *
 * index 是proxy的index 
 */
void RemoteView::slot_dir_tree_item_clicked(const QModelIndex & index)
{
    qDebug() <<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;
    QString file_path;

    remote_dir_model->slot_remote_dir_node_clicked(this->m_treeProxyModel->mapToSource(index));
    
    // file_path = this->m_treeProxyModel->filePath(index);
    // this->uiw->tableView->setRootIndex(this->m_tableProxyModel->index(file_path));
    // for (int i = 0 ; i < this->m_tableProxyModel->rowCount(this->m_tableProxyModel->index(file_path)); i ++ ) {
    //     this->uiw->tableView->setRowHeight(i, this->table_row_height);
    // }
    this->uiw->tableView->resizeColumnToContents(0);
}

void RemoteView::slot_dir_file_view_double_clicked(const QModelIndex & index)
{
    Q_UNUSED(index);
    qDebug() <<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;
    //TODO if the clicked item is direcotry ,
    //expand left tree dir and update right table view
    // got the file path , tell tree ' model , then expand it
    //文件列表中的双击事件
    //1。　本地主机，如果是目录，则打开这个目录，如果是文件，则使用本机的程序打开这个文件
    //2。对于远程主机，　如果是目录，则打开这个目录，如果是文件，则提示是否要下载它(或者也可以直接打开这个文件）。
    QString file_path;
    // if (this->m_tableProxyModel->isDir(index)) {
    //     this->uiw->treeView->expand(this->m_treeProxyModel->index(this->m_tableProxyModel->filePath(index)).parent());
    //     this->uiw->treeView->expand(this->m_treeProxyModel->index(this->m_tableProxyModel->filePath(index)));
    //     this->slot_dir_tree_item_clicked(this->m_treeProxyModel->index(this->m_tableProxyModel->filePath(index)));
    //     this->uiw->treeView->selectionModel()->clearSelection();
    //     this->uiw->treeView->selectionModel()->select(this->m_treeProxyModel->index(this->m_tableProxyModel->filePath(index)) , QItemSelectionModel::Select);
    // } else if (this->m_tableProxyModel->isSymLink(index)) {
    //     QModelIndex idx = this->m_tableProxyModel->mapToSource(index);
    //     NetDirNode *node_item = (NetDirNode*)idx.internalPointer();
    //     q_debug()<<node_item->fullPath;
    //     this->remote_dir_model->slot_execute_command(node_item, idx.internalPointer(),
    //                                                  SSH2_FXP_REALPATH, QString(""));
    // } else {
    //     q_debug()<<"double clicked a regular file, no op now, only now";
    // }
}

void RemoteView::slot_drag_ready()
{
    //qDebug() <<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;
    //TODO 处理从树目录及文件列表视图中的情况
    //QAbstractItemView * sender_view = qobject_cast<QAbstractItemView*>(sender());
    QString  temp_file_path, remote_file_name;
    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;
    
    //这个视图中所选择的目录优先，如果没有则查找左侧目录树是是否有选择的目录，如果再找不到，则使用右侧表视图的根
    // QItemSelectionModel *ism = this->uiw->tableView->selectionModel();
    // QModelIndexList mil = ism->selectedIndexes();
    // if (mil.count() == 0) {
    //     ism = this->uiw->treeView->selectionModel();
    //     mil = ism->selectedIndexes();
    // }

    // TaskPackage tpkg(PROTO_SFTP);
    
    // tpkg.host = this->conn->hostName;
    // tpkg.port = QString("%1").arg(this->conn->port);
    // tpkg.username = this->conn->userName;
    // tpkg.password = this->conn->password;
    // tpkg.pubkey = this->conn->pubkey;


    // for (int i = 0 ; i< mil.count() ;i += this->remote_dir_model->columnCount()) {
    //     QModelIndex midx = mil.at(i);
    //     temp_file_path = (qobject_cast<RemoteDirModel*>(this->remote_dir_model))
    //         ->filePath(this->m_tableProxyModel->mapToSource(midx) );
    //     tpkg.files<<temp_file_path;
    // }
    
    // mimeData->setData("application/task-package", tpkg.toRawData());
    // drag->setMimeData(mimeData);
    
    // if (mil.count() > 0) {
    //     Qt::DropAction dropAction = drag->exec(Qt::CopyAction | Qt::MoveAction);
    //     Q_UNUSED(dropAction);
    // }
    // qDebug()<<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__<<"drag->exec returned";
}

bool RemoteView::slot_drop_mime_data(const QMimeData *data, Qt::DropAction action,
                                     int row, int column, const QModelIndex &parent)
{
    qDebug()<<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;
    Q_UNUSED(action);
    Q_UNUSED(row);
    Q_UNUSED(column);
    
    TaskPackage local_pkg(PROTO_FILE);
    TaskPackage remote_pkg(PROTO_SFTP);
   
    NetDirNode *aim_item = static_cast<NetDirNode*>(parent.internalPointer());        
    QString remote_file_name = aim_item->fullPath;

    remote_pkg.files<<remote_file_name;

    // remote_pkg.host = this->host_name;
    // remote_pkg.username = this->user_name;
    // remote_pkg.password = this->password;
    // remote_pkg.port = QString("%1").arg(this->port);
    // remote_pkg.pubkey = this->pubkey;    

    remote_pkg.host = this->conn->hostName;
    remote_pkg.port = QString("%1").arg(this->conn->port);
    remote_pkg.username = this->conn->userName;
    remote_pkg.password = this->conn->password;
    remote_pkg.pubkey = this->conn->pubkey;

    if (data->hasFormat("application/task-package")) {
        local_pkg = TaskPackage::fromRawData(data->data("application/task-package"));
        if (local_pkg.isValid(local_pkg)) {
            // TODO 两个sftp服务器间拖放的情况
            this->slot_new_upload_requested(local_pkg, remote_pkg);
        }
    } else if (data->hasFormat("text/uri-list")) {
        // from localview
        QList<QUrl> files = data->urls();
        if (files.count() == 0) {
            // return false;
            assert(0);
        } else {
            for (int i = 0 ; i < files.count(); i++) {
                QString path = files.at(i).path();
                #ifdef WIN32
                // because on win32, now path=/C:/xxxxx
                path = path.right(path.length() - 1);
                #endif
                local_pkg.files<<path;
            }
            this->slot_new_upload_requested(local_pkg, remote_pkg);
        }        
    } else {
        qDebug()<<"invalid mime type:"<<data->formats();
    }
    qDebug()<<"drop mime data processed";
    
    return true;
}

void RemoteView::slot_show_hidden(bool show)
{
    Q_UNUSED(show);
    // if (show) {
    //     m_tableProxyModel->setFilter(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot);
    //     m_treeProxyModel->setFilter(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot);
    // } else {
    //     m_tableProxyModel->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
    //     m_treeProxyModel->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
    // }
}

void RemoteView::onUpdateEntriesStatus()
{
    q_debug()<<"";
}


void RemoteView::slot_operation_triggered(QString text)
{
    if (this->m_operationLogModel == NULL) {
        this->m_operationLogModel = new QStringListModel();
    }
    int rc = this->m_operationLogModel->rowCount();
    this->m_operationLogModel->insertRows(rc, 1, QModelIndex());
    QModelIndex useIndex = this->m_operationLogModel->index(rc, 0, QModelIndex());
    this->m_operationLogModel->setData(useIndex, QVariant(text));
    this->uiw->listView->scrollTo(useIndex);

    if (rc > 300) {
        useIndex = this->m_operationLogModel->index(0, 0, QModelIndex());
        this->m_operationLogModel->removeRows(0, 1, QModelIndex());
    }
}


void RemoteView::encryption_focus_label_double_clicked()
{
    //qDebug()<<__FILE__<<":"<<__LINE__;
    EncryptionDetailDialog *enc_dlg = 0;
    char **server_info, **pptr, *p;
    int sftp_version;
    
	server_info = (char**)malloc(10 * sizeof(char*));
	for (int i = 0; i < 10; i++) {
		server_info[i] = (char*)malloc(512);
	}

    pptr = server_info = libssh2_session_get_remote_info(this->conn->sess, server_info);
    sftp_version = libssh2_sftp_get_version(this->ssh2_sftp); 

    enc_dlg = new EncryptionDetailDialog(server_info, this);
    enc_dlg->exec();

    //if(server_info != NULL) free(server_info);
    delete enc_dlg;

    // while (*pptr != NULL) {
    //     free(*pptr);
    //     pptr ++;
    // }
	for (int i = 0; i < 10; i++) {
		p = server_info[i];
		free(p);
	}

    free(server_info);
}

void RemoteView::host_info_focus_label_double_clicked()
{
    HostInfoDetailFocusLabel *hi_label = (HostInfoDetailFocusLabel*)sender();
    qDebug()<<"hehe"<<hi_label;

    LIBSSH2_CHANNEL *ssh2_channel = 0;
    int rv = -1;
    char buff[1024] ;
    QString evn_output;
    QString uname_output;
    const char *cmd = "uname -a";

    ssh2_channel = libssh2_channel_open_session(this->conn->sess);
    //libssh2_channel_set_blocking(ssh2_channel, 1);
    rv = libssh2_channel_exec(ssh2_channel, cmd);
    qDebug()<<"SSH2 exec: "<<rv;
  
    memset(buff, 0, sizeof(buff));
    while ((rv = libssh2_channel_read(ssh2_channel, buff, 1000)) > 0) {
        qDebug()<<"Channel read: "<<rv<<" -->"<<buff;
        uname_output += QString(buff);
        memset(buff, 0, sizeof(buff));
    }

    libssh2_channel_close(ssh2_channel);
    libssh2_channel_free(ssh2_channel);
    
    qDebug()<<"Host Info: "<<uname_output;
    hi_label->setToolTip(uname_output);
    
    QDialog *dlg = new QDialog(this);
    dlg->setFixedWidth(400);
    dlg->setFixedHeight(100);
    QLabel * label = new QLabel("", dlg);
    label->setWordWrap(true);
    label->setText(uname_output);
    // dlg->layout()->addWidget(label);
    dlg->exec();
    delete dlg;
}

void RemoteView::onDirectoryLoaded(const QString &path)
{
    q_debug()<<"Warning: base call."<<path;
}

void RemoteView::slot_dir_nav_go_home()
{
    q_debug()<<"";
    // check if current index is home index
    // if no, callepse all and expand to home
    // tell dir nav instance the home path

    // QModelIndex sourceIndex = this->uiw->tableView->rootIndex();
    // QString rootPath = this->model->filePath(sourceIndex);
    // if (rootPath != QDir::homePath()) {
    //     this->uiw->treeView->collapseAll();
    //     this->expand_to_home_directory(QModelIndex(), 1);
    //     this->uiw->tableView->setRootIndex(this->model->index(QDir::homePath()));
    // }
    // this->uiw->widget->onSetHome(QDir::homePath());
}

void RemoteView::slot_dir_nav_prefix_changed(const QString &prefix)
{
    Q_UNUSED(prefix);
    // q_debug()<<""<<prefix;
    // QStringList matches;
    // QModelIndex sourceIndex = this->model->index(prefix);
    // QModelIndex currIndex;

    // if (prefix == "") {
    //     sourceIndex = this->model->index(0, 0, QModelIndex());
    // } else if (!sourceIndex.isValid()) {
    //     int pos = prefix.lastIndexOf('/');
    //     if (pos == -1) {
    //         pos = prefix.lastIndexOf('\\');
    //     }
    //     if (pos == -1) {
    //         // how deal this case
    //         Q_ASSERT(pos >= 0);
    //     }
    //     sourceIndex = this->model->index(prefix.left(prefix.length() - pos));
    // }
    // if (sourceIndex.isValid()) {
    //     if (this->model->canFetchMore(sourceIndex)) {
    //         while (this->model->canFetchMore(sourceIndex)) {
    //             this->is_dir_complete_request = true;
    //             this->dir_complete_request_prefix = prefix;
    //             this->model->fetchMore(sourceIndex);
    //         }
    //         // sience qt < 4.7 has no directoryLoaded signal, so we should execute now if > 0
    //         if (strcmp(qVersion(), "4.7.0") < 0) {
    //             // QTimer::singleShot(this, SLOT(
    //         }
    //     } else {
    //         int rc = this->model->rowCount(sourceIndex);
    //         q_debug()<<"lazy load dir rows:"<<rc;
    //         for (int i = rc - 1; i >= 0; --i) {
    //             currIndex = this->model->index(i, 0, sourceIndex);
    //             if (this->model->isDir(currIndex)) {
    //                 matches << this->model->filePath(currIndex);
    //             }
    //         }
    //         this->uiw->widget->onSetCompleteList(prefix, matches);
    //     }
    // } else {
    //     // any operation for this case???
    //     q_debug()<<"any operation for this case???"<<prefix;
    // }
}

void RemoteView::slot_dir_nav_input_comfirmed(const QString &prefix)
{
    Q_UNUSED(prefix);
    q_debug()<<"";

    // QModelIndex sourceIndex = this->model->index(prefix);
    // QModelIndex currIndex = this->uiw->tableView->rootIndex();
    // QString currPath = this->model->filePath(currIndex);

    // if (!sourceIndex.isValid()) {
    //     q_debug()<<"directory not found!!!"<<prefix;
    //     // TODO, show status???
    //     this->status_bar->showMessage(tr("Directory not found: %1").arg(prefix));
    //     return;
    // }

    // if (currPath != prefix) {
    //     this->uiw->treeView->collapseAll();
    //     this->expand_to_directory(prefix, 1);
    //     this->uiw->tableView->setRootIndex(this->model->index(prefix));
    // }
}

void RemoteView::slot_icon_size_changed(int value)
{
    this->uiw->listView_2->setGridSize(QSize(value, value));
}

void RemoteView::setFileListViewMode(int mode)
{
    if (mode == GlobalOption::FLV_BOTH_VIEW) {
        // for debug purpose
        this->uiw->tableView->setVisible(true);
        this->uiw->listView_2->setVisible(true);
    } else if (mode == GlobalOption::FLV_LARGE_ICON) {
        this->uiw->tableView->setVisible(false);
        this->uiw->listView_2->setVisible(true);
        this->slot_icon_size_changed(96);
        this->uiw->listView_2->setViewMode(QListView::IconMode);
    } else if (mode == GlobalOption::FLV_SMALL_ICON) {
        this->uiw->tableView->setVisible(false);
        this->uiw->listView_2->setVisible(true);
        this->slot_icon_size_changed(48);
        this->uiw->listView_2->setViewMode(QListView::IconMode);
    } else if (mode == GlobalOption::FLV_LIST) {
        this->uiw->tableView->setVisible(false);
        this->uiw->listView_2->setVisible(true);
        this->slot_icon_size_changed(32);
        this->uiw->listView_2->setViewMode(QListView::ListMode);
    } else if (mode == GlobalOption::FLV_DETAIL) {
        this->uiw->tableView->setVisible(true);
        this->uiw->listView_2->setVisible(false);
    } else {
        Q_ASSERT(1 == 2);
    }    
}
