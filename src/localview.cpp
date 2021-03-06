// localview.cpp --- 
// 
// Author: liuguangzhao
// Copyright (C) 2007-2010 liuguangzhao@users.sf.net
// URL: http://www.qtchina.net http://nullget.sourceforge.net
// Created: 2008-05-31 15:26:15 +0800
// Version: $Id$
// 

#include <QtCore>
#include <QtGui>

#include "utils.h"
#include "remoteview.h"
#include "localview.h"
#include "localfilesystemmodel.h"
#include "globaloption.h"
#include "fileproperties.h"
#include "progressdialog.h"

#include "ui_localview.h"

LocalView::LocalView(QMdiArea *main_mdi_area, QWidget *parent)
    : QWidget(parent)
    , uiw(new Ui::LocalView())
    , main_mdi_area(main_mdi_area)
    , own_progress_dialog(NULL)
{
    this->uiw->setupUi(this);
    this->setObjectName("LocalFileSystemView");
    ////
    this->status_bar = new QStatusBar();
    this->layout()->addWidget(this->status_bar);
    this->status_bar->showMessage(tr("Ready"));
    this->entriesLabel = new QLabel(tr("Entries label"), this);
    this->status_bar->addPermanentWidget(this->entriesLabel);
    this->entriesLabel->setTextInteractionFlags(this->entriesLabel->textInteractionFlags() 
                                                | Qt::TextSelectableByMouse);

    QLabel *tmpLabel = new QLabel(tr("System"), this);
    this->status_bar->addPermanentWidget(tmpLabel);
#if defined(Q_OS_WIN)
    tmpLabel->setPixmap(QPixmap(":/icons/os/windows.png").scaled(22, 22));
    tmpLabel->setToolTip(tr("Running Windows OS."));
#elif defined(Q_OS_MAC)
    tmpLabel->setPixmap(QPixmap(":/icons/os/osx.png").scaled(22, 22));
    tmpLabel->setToolTip(tr("Running Mac OS X."));
#else
    tmpLabel->setPixmap(QPixmap(":/icons/os/tux.png").scaled(22, 22));
    tmpLabel->setToolTip(tr("Running Linux/Unix Like OS."));
#endif
    
    ////
    this->model2 = new LocalFileSystemModel();
    // this->model = new QFileSystemModel(); // temp comment for test
    this->model = this->model2;
    QObject::connect(model, SIGNAL(directoryLoaded(const QString &)),
                     this, SLOT(onDirectoryLoaded(const QString &)));
    QObject::connect(model, SIGNAL(fileRenamed(const QString &, const QString &, const QString &)),
                     this, SLOT(onFileRenamed(const QString &, const QString &, const QString &)));
    QObject::connect(model, SIGNAL(rootPathChanged(const QString &)),
                     this, SLOT(onRootPathChanged(const QString &)));

    QObject::connect(this->model2,
                     SIGNAL(sig_drop_mime_data(const QMimeData *, Qt::DropAction, int, int, const QModelIndex &)),
                     this, SLOT(slot_drop_mime_data(const QMimeData *, Qt::DropAction, int, int, const QModelIndex &)));
    

    model->setRootPath(""); // on widows this can list all drivers

    // model->setFilter(QDir::AllEntries|QDir::Hidden|QDir::NoDotAndDotDot);
    this->dir_file_model = new LocalDirSortFilterModel();
    this->dir_file_model->setSourceModel(model);

    
    this->uiw->treeView->setModel(this->dir_file_model);
    this->uiw->treeView->setRootIndex(this->dir_file_model->index(""));
    // this->uiw->treeView->setColumnHidden(1, true);
    this->uiw->treeView->setColumnWidth(1, 0);
    this->uiw->treeView->setColumnHidden(2, true);
    this->uiw->treeView->setColumnHidden(3, true);
    this->uiw->treeView->setColumnWidth(0, this->uiw->treeView->columnWidth(0) * 2);    
    this->expand_to_home_directory(this->uiw->treeView->rootIndex(), 1);
    // this->uiw->treeView->expand(this->dir_file_model->index("/home/gzleo"));
  
    this->init_local_dir_tree_context_menu();
    this->uiw->treeView->setAnimated(true);
  
    this->uiw->tableView->setModel(this->model);
    this->uiw->tableView->setRootIndex(this->model->index(QDir::homePath()));
    this->uiw->tableView->verticalHeader()->setVisible(false);

    //change row height of table 
    if (this->model->rowCount(this->model->index(QDir::homePath())) > 0) {
        this->table_row_height = this->uiw->tableView->rowHeight(0) * 2 / 3;
    } else {
        this->table_row_height = 20;
    }
    for (int i = 0; i < this->model->rowCount(this->model->index(QDir::homePath())); i ++) {
        this->uiw->tableView->setRowHeight(i, this->table_row_height);
    }
  
    this->uiw->tableView->resizeColumnToContents(0);

    // this->uiw->treeView->setAcceptDrops(true);
    // // this->uiw->treeView->setDragEnabled(false);
    // this->uiw->treeView->setDragEnabled(true);
    // this->uiw->treeView->setDropIndicatorShown(true);
    // // this->uiw->treeView->setDragDropMode(QAbstractItemView::DropOnly);
    // this->uiw->treeView->setDragDropMode(QAbstractItemView::DragDrop);
    
    // this->uiw->tableView->setAcceptDrops(true);
    // this->uiw->tableView->setDragEnabled(false);
    // this->uiw->tableView->setDropIndicatorShown(false);
    // this->uiw->tableView->setDragDropMode(QAbstractItemView::DragDrop);
    // this->uiw->tableView->setDragDropOverwriteMode(true);

    /////
    QObject::connect(this->uiw->treeView, SIGNAL(clicked(const QModelIndex &)),
                     this, SLOT(slot_dir_tree_item_clicked(const QModelIndex &)));
    QObject::connect(this->uiw->tableView, SIGNAL(doubleClicked(const QModelIndex &)),
                     this, SLOT(slot_dir_file_view_double_clicked(const QModelIndex &)));
    QObject::connect(this->uiw->listView, SIGNAL(doubleClicked(const QModelIndex &)),
                     this, SLOT(slot_dir_file_view_double_clicked(const QModelIndex &)));

    // list view of icon mode
    this->uiw->listView->setModel(this->model);
    this->uiw->listView->setRootIndex(this->model->index(QDir::homePath()));
    this->uiw->listView->setViewMode(QListView::IconMode);
    this->uiw->listView->setGridSize(QSize(80, 80));
    
    ////////ui area custom
    this->uiw->splitter->setStretchFactor(0, 1);
    this->uiw->splitter->setStretchFactor(1, 2);
    //this->uiw->listView->setVisible(false);    //暂时没有功能在里面先隐藏掉

    // dir navbar
    this->is_dir_complete_request = false;
    // this->dir_complete_request_prefix = "";
    QObject::connect(this->uiw->widget, SIGNAL(goHome()),
                     this, SLOT(slot_dir_nav_go_home()));
    QObject::connect(this->uiw->widget, SIGNAL(dirPrefixChanged(const QString &)),
                     this, SLOT(slot_dir_nav_prefix_changed(const QString &)));
    QObject::connect(this->uiw->widget, SIGNAL(dirInputConfirmed(const QString &)),
                     this, SLOT(slot_dir_nav_input_comfirmed(const QString &)));
    QObject::connect(this->uiw->widget, SIGNAL(iconSizeChanged(int)),
                     this, SLOT(slot_icon_size_changed(int)));
    this->uiw->widget->onSetHome(QDir::homePath());

    this->uiw->listView->setSelectionModel(this->uiw->tableView->selectionModel());
    this->setFileListViewMode(GlobalOption::FLV_DETAIL);

    //TODO localview 标题格式: Local(主机名) - 当前所在目录名
    //TODO remoteview 标题格式: user@hostname - 当前所在目录名
    //TODO 状态栏: 信息格式:  n entries (m hidden entries) . --- 与remoteview相同
}


