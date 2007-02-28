/***************************************************************************
                   extensiondlg.cpp
                             -------------------
    begin                : 02/28/07
    copyright            : (C) 2007 The University of Toronto
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

#include "extensiondlg.h"

#include <kst_export.h>

ExtensionDialog::ExtensionDialog(QWidget *parent)
    : QWidget(parent) {
  setupUi(this);

  connect(, SIGNAL(), this, SLOT());
}


ExtensionDialog::~ExtensionDialog() {}


void ExtensionDialog::show() {
  _extensions->clear();
  KService::List sl = KServiceType::offers("Kst Extension");
  for (KService::List::ConstIterator it = sl.begin(); it != sl.end(); ++it) {
    KService::Ptr service = *it;
    QString name = service->property("Name").toString();
    QCheckListItem *i = new QCheckListItem(_extensions, name, QCheckListItem::CheckBox);
    i->setText(1, service->property("Comment").toString());
    i->setText(2, service->property("X-Kst-Plugin-Author").toString());
    i->setText(3, KLibLoader::findLibrary(service->library().latin1(), KstApp::inst()->instance()));
    if (!ExtensionMgr::self()->extensions().contains(name)) {
      ExtensionMgr::self()->setEnabled(name, service->property("X-Kst-Enabled").toBool());
    }
    i->setOn(ExtensionMgr::self()->enabled(name));
  }
  QDialog::show();
}


void ExtensionDialog::accept() {
  ExtensionMgr *mgr = ExtensionMgr::self();
  QListViewItemIterator it(_extensions); // don't use Checked since it is too new
  while (it.current()) {
    mgr->setEnabled(it.current()->text(0), static_cast<QCheckListItem*>(it.current())->isOn());
    ++it;
  }
  mgr->updateExtensions();
  QDialog::accept();
}

#include "extensiondialog.moc"

// vim: ts=2 sw=2 et
