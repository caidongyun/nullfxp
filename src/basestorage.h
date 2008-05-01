/* basestorage.h --- 
 * 
 * Filename: basestorage.h
 * Description: 
 * Author: liuguangzhao
 * Maintainer: 
 * Created: 五  4月  4 14:46:49 2008 (CST)
 * Version: 
 * Last-Updated: 五  4月  4 14:47:37 2008 (CST)
 *           By: liuguangzhao
 *     Update #: 1
 * URL: 
 * Keywords: 
 * Compatibility: 
 * 
 */

/* Commentary: 
 * 
 * 
 * 
 */

/* Change log:
 * 
 * 
 */

/* This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth
 * Floor, Boston, MA 02110-1301, USA.

 */

/* Code: */




#include <QtCore>
#include <QDataStream>

class BaseStorage
{
 public:
  BaseStorage();
  ~BaseStorage();
  bool open();
  bool close();
  bool save();

  bool addHost(QMap<QString,QString> host);
  bool removeHost(QString show_name);
  bool updateHost(QMap<QString,QString> host);
  bool clearHost();

  bool containsHost(QString show_name);

  QMap<QString, QMap<QString,QString> > & getAllHost();
  QMap<QString,QString> getHost(QString show_name);
  int hostCount();
  
 signals:
  void hostListChanged();
  void hostLIstChanged(QString show_name);
 private:
  QMap<QString,QMap<QString,QString> >  hosts;
  bool opened;
  bool changed;
  QDataStream  ioStream;
};

/* basestorage.h ends here */