LocalView::~LocalView()
{
}

void LocalView::init_local_dir_tree_context_menu()
{
    this->local_dir_tree_context_menu = new QMenu();

    QAction *action = new QAction(tr("Upload"), 0);

    this->local_dir_tree_context_menu->addAction(action);

    QObject::connect(action, SIGNAL(triggered()), this, SLOT(slot_local_new_upload_requested()));

    action = new QAction("", 0);
    action->setSeparator(true);
    this->local_dir_tree_context_menu->addAction(action);
    
    ////reresh action
    action = new QAction(tr("Refresh"), 0);
    this->local_dir_tree_context_menu->addAction(action);
    
    QObject::connect(action, SIGNAL(triggered()),
                     this, SLOT(slot_refresh_directory_tree()));
                       
    action = new QAction(tr("Properties..."), 0);
    this->local_dir_tree_context_menu->addAction(action);
    QObject::connect(action,SIGNAL(triggered()), this, SLOT(slot_show_properties()));
    //////
    action = new QAction(tr("Show Hidden"), 0);
    action->setCheckable(true);
    this->local_dir_tree_context_menu->addAction(action);
    QObject::connect(action, SIGNAL(toggled(bool)), this, SLOT(slot_show_hidden(bool)));
    action = new QAction("", 0);
    action->setSeparator(true);
    this->local_dir_tree_context_menu->addAction(action);

    action = new QAction(tr("Copy &Path"), 0);
    this->local_dir_tree_context_menu->addAction(action);
    QObject::connect(action, SIGNAL(triggered()), this, SLOT(slot_copy_path_url()));
    
    action = new QAction(tr("Create directory..."), 0);
    this->local_dir_tree_context_menu->addAction(action);
    QObject::connect(action, SIGNAL(triggered()), this, SLOT(slot_mkdir()));

    action = new QAction(tr("Delete directory"), 0);
    this->local_dir_tree_context_menu->addAction(action);
    QObject::connect(action, SIGNAL(triggered()), this, SLOT(slot_rmdir()));
    action = new QAction(tr("Remove file"), 0);
    this->local_dir_tree_context_menu->addAction(action);
    QObject::connect(action, SIGNAL(triggered()), this, SLOT(slot_remove()));

    action = new QAction("", 0);
    action->setSeparator(true);
    this->local_dir_tree_context_menu->addAction(action);
  
    //递归删除目录，删除文件的用户功能按钮
    // action = new QAction(tr("Remove recursively !!!"), 0);
    // this->local_dir_tree_context_menu->addAction(action);
    // QObject::connect(action, SIGNAL(triggered()), this, SLOT(rm_file_or_directory_recursively()));

    // action = new QAction("", 0);
    // action->setSeparator(true);
    // this->local_dir_tree_context_menu->addAction(action);

    action = new QAction(tr("Rename ..."), 0);
    this->local_dir_tree_context_menu->addAction(action);
    QObject::connect(action, SIGNAL(triggered()), this, SLOT(slot_rename()));
    
    QObject::connect(this->uiw->treeView, SIGNAL(customContextMenuRequested(const QPoint &)),
                     this, SLOT(slot_local_dir_tree_context_menu_request(const QPoint &)));
    QObject::connect(this->uiw->tableView, SIGNAL(customContextMenuRequested(const QPoint &)),
                     this, SLOT(slot_local_dir_tree_context_menu_request(const QPoint &)));
    QObject::connect(this->uiw->listView, SIGNAL(customContextMenuRequested(const QPoint &)),
                     this, SLOT(slot_local_dir_tree_context_menu_request(const QPoint &)));
}

