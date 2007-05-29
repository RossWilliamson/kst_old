/***************************************************************************
 *                                                                         *
 *   copyright : (C) 2007 The University of Toronto                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <QString>

namespace Kst {

class SessionModel;

class Document {
  public:
    Document();
    ~Document();

    SessionModel* session() const;

    void save(const QString& to = QString::null);

  private:
    SessionModel *_session;
};

}

#endif

// vim: ts=2 sw=2 et
