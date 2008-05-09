/***************************************************************************
 *                                                                         *
 *   copyright : (C) 2007 The University of Toronto                        *
 *   copyright : (C) 2005  University of British Columbia                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
// use KCodecs::base64Encode() in kmdcodecs.h
// Create QDataStream into a QByteArray
// qCompress the bytearray
#include <QXmlStreamWriter>

#include "editablevector.h"
#include "debug.h"
#include "kst_i18n.h"

namespace Kst {

const QString EditableVector::staticTypeString = I18N_NOOP("Editable Vector");
const QString EditableVector::staticTypeTag = I18N_NOOP("editablevector");

EditableVector::EditableVector(ObjectStore *store, const ObjectTag& tag, const QByteArray &data)
    : Vector(store, tag, data) {
  _editable = true;
  _saveable = true;
  _saveData = true;
}


EditableVector::EditableVector(ObjectStore *store, const ObjectTag& tag, int n)
    : Vector(store, tag, n) {
  _editable = true;
  _saveable = true;
  _saveData = true;
}


const QString& EditableVector::typeString() const {
  return staticTypeString;
}


Object::UpdateType EditableVector::update() {
  Q_ASSERT(myLockStatus() == KstRWLock::WRITELOCKED);

  bool force = dirty();

  Object::UpdateType baseRC = Vector::update();
  if (force) {
    baseRC = UPDATE;
  }

  return baseRC;
}


void EditableVector::setSaveData(bool save) {
  Q_UNUSED(save)
}

/** Save vector information */
void EditableVector::save(QXmlStreamWriter &s) {
  s.writeStartElement("editablevector");
  s.writeAttribute("tag", tag().tagString());
  saveNameInfo(s);

  if (_saveData) {
    QByteArray qba(length()*sizeof(double), '\0');
    QDataStream qds(&qba, QIODevice::WriteOnly);

    for (int i = 0; i < length(); i++) {
      qds << _v[i];
    }

    s.writeTextElement("data", qCompress(qba).toBase64());
  }
  s.writeEndElement();
}

QString EditableVector::_automaticDescriptiveName() {
  QString name("(");
  if (length()>=1) {
    name += QString::number(_v[0]);
  }
  if (length()>=2) {
    name += ", " + QString::number(_v[1]);
  }

  if (length()>=3) {
    name += ", ...";
  }

  name += ")";

  return name;
}
}
// vim: ts=2 sw=2 et