// 可以写成一个通用的expand_to_directory(QString fullPath, int level);
// 仅会被调用一次，在该实例的构造函数中
void LocalView::expand_to_home_directory(QModelIndex parent_model, int level)
{
    Q_UNUSED(parent_model);
    QString homePath = QDir::homePath();
    QStringList homePathParts = QDir::homePath().split('/');
    // qDebug()<<home_path_grade<<level<<row_cnt;
    QStringList stepPathParts;
    QString tmpPath;
    QModelIndex currIndex;

    // windows fix case: C:/abcd/efg/hi
    bool unixRootFix = true;
    if (homePath.length() > 1 && homePath.at(1) == ':') {
        unixRootFix = false;
    }
    
    for (int i = 0; i < homePathParts.count(); i++) {
        stepPathParts << homePathParts.at(i);
        tmpPath = (unixRootFix ? QString("/") : QString()) + stepPathParts.join("/");
        /// qDebug()<<tmpPath<<stepPathParts;
        currIndex = this->dir_file_model->index(tmpPath);
        this->uiw->treeView->expand(currIndex);
    }
    if (level == 1) {
        this->uiw->treeView->scrollTo(currIndex);
    }
    //qDebug()<<" root row count:"<< row_cnt ;
}

void LocalView::expand_to_directory(QString path, int level)
{
    QString homePath = path;
    QStringList homePathParts = homePath.split('/');
    // qDebug()<<home_path_grade<<level<<row_cnt;
    QStringList stepPathParts;
    QString tmpPath;
    QModelIndex curr_model;

    // windows fix case: C:/abcd/efg/hi
    bool unixRootFix = true;
    if (homePath.length() > 1 && homePath.at(1) == ':') {
        unixRootFix = false;
    }
    
    for (int i = 0; i < homePathParts.count(); i++) {
        stepPathParts << homePathParts.at(i);
        tmpPath = (unixRootFix ? QString("/") : QString()) + stepPathParts.join("/");
        /// qDebug()<<tmpPath<<stepPathParts;
        curr_model = this->dir_file_model->index(tmpPath);
        this->uiw->treeView->expand(curr_model);
    }
    if (level == 1) {
        this->uiw->treeView->scrollTo(curr_model);
    }
    //qDebug()<<" root row count:"<< row_cnt ;
}


void LocalView::slot_local_dir_tree_context_menu_request(const QPoint & pos)
{
    //qDebug() <<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;
    this->curr_item_view = static_cast<QAbstractItemView*>(sender());
    QPoint real_pos = this->curr_item_view->mapToGlobal(pos);
    real_pos = QPoint(real_pos.x()+2, real_pos.y() + 32);
    this->local_dir_tree_context_menu->popup(real_pos);
    
}

// can not support recursive selected now.
void LocalView::slot_local_new_upload_requested()
{
    //qDebug() <<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;
    TaskPackage pkg(PROTO_FILE);
    QString local_file_name;
    QByteArray ba;

    QItemSelectionModel *ism = this->curr_item_view->selectionModel();
    QModelIndex cidx, idx, pidx;

    cidx = ism->currentIndex();
    pidx = cidx.parent();

    for (int i = ism->model()->rowCount(pidx) - 1 ; i >= 0 ; --i) {
        if (ism->isRowSelected(i, pidx)) {
            QModelIndex midx = idx = ism->model()->index(i, 0, pidx);
            if (this->curr_item_view == this->uiw->treeView) {
                midx = this->dir_file_model->mapToSource(midx);
            }
            qDebug()<<this->model->fileName(midx);
            qDebug()<<this->model->filePath(midx);
            local_file_name = this->model->filePath(midx);
            pkg.files<<local_file_name;
        }
    }
    
    // QModelIndexList mil = ism->selectedIndexes(); // TODO should fix win x64

    // for (int i = 0 ; i < mil.count() ; i += this->curr_item_view->model()->columnCount(QModelIndex())) {
    //     QModelIndex midx = mil.at(i);
    //     if (this->curr_item_view==this->uiw->treeView) {
    //         midx = this->dir_file_model->mapToSource(midx);
    //     }
    //     qDebug()<<this->model->fileName(midx);
    //     qDebug()<<this->model->filePath(midx);
    //     local_file_name = this->model->filePath(midx);
    //     pkg.files<<local_file_name;
    // }
    emit new_upload_requested(pkg);
}

void LocalView::slot_local_new_download_requested(const TaskPackage &local_pkg, const TaskPackage &remote_pkg)
{
    ProgressDialog *pdlg = new ProgressDialog(0);
    // src is remote file , dest if localfile 
    pdlg->set_transfer_info(remote_pkg, local_pkg);
    QObject::connect(pdlg, SIGNAL(transfer_finished(int, QString)),
                     this, SLOT(slot_transfer_finished(int, QString)));
    this->main_mdi_area->addSubWindow(pdlg);
    pdlg->show();
    this->own_progress_dialog = pdlg;
}

