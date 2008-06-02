/***************************************************************************
                 object.h: abstract base class for all Kst objects
                             -------------------
    begin                : May 22, 2003
    copyright            : (C) 2003 The University of Toronto
    email                :
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef OBJECT_H
#define OBJECT_H

#include <QPointer>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QDebug>
#include <QStringList>
#include <QMetaType>
#include <QXmlStreamWriter>

#include "namedobject.h"
#include "kst_export.h"
#include "sharedptr.h"
#include "rwlock.h"

namespace Kst {

class ObjectStore;
class Object;

typedef SharedPtr<Object> ObjectPtr;


class Object : public QObject, public Shared, public KstRWLock, public NamedObject {
  Q_OBJECT

  public:
    static QString type();

    enum UpdateType { NO_CHANGE = 0, UPDATE };

    virtual UpdateType update() = 0;

    virtual const QString& typeString() const;
    static const QString staticTypeString;

    ObjectStore *store() const;

    // Returns count - 2 to account for "this" and the list pointer, therefore
    // you MUST have a reference-counted pointer to call this function
    virtual int getUsage() const;

    // TODO: do we need this?
//    int operator==(const QString&) const;

    virtual void deleteDependents();

    // @since 1.1.0
    virtual void setDirty(bool dirty = true);
    // @since 1.1.0
    bool dirty() const;

    virtual void beginUpdate(ObjectPtr object);
    virtual void processUpdate(ObjectPtr object);
  protected:
    Object();
    virtual ~Object();

    friend class ObjectStore;
    ObjectStore *_store;  // set by ObjectStore

  private:
    bool _dirty;

} KST_EXPORT;



template <typename T, typename U>
inline SharedPtr<T> kst_cast(SharedPtr<U> object) {
  return qobject_cast<T*>(object.data());
}
// FIXME: make this safe
template <typename T>
inline SharedPtr<T> kst_cast(QObject *object) {
  return qobject_cast<T*>(object);
}

}

Q_DECLARE_METATYPE(Kst::Object*)

#endif

// vim: ts=2 sw=2 et
