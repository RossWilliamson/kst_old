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

#include "kst_i18n.h"
#include "generaltab.h"

namespace Kst {

GeneralTab::GeneralTab(QWidget *parent)
  : DialogTab(parent) {

  setupUi(this);
  setTabTitle(tr("General"));

  connect(_useOpenGL, SIGNAL(stateChanged(int)), this, SIGNAL(modified()));
  connect(_maxUpdate, SIGNAL(valueChanged(int)), this, SIGNAL(modified()));
}


GeneralTab::~GeneralTab() {
}

bool GeneralTab::useOpenGL() const {
  return _useOpenGL->isChecked();
}


void GeneralTab::setUseOpenGL(const bool useOpenGL) {
  _useOpenGL->setChecked(useOpenGL);
}


int GeneralTab::minimumUpdatePeriod() const {
  return _maxUpdate->value();
}


void GeneralTab::setMinimumUpdatePeriod(const int period) {
  _maxUpdate->setValue(period);
}

}

// vim: ts=2 sw=2 et