void LocalView::slot_transfer_finished(int status, QString errorString)
{
    qDebug() <<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__<<status; 
    // SFTPView *remote_view = this;
    
    ProgressDialog *pdlg = (ProgressDialog*)sender();

    this->main_mdi_area->removeSubWindow(pdlg->parentWidget());

    delete pdlg;
    this->own_progress_dialog = NULL;

    if (status == 0 // || status == 3
        ) {
        //TODO 通知UI更新目录结构,在某些情况下会导致左侧树目录变空。
        //int transfer_type = pdlg->get_transfer_type();
        //if ( transfer_type == TransferThread::TRANSFER_GET )
        {
            // this->local_view->update_layout();
            // local 不需要吧，这个能自动更新的？？？
            this->update_layout();
        }
        //else if ( transfer_type == TransferThread::TRANSFER_PUT )
        {
            // remote_view->update_layout();
        }
        //else
        {
            // xxxxx: 没有预期到的错误
            //assert ( 1== 2 );
        }
    } else if (status == 52 // Transportor::ERROR_CANCEL
               ) {
        // user cancel, show nothing
    } else if (status != 0 // && status != 3
               ) {
        QString errmsg = QString(errorString + " Code: %1").arg(status);
        if (errmsg.length() < 50) errmsg = errmsg.leftJustified(50);
        QMessageBox::critical(this, QString(tr("Error: ")), errmsg);
    } else {
        Q_ASSERT(1 == 2);
    }
    // remote_view->slot_leave_remote_dir_retrive_loop();
}

QString LocalView::get_selected_directory()
{
    qDebug() <<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;
    
    QString local_path;
    QItemSelectionModel *ism = this->uiw->treeView->selectionModel();
    QModelIndex cidx, idx;

    if (ism == 0) {        
        return QString();
    }

    if (!ism->hasSelection()) {
        qDebug()<<"why no tree selection???";
        return QString();
    }

    // QModelIndexList mil = ism->selectedIndexes();
    // if (mil.count() == 0) {
    //     return QString();
    // }
    
    //qDebug() << mil ;
    //qDebug() << model->fileName ( mil.at ( 0 ) );
    //qDebug() << model->filePath ( mil.at ( 0 ) );

    cidx = ism->currentIndex();
    if (!ism->isSelected(cidx)) {
        // so currentIndex is not always a selected index !!!!!!
        qDebug()<<"Why current index is not a selected index???";
    }
    idx = ism->model()->index(cidx.row(), 0, cidx.parent());

    // QString local_file = this->dir_file_model->filePath(mil.at(0));
    // local_path = this->dir_file_model->filePath(mil.at(0));

    QString local_file = this->dir_file_model->filePath(idx);
    local_path = this->dir_file_model->filePath(idx);

    return local_path;
}

void LocalView::slot_refresh_directory_tree()
{
    //qDebug() <<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;

    QItemSelectionModel *ism = this->uiw->treeView->selectionModel();
    QModelIndex cidx, idx;

    if (ism != 0) {
        if (ism->hasSelection()) {
            cidx = ism->currentIndex();
            idx = ism->model()->index(cidx.row(), 0, cidx.parent());
            // QModelIndex origIndex = this->dir_file_model->mapToSource(mil.at(0));
            QModelIndex origIndex = this->dir_file_model->mapToSource(idx);
        }
        // QModelIndexList mil = ism->selectedIndexes();
        // if (mil.count() > 0) {
        //     // model->refresh(mil.at(0));
        //     QModelIndex origIndex = this->dir_file_model->mapToSource(mil.at(0));
        //     q_debug()<<mil.at(0)<<origIndex;
        //     // model->refresh(origIndex);
        // }
    }
    this->dir_file_model->refresh(this->uiw->tableView->rootIndex());
}
void LocalView::update_layout()
{
    //qDebug() <<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;
    //     this->dir_file_model->refresh( this->uiw->tableView->rootIndex());
    //     this->model->refresh(QModelIndex());
    this->slot_refresh_directory_tree();
}

void LocalView::closeEvent(QCloseEvent *event)
{
    //qDebug() <<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;
    event->ignore();
    //this->setVisible(false); 
    // QMessageBox::information(this, tr("Attemp to close this window?"), tr("Close this window is not needed."));
    // 把这个窗口最小化是不是好些。
    this->showMinimized();
}

