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

#include "vectoreditordialog.h"

#include "document.h"
#include "vectortablemodel.h"

#include <kstdatacollection.h>

namespace Kst {

VectorEditorDialog::VectorEditorDialog(QWidget *parent, Document *doc)
: QDialog(parent), _doc(doc) {
  _model = new VectorTableModel;
  // TODO: Extract this model so the dialog can be reused, and make a new model
  // or modification to the model so that it tracks all the vector creates and
  // destroys
  KST::vectorList.lock().readLock();
  foreach (KstVectorPtr v, KST::vectorList.list()) {
    VectorModel *vm = new VectorModel(v);
    _model->vectors().append(vm);
  }
  KST::vectorList.lock().unlock();
  ui.setupUi(this);
  ui.vectors->setModel(_model);
}


VectorEditorDialog::~VectorEditorDialog() {
  ui.vectors->setModel(0);
  qDeleteAll(_model->vectors());
  _model->vectors().clear();
  delete _model;
  _model = 0;
}

}

#include "vectoreditordialog.moc"
// vim: ts=2 sw=2 et