void LocalView::keypressEvent(QKeyEvent *e)
{
    QWidget::keyPressEvent(e);
    return;
   
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

void LocalView::slot_dir_tree_item_clicked(const QModelIndex &index)
{
    //qDebug() <<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;
    QString file_path;
    
    file_path = this->dir_file_model->filePath(index);
    this->uiw->tableView->setRootIndex(this->model->index(file_path));
    for (int i = 0 ; i < this->model->rowCount(this->model->index(file_path)); i ++)
        this->uiw->tableView->setRowHeight(i, this->table_row_height);
    this->uiw->tableView->resizeColumnToContents(0);

    this->uiw->listView->setRootIndex(this->model->index(file_path));

    this->uiw->widget->onNavToPath(file_path);
    this->onUpdateEntriesStatus();
}

void LocalView::slot_dir_file_view_double_clicked(const QModelIndex &index)
{
    //qDebug() <<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;
    //TODO if the clicked item is direcotry ,
    //expand left tree dir and update right table view
    // got the file path , tell tree ' model , then expand it
    //文件列表中的双击事件
    //1。 本地主机，如果是目录，则打开这个目录，如果是文件，则使用本机的程序打开这个文件
    //2。 对于远程主机，　如果是目录，则打开这个目录，如果是文件，则提示是否要下载它。
    QString file_path;
    QModelIndex idx, idx2, idx_top_left, idx_bottom_right;
    
    if (this->model->isDir(index)) {
        this->uiw->treeView->expand( this->dir_file_model->index(this->model->filePath(index)).parent());        
        this->uiw->treeView->expand( this->dir_file_model->index(this->model->filePath(index)));
        this->slot_dir_tree_item_clicked(this->dir_file_model->index(this->model->filePath(index)));
        // this->uiw->treeView->selectionModel()->clearSelection();
        // this->uiw->treeView->selectionModel()->select(this->dir_file_model->index(this->model->filePath(index)), QItemSelectionModel::Select);

        idx = this->dir_file_model->index(this->model->filePath(index));
        // qDebug()<<"column count:"<<this->dir_file_model->columnCount(idx);
        idx2 = idx.parent();
        idx_top_left = this->dir_file_model->index(idx.row(), 0, idx2);
        idx_bottom_right = this->dir_file_model->index(idx.row(), 
                                                       this->dir_file_model->columnCount(idx)-1,
                                                       idx2);
        QItemSelection iselect(idx_top_left, idx_bottom_right);
        this->uiw->treeView->selectionModel()->clearSelection();
        this->uiw->treeView->selectionModel()->select(iselect, QItemSelectionModel::Select);
        this->uiw->treeView->selectionModel()->setCurrentIndex(idx_top_left, QItemSelectionModel::Select);
        this->uiw->treeView->setFocus(Qt::ActiveWindowFocusReason);
    } else {
        qDebug()<<" double clicked a regular file , no op now,only now";
    }
}

//accept drop ok now
bool LocalView::slot_drop_mime_data(const QMimeData *data, Qt::DropAction action,
                                     int row, int column, const QModelIndex &parent)
{
    qDebug()<<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;
    Q_UNUSED(row);
    Q_UNUSED(column);
    
    TaskPackage local_pkg(PROTO_FILE);
    TaskPackage remote_pkg(PROTO_SFTP); // MAYBE also PROTO_FILE
   
    QString local_file_name = this->model->filePath(parent);
    local_pkg.files<<local_file_name;

    if (data->hasFormat("application/task-package")) {
        remote_pkg = TaskPackage::fromRawData(data->data("application/task-package"));
        qDebug()<<remote_pkg;
        if (remote_pkg.isValid(remote_pkg)) {
            // fixed 两个sftp服务器间拖放的情况, 也可以，已经完成
            // this->slot_new_upload_requested(local_pkg, remote_pkg);
            this->slot_local_new_download_requested(local_pkg, remote_pkg);
        }
    } else if (data->hasFormat("text/uri-list")) {
        // from native file explore like dolphin. or self?
        remote_pkg.setProtocol(PROTO_FILE);
        QList<QUrl> files = data->urls();
        if (files.count() == 0) {
             return false;
             assert(0);
        } else {
            for (int i = 0 ; i < files.count(); i++) {
                QString path = files.at(i).path();
                #ifdef WIN32
                // because on win32, now path=/C:/xxxxx, should
                path = path.right(path.length() - 1);
                #endif
                remote_pkg.files<<path;
            }
            // this->slot_new_upload_requested(local_pkg, remote_pkg);
            this->slot_local_new_download_requested(local_pkg, remote_pkg);
        }
    } else {
        qDebug()<<"Invalid mime type:"<<data->formats();
    }

    // NetDirNode *aim_item = static_cast<NetDirNode*>(parent.internalPointer());        
    // QString remote_file_name = aim_item->fullPath;

    // remote_pkg.files<<remote_file_name;

    // remote_pkg.host = this->conn->hostName;
    // remote_pkg.port = QString("%1").arg(this->conn->port);
    // remote_pkg.username = this->conn->userName;
    // remote_pkg.password = this->conn->password;
    // remote_pkg.pubkey = this->conn->pubkey;

    // if (data->hasFormat("application/task-package")) {
    //     local_pkg = TaskPackage::fromRawData(data->data("application/task-package"));
    //     qDebug()<<local_pkg;
    //     if (local_pkg.isValid(local_pkg)) {
    //         // fixed 两个sftp服务器间拖放的情况, 也可以，已经完成
    //         this->slot_new_upload_requested(local_pkg, remote_pkg);
    //     }
    // } else if (data->hasFormat("text/uri-list")) {
    //     // from localview
    //     QList<QUrl> files = data->urls();
    //     if (files.count() == 0) {
    //         // return false;
    //         assert(0);
    //     } else {
    //         for (int i = 0 ; i < files.count(); i++) {
    //             QString path = files.at(i).path();
    //             #ifdef WIN32
    //             // because on win32, now path=/C:/xxxxx
    //             path = path.right(path.length() - 1);
    //             #endif
    //             local_pkg.files<<path;
    //         }
    //         this->slot_new_upload_requested(local_pkg, remote_pkg);
    //     }        
    // } else {
    //     qDebug()<<"invalid mime type:"<<data->formats();
    // }
    qDebug()<<__FILE__<<__LINE__<<__FUNCTION__<<"drop mime data processed";
    
    return true;
}

void LocalView::slot_show_hidden(bool show)
{
    if (show) {
        this->model->setFilter(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot);
    } else {
        this->model->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot );
    }
}

void LocalView::slot_mkdir()
{
    QString dir_name;
    QItemSelectionModel *ism = this->curr_item_view->selectionModel();
    QModelIndex cidx, idx;

    // QModelIndexList mil;
    if (ism == 0 || !ism->hasSelection()) {
        // qDebug()<<" selectedIndexes count :"<< mil.count() << " why no item selected????";
        qDebug()<<" selectedIndexes count :"<< ism->hasSelection() << " why no item selected????";
        QMessageBox::critical(this, tr("Warning..."), tr("No item selected"));
        return;
    }

    // mil = ism->selectedIndexes() ;
    cidx = ism->currentIndex();
    idx = ism->model()->index(cidx.row(), 0, cidx.parent());

    // QModelIndex midx = mil.at(0);
    QModelIndex midx = idx;
    QModelIndex aim_midx = (this->curr_item_view == this->uiw->treeView)
        ? this->dir_file_model->mapToSource(midx): midx;

    //检查所选择的项是不是目录
    if (!this->model->isDir(aim_midx)) {
        QMessageBox::critical(this, tr("Warning..."), tr("The selected item is not a directory."));
        return;
    }
    
    dir_name = QInputDialog::getText(this, tr("Create directory:"),
                                     tr("Input directory name:").leftJustified(80, ' '),
                                     QLineEdit::Normal, tr("new_direcotry"));
    if (dir_name == QString::null) {
        return;
    } 
    if (dir_name.length () == 0) {
        // qDebug()<<" selectedIndexes count :"<< mil.count() << " why no item selected????";
        qDebug()<<" selectedIndexes count :"<< ism->hasSelection() << " why no item selected????";
        QMessageBox::critical(this, tr("Warning..."), tr("No directory name supplyed."));
        return;
    }

    if (!QDir().mkdir(this->model->filePath(aim_midx) + "/" + dir_name)) {
        QMessageBox::critical(this, tr("Warning..."), tr("Create directory faild."));
    } else {
        this->slot_refresh_directory_tree();
    }
}

void LocalView::slot_rmdir()
{
    QString dir_name;
    
    QItemSelectionModel *ism = this->curr_item_view->selectionModel();
    // QModelIndexList mil;
    QModelIndex cidx, idx;

    if (ism == 0 || !ism->hasSelection()) {
        qDebug()<<"SelectedIndexes count :"<< ism->hasSelection() << " why no item selected????";
        QMessageBox::critical(this, tr("Warning..."), tr("No item selected"));
        return;
    }    
    
    // mil = ism->selectedIndexes();
    cidx = ism->currentIndex();
    idx = ism->model()->index(cidx.row(), 0, cidx.parent());
    
    QModelIndex midx = idx;
    QModelIndex aim_midx = (this->curr_item_view == this->uiw->treeView) 
        ? this->dir_file_model->mapToSource(midx): midx;

    //检查所选择的项是不是目录
    if (!this->model->isDir(aim_midx)) {
        QMessageBox::critical(this, tr("Warning..."), tr("The selected item is not a directory."));
        return ;
    }
    // qDebug()<<QDir(this->model->filePath(aim_midx)).count();
    if (QDir(this->model->filePath(aim_midx)).count() > 2) {
        QMessageBox::critical(this, tr("Warning..."), tr("Selected director not empty."));
        return;
    }

    QModelIndex tree_midx = this->dir_file_model->mapFromSource(aim_midx);
    QModelIndex pidx = aim_midx.parent();
    int it_row = tree_midx.row();
    QModelIndex nidx = tree_midx.parent();
    QString next_select_path;
    
    if (this->dir_file_model->rowCount(nidx) == 1) {
        // goto parent 
        next_select_path = this->dir_file_model->filePath(nidx);
    } else if (it_row == this->dir_file_model->rowCount(nidx)-1) {
        // goto privious
        next_select_path = this->dir_file_model->filePath(this->dir_file_model->index(it_row-1, 0, nidx));
    } else if (it_row > this->dir_file_model->rowCount(nidx)-1) {
        // not possible
    } else if (it_row < this->dir_file_model->rowCount(nidx)-1) {
        // goto next
        next_select_path = this->dir_file_model->filePath(this->dir_file_model->index(it_row+1, 0, nidx));
    } else {
        // not possible
    }

    Q_ASSERT(!next_select_path.isEmpty());
    
    if (this->model->rmdir(aim_midx)) {
        if (this->curr_item_view == this->uiw->treeView) {
            // this->slot_dir_tree_item_clicked(tree_midx.parent());
            // A: if has next sible, will select next sible
            // B: if has priv sible, will select privious sible
            // C: if no next and no priv, will select parent 

            // ism = this->curr_item_view->selectionModel();
            idx = ism->currentIndex();
            qDebug()<<idx<<this->dir_file_model->filePath(idx);

            // set selection and go on
            ism->clearSelection();
            QItemSelection *selection = new QItemSelection();
                                                           
            // ism->select(this->dir_file_model->index(next_select_path), QItemSelectionModel::Select | QItemSelectionModel::Current  | QItemSelectionModel::Rows);
            ism->setCurrentIndex(this->dir_file_model->index(next_select_path), QItemSelectionModel::Select | QItemSelectionModel::Current  | QItemSelectionModel::Rows);

            idx = ism->currentIndex();
            qDebug()<<idx<<this->dir_file_model->filePath(idx);

            this->slot_dir_tree_item_clicked(this->dir_file_model->index(next_select_path));
        }
    } else {
        QMessageBox::critical(this, tr("Warning..."),
                              tr("Delete directory faild. Maybe the directory is not empty."));
    }

    // if (!QDir().rmdir(this->model->filePath(aim_midx))) {
    //     QMessageBox::critical(this, tr("Warning..."), tr("Delete directory faild. Mayby the directory is not empty."));
    // } else {
    //     this->slot_refresh_directory_tree();
    // }
}

void LocalView::slot_remove()
{
    QItemSelectionModel *ism = this->curr_item_view->selectionModel();
    // QModelIndexList mil;
    QModelIndex cidx, idx;

    if (ism == 0 || !ism->hasSelection()) {
        QMessageBox::critical(this, tr("Warning..."), tr("No item selected").leftJustified(50, ' '));
        return;
    }
    // mil = ism->selectedIndexes();
    cidx = ism->currentIndex();
    idx = ism->model()->index(cidx.row(), 0, cidx.parent());

    QString local_file = this->curr_item_view==this->uiw->treeView
        ? this->dir_file_model->filePath(idx) : this->model->filePath(idx);

    QStringList local_files;
    for (int i = ism->model()->rowCount(cidx.parent())-1; i >= 0; --i) {
        idx = ism->model()->index(i, 0, cidx.parent());
        if (ism->isRowSelected(i, cidx.parent())) {
            local_file = this->curr_item_view==this->uiw->treeView
                ? this->dir_file_model->filePath(idx) : this->model->filePath(idx);
            local_files.prepend(local_file);
        }
    }

    if (QMessageBox::question(this, tr("Question..."), 
                             QString("%1\n    %2").arg(QString(tr("Are you sure remove it?")))
                              .arg(local_files.join("\n    ")),
                             QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes) {

        bool bok;
        for (int i = ism->model()->rowCount(cidx.parent())-1; i >= 0; --i) {
            idx = ism->model()->index(i, 0, cidx.parent());
            if (ism->isRowSelected(i, cidx.parent())) {
                idx = ism->model()->index(i, 0, cidx.parent());
                if (ism->model() == this->model ?
                    this->model->isDir(idx) : this->model->isDir(idx)) {
                    bok = ism->model() == this->model ?
                        this->model->rmdir(idx) : this->model->rmdir(idx);
                } else {
                    bok = ism->model() == this->model ?
                        this->model->remove(idx) : this->model->remove(idx);
                }

                if (bok) {
                    // this->slot_refresh_directory_tree();
                } else {
                    local_file = this->curr_item_view==this->uiw->treeView
                        ? this->dir_file_model->filePath(idx) : this->model->filePath(idx);
                    q_debug()<<"can not remove file/direcotry:"<<i<<local_file;
                }
            }
        }

        // if (QFile::remove(local_file)) {
        //     this->slot_refresh_directory_tree();    
        // } else {
        //     q_debug()<<"can not remove file:"<<local_file;
        // }
    }
}

void LocalView::slot_rename()
{
    qDebug() <<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;
    QItemSelectionModel *ism = this->curr_item_view->selectionModel();
    // QModelIndexList mil;
    QModelIndex cidx, idx;

    if (ism == 0 || !ism->hasSelection()) {
        QMessageBox::critical(this, tr("Warning..."),
                              tr("No item selected").leftJustified(60, ' '));
        return;
    }
    // mil = ism->selectedIndexes();
    cidx = ism->currentIndex();
    idx = ism->model()->index(cidx.row(), 0, cidx.parent());

    QString local_file = this->curr_item_view==this->uiw->treeView
        ? this->dir_file_model->filePath(idx) : this->model->filePath(idx);
    QString file_name = this->curr_item_view==this->uiw->treeView
        ? this->dir_file_model->fileName(idx) : this->model->fileName(idx);

    QString rename_to;
    rename_to = QInputDialog::getText(this, tr("Rename to:"), 
                                      tr("Input new name for: \"%1\"").arg(file_name).leftJustified(100, ' '),
                                      QLineEdit::Normal, file_name );
     
    if (rename_to  == QString::null) {
        //qDebug()<<" selectedIndexes count :"<< mil.count() << " why no item selected????";
        //QMessageBox::critical(this,tr("Warning..."),tr("No new name supplyed "));
        return;
    }
    if (rename_to.length() == 0) {
        QMessageBox::critical(this, tr("Warning..."), tr("No new name supplyed "));
        return;
    }
    q_debug()<<rename_to<<local_file<<this->curr_item_view<<file_name;
    // QTextCodec *codec = GlobalOption::instance()->locale_codec;
    QString file_path = local_file.left(local_file.length()-file_name.length());
    rename_to = file_path + rename_to;

    if (!QFile::rename(local_file, rename_to)) {
        q_debug()<<"file rename faild";
    }
    // 为什么用这个函数,直接用qt的函数不好吗
    // ::rename(codec->fromUnicode(local_file).data(), codec->fromUnicode(rename_to).data());
    
    this->slot_refresh_directory_tree();
}
void LocalView::slot_copy_path_url()
{
    QItemSelectionModel *ism = this->curr_item_view->selectionModel();
    QModelIndex cidx, idx;

    if (ism == 0) {
        qDebug()<<"Why???? no QItemSelectionModel??";        
        return;
    }
    
    // QModelIndexList mil = ism->selectedIndexes();
    if (!ism->hasSelection()) {
        qDebug()<<" why???? no QItemSelectionModel??";
        return;
    }

    cidx = ism->currentIndex();
    idx = ism->model()->index(cidx.row(), 0, cidx.parent());

    QString local_file = this->curr_item_view==this->uiw->treeView
        ? this->dir_file_model->filePath(idx) : this->model->filePath(idx);
    
    QApplication::clipboard()->setText(local_file);
}

void LocalView::slot_show_properties()
{
    //qDebug() <<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;
    QItemSelectionModel *ism = this->curr_item_view->selectionModel();
    QModelIndex cidx, idx;
    
    if (ism == 0) {
        qDebug()<<"Why???? no QItemSelectionModel??";
        return;
    }
    
    // QModelIndexList mil = ism->selectedIndexes();
    if (!ism->hasSelection()) {
        qDebug()<<"Why???? no QItemSelectionModel??";
        return;
    }

    cidx = ism->currentIndex();
    idx = ism->model()->index(cidx.row(), 0, cidx.parent());

    QString local_file = this->curr_item_view==this->uiw->treeView
        ? this->dir_file_model->filePath(idx) : this->model->filePath(idx);
    //  文件类型，大小，几个时间，文件权限
    //TODO 从模型中取到这些数据并显示在属性对话框中。
    LocalFileProperties *fp = new LocalFileProperties(this);
    fp->set_file_info_model_list(local_file);
    fp->exec();
    delete fp;
}

void LocalView::rm_file_or_directory_recursively()
{
    qDebug() <<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;
}

void LocalView::onDirectoryLoaded(const QString &path)
{
    q_debug()<<path;
    if (this->model->filePath(this->uiw->tableView->rootIndex()) == path) {
        this->uiw->tableView->resizeColumnToContents(0);
        this->onUpdateEntriesStatus();
    }

    if (this->is_dir_complete_request) {
        this->is_dir_complete_request = false;

        QString prefix = this->dir_complete_request_prefix;
        
        QStringList matches;
        QModelIndex currIndex;
        QModelIndex sourceIndex = this->model->index(path);
        int rc = this->model->rowCount(sourceIndex);
        q_debug()<<"lazy load dir rows:"<<rc;
        for (int i = rc - 1; i >= 0; --i) {
            currIndex = this->model->index(i, 0, sourceIndex);
            if (this->model->isDir(currIndex)) {
                matches << this->model->filePath(currIndex);
            }
        }
        this->uiw->widget->onSetCompleteList(prefix, matches);
    }
}

void LocalView::onFileRenamed(const QString &path, const QString &oldName, const QString &newName)
{
    q_debug()<<path<<oldName<<newName;
}

void LocalView::onRootPathChanged(const QString &newPath)
{
    q_debug()<<newPath;
}

void LocalView::onUpdateEntriesStatus()
{
    int entries = this->model->rowCount(this->uiw->tableView->rootIndex());
    QString msg = QString("%1 entries").arg(entries);
    // this->status_bar->showMessage(msg);
    this->entriesLabel->setText(msg);
}

void LocalView::slot_dir_nav_go_home()
{
    q_debug()<<"";
    // check if current index is home index
    // if no, callepse all and expand to home
    // tell dir nav instance the home path

    QModelIndex sourceIndex = this->uiw->tableView->rootIndex();
    QString rootPath = this->model->filePath(sourceIndex);
    if (rootPath != QDir::homePath()) {
        this->uiw->treeView->collapseAll();
        this->expand_to_home_directory(QModelIndex(), 1);
        this->uiw->tableView->setRootIndex(this->model->index(QDir::homePath()));
    }
    this->uiw->widget->onSetHome(QDir::homePath());
}

void LocalView::slot_dir_nav_prefix_changed(const QString &prefix)
{
    // q_debug()<<""<<prefix;
    QStringList matches;
    QModelIndex sourceIndex = this->model->index(prefix);
    QModelIndex currIndex;

    if (prefix == "") {
        sourceIndex = this->model->index(0, 0, QModelIndex());
    } else if (prefix.length() > 1
               && prefix.toUpper().at(0) >= 'A' && prefix.toUpper().at(0) <= 'Z'
               && prefix.at(1) == ':') {
        // leave not changed
    }else if (!sourceIndex.isValid()) {
        int pos = prefix.lastIndexOf('/');
        if (pos == -1) {
            pos = prefix.lastIndexOf('\\');
        }
        if (pos == -1) {
            // how deal this case
            Q_ASSERT(pos >= 0);
        }
        sourceIndex = this->model->index(prefix.left(prefix.length() - pos));
    }
    if (sourceIndex.isValid()) {
        if (this->model->canFetchMore(sourceIndex)) {
            while (this->model->canFetchMore(sourceIndex)) {
                this->is_dir_complete_request = true;
                this->dir_complete_request_prefix = prefix;
                this->model->fetchMore(sourceIndex);
            }
            // sience qt < 4.7 has no directoryLoaded signal, so we should execute now if > 0
            if (strcmp(qVersion(), "4.7.0") < 0) {
                // QTimer::singleShot(this, SLOT(
            }
        } else {
            int rc = this->model->rowCount(sourceIndex);
            q_debug()<<"lazy load dir rows:"<<rc;
            for (int i = rc - 1; i >= 0; --i) {
                currIndex = this->model->index(i, 0, sourceIndex);
                if (this->model->isDir(currIndex)) {
                    matches << this->model->filePath(currIndex);
                }
            }
            this->uiw->widget->onSetCompleteList(prefix, matches);
        }
    } else {
        // any operation for this case???
        q_debug()<<"any operation for this case???"<<prefix;
    }
}

void LocalView::slot_dir_nav_input_comfirmed(const QString &prefix)
{
    q_debug()<<"";

    QModelIndex sourceIndex = this->model->index(prefix);
    QModelIndex currIndex = this->uiw->tableView->rootIndex();
    QString currPath = this->model->filePath(currIndex);

    if (!sourceIndex.isValid()) {
        q_debug()<<"directory not found!!!"<<prefix;
        // TODO, show status???
        this->status_bar->showMessage(tr("Directory not found: %1").arg(prefix));
        return;
    }

    if (currPath != prefix) {
        this->uiw->treeView->collapseAll();
        this->expand_to_directory(prefix, 1);
        this->uiw->tableView->setRootIndex(this->model->index(prefix));
        this->uiw->listView->setRootIndex(this->uiw->tableView->rootIndex());
    }
}

void LocalView::slot_icon_size_changed(int value)
{
    q_debug()<<value;
    this->uiw->listView->setGridSize(QSize(value, value));
}

void LocalView::setFileListViewMode(int mode)
{
    if (mode == GlobalOption::FLV_LARGE_ICON) {
        this->uiw->tableView->setVisible(false);
        this->uiw->listView->setVisible(true);
        this->slot_icon_size_changed(96);
        this->uiw->listView->setViewMode(QListView::IconMode);
    } else if (mode == GlobalOption::FLV_SMALL_ICON) {
        this->uiw->tableView->setVisible(false);
        this->uiw->listView->setVisible(true);
        this->slot_icon_size_changed(48);
        this->uiw->listView->setViewMode(QListView::IconMode);
    } else if (mode == GlobalOption::FLV_LIST) {
        this->uiw->tableView->setVisible(false);
        this->uiw->listView->setVisible(true);
        this->slot_icon_size_changed(32);
        this->uiw->listView->setViewMode(QListView::ListMode);
    } else if (mode == GlobalOption::FLV_DETAIL) {
        this->uiw->tableView->setVisible(true);
        this->uiw->listView->setVisible(false);
    } else {
        Q_ASSERT(1 == 2);
    }
}

