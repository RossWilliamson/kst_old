/***************************************************************************
 *                                                                         *
 *   copyright : (C) 2007 The University of Toronto                        *
 *                   netterfield@astro.utoronto.ca                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "netcdf4.h"

#include <QXmlStreamWriter>
#include <QImageReader>
#include <qcolor.h>

#include <QFile>
#include <QFileInfo>

#include <sstream>
#include <netcdf.h> //Try and use the sync function

using namespace Kst;
using namespace netCDF;
using namespace netCDF::exceptions;

/**********************
Netcdf4Source::Config - This class defines the config widget that will be added to the 
Dialog Config Button for configuring the plugin.  This is only needed for special handling required
by the plugin.  Many plugins will not require configuration.  See plugins/sampleplugin for additional
details.

***********************/
class Netcdf4Source::Config {
public:
  Config() {
  }

  void read(QSettings *cfg, const QString& fileName = QString()) {
    Q_UNUSED(fileName);
    cfg->beginGroup("NetCDF4 Datasource");
    cfg->endGroup();
  }

  void save(QXmlStreamWriter& s) {
    Q_UNUSED(s);
  }

  void load(const QDomElement& e) {
    Q_UNUSED(e);
  }
};


//
// Scalar interface
//

class DataInterfaceNetCdf4Scalar : public DataSource::DataInterface<DataScalar>
{
public:
  DataInterfaceNetCdf4Scalar(Netcdf4Source& s) : netcdf4(s) {}

  // read one element
  int read(const QString&, DataScalar::ReadInfo&);

  // named elements
  QStringList list() const { return netcdf4._scalarList; }
  bool isListComplete() const { return true; }
  bool isValid(const QString&) const;

  // T specific
  const DataScalar::DataInfo dataInfo(const QString&) const { return DataScalar::DataInfo(); }
  void setDataInfo(const QString&, const DataScalar::DataInfo&) {}

  // meta data
  QMap<QString, double> metaScalars(const QString&) { return QMap<QString, double>(); }
  QMap<QString, QString> metaStrings(const QString&) { return QMap<QString, QString>(); }


private:
  Netcdf4Source& netcdf4;
};


int DataInterfaceNetCdf4Scalar::read(const QString& scalar, DataScalar::ReadInfo& p)
{
  return netcdf4.readScalar(p.value, scalar);
}


bool DataInterfaceNetCdf4Scalar::isValid(const QString& scalar) const
{
  return  netcdf4._scalarList.contains( scalar );
}


//
// String interface
//

class DataInterfaceNetCdf4String : public DataSource::DataInterface<DataString>
{
public:
  DataInterfaceNetCdf4String(Netcdf4Source& s) : netcdf4(s) {}

  // read one element
  int read(const QString&, DataString::ReadInfo&);

  // named elements
  QStringList list() const { return netcdf4._strings.keys(); }
  bool isListComplete() const { return true; }
  bool isValid(const QString&) const;

  // T specific
  const DataString::DataInfo dataInfo(const QString&) const { return DataString::DataInfo(); }
  void setDataInfo(const QString&, const DataString::DataInfo&) {}

  // meta data
  QMap<QString, double> metaScalars(const QString&) { return QMap<QString, double>(); }
  QMap<QString, QString> metaStrings(const QString&) { return QMap<QString, QString>(); }


private:
  Netcdf4Source& netcdf4;
};


//-------------------------------------------------------------------------------------------
int DataInterfaceNetCdf4String::read(const QString& string, DataString::ReadInfo& p)
{
  //return netcdf4.readString(p.value, string);
  if (isValid(string) && p.value) {
    *p.value = netcdf4._strings[string];
    return 1;
  }
  return 0;
}


bool DataInterfaceNetCdf4String::isValid(const QString& string) const
{
  return netcdf4._strings.contains( string );
}


//
// Vector interface
//

class DataInterfaceNetCdf4Vector : public DataSource::DataInterface<DataVector>
{
public:
  DataInterfaceNetCdf4Vector(Netcdf4Source& s) : netcdf4(s) {}

  // read one element
  int read(const QString&, DataVector::ReadInfo&);

  // named elements
  QStringList list() const { return netcdf4._fieldList; }
  bool isListComplete() const { return true; }
  bool isValid(const QString&) const;

  // T specific
  const DataVector::DataInfo dataInfo(const QString&) const;
  void setDataInfo(const QString&, const DataVector::DataInfo&) {}

  // meta data
  QMap<QString, double> metaScalars(const QString&);
  QMap<QString, QString> metaStrings(const QString&);


private:
  Netcdf4Source& netcdf4;
};


const DataVector::DataInfo DataInterfaceNetCdf4Vector::dataInfo(const QString &field) const
{
  if (!netcdf4._fieldList.contains(field))
    return DataVector::DataInfo();

  return DataVector::DataInfo(netcdf4.frameCount(field), netcdf4.samplesPerFrame(field));
}



int DataInterfaceNetCdf4Vector::read(const QString& field, DataVector::ReadInfo& p)
{
  return netcdf4.readField(p.data, field, p.startingFrame, p.numberOfFrames);
}


bool DataInterfaceNetCdf4Vector::isValid(const QString& field) const
{
  return  netcdf4._fieldList.contains( field );
}

QMap<QString, double> DataInterfaceNetCdf4Vector::metaScalars(const QString& field)
{
  NcVar var = netcdf4.getVariable(field.toStdString());
  if (var.isNull()) {
    KST_DBG qDebug() << "Queried field " << field << " which can't be read" << endl;
    return QMap<QString, double>();
  }
  QMap<QString, double> fieldScalars;
  fieldScalars["NbAttributes"] = var.getAttCount();
  for (int i=0; i<var.getAttCount(); ++i) {
    //NcAtt att = var.getAtt("test");
    //Again we're not going to implement this just yet
    //Just return the fieldScalars attribute number
    // Only handle char attributes as fieldStrings, the others as fieldScalars
    //if (att->type() == NC_BYTE || att->type() == NC_SHORT || att->type() == NC_INT
    //    || att->type() == NC_LONG || att->type() == NC_FLOAT || att->type() == NC_DOUBLE) {
    // Some attributes may have multiple values => load the first as is, and for the others
    // add a -2, -3, etc... suffix as obviously we can have only one value per scalar.
    // Do it in two steps to avoid a test in the loop while keeping a "clean" name for the first one
    //fieldScalars[QString(att->name())] = att->values()->as_double(0);
    //for (int j=1; j<att->values()->num(); ++j) {
    //    fieldScalars[QString(att->name()) + QString("-") + QString::number(j+1)] = att->values()->as_double(j);
    //}
  }
  return fieldScalars;
}

QMap<QString, QString> DataInterfaceNetCdf4Vector::metaStrings(const QString& field)
{
  NcVar var = netcdf4.getVariable(field.toStdString());
  if (var.isNull()) {
    KST_DBG qDebug() << "Queried field " << field << " which can't be read" << endl;
    return QMap<QString, QString>();
  }

  QMap<QString, QString> fieldStrings;
  QString tmpString;
  for (int i=0; i<var.getAttCount(); ++i) {
    //This is not implemented - just return fieldStrings....
    //Need to implement this
    //
    //NcAtt att = var.getAtt("test")
    // Only handle char/unspecified attributes as fieldStrings, the others as fieldScalars
    //if (att->type() == NC_CHAR || att->type() == NC_UNSPECIFIED) {
    //    fieldStrings[att->name()] = QString(att->values()->as_string(0));
    //}
    // qDebug() << att->name() << ": " << att->values()->num() << endl;
  }
  return fieldStrings;
}


//
// Matrix interface
//

class DataInterfaceNetCdf4Matrix : public DataSource::DataInterface<DataMatrix>
{
public:

  DataInterfaceNetCdf4Matrix(Netcdf4Source& s) : netcdf4(s) {}

  // read one element
  int read(const QString&, DataMatrix::ReadInfo&);

  // named elements
  QStringList list() const { return netcdf4._matrixList; }
  bool isListComplete() const { return true; }
  bool isValid(const QString&) const;

  // T specific
  const DataMatrix::DataInfo dataInfo	(const QString&) const;
  void setDataInfo(const QString&, const DataMatrix::DataInfo&) {}

  // meta data
  QMap<QString, double> metaScalars(const QString&) { return QMap<QString, double>(); }
  QMap<QString, QString> metaStrings(const QString&) { return QMap<QString, QString>(); }


private:
  Netcdf4Source& netcdf4;
};


const DataMatrix::DataInfo DataInterfaceNetCdf4Matrix::dataInfo(const QString& matrix) const
{
  if (!netcdf4._matrixList.contains( matrix ) ) {
    return DataMatrix::DataInfo();
  }

  NcVar var = netcdf4.getVariable(matrix.toStdString());
  if (var.isNull()) {
    return DataMatrix::DataInfo();
  }

  if (var.getDimCount() != 2) {
    return DataMatrix::DataInfo();
  }

  DataMatrix::DataInfo info;
  info.samplesPerFrame = 1;
  // TODO is this right?
  info.xSize = var.getDim(0).getSize();
  info.ySize = var.getDim(1).getSize();

  return info;
}


int DataInterfaceNetCdf4Matrix::read(const QString& field, DataMatrix::ReadInfo& p)
{
  int count = netcdf4.readMatrix(p.data->z, field);

  p.data->xMin = 0;
  p.data->yMin = 0;
  p.data->xStepSize = 1;
  p.data->yStepSize = 1;

  return count;
}


bool DataInterfaceNetCdf4Matrix::isValid(const QString& field) const {
  return  netcdf4._matrixList.contains( field );
}



/**********************
Netcdf4Source - This class defines the main DataSource which derives from DataSource.
The key functions that this class must provide is the ability to create the source, provide details about the source
be able to process the data.

***********************/
Netcdf4Source::Netcdf4Source(Kst::ObjectStore *store, QSettings *cfg, const QString& filename, const QString& type, const QDomElement& e)
  : Kst::DataSource(store, cfg, filename, type), 
    _ncfile(0L),
    _config(0L),
    //_ncErr(NcError::silent_nonfatal), !!!Fix me
    is(new DataInterfaceNetCdf4Scalar(*this)),
    it(new DataInterfaceNetCdf4String(*this)),
    iv(new DataInterfaceNetCdf4Vector(*this)),
    im(new DataInterfaceNetCdf4Matrix(*this))

{
  setInterface(is);
  setInterface(it);
  setInterface(iv);
  setInterface(im);
 
  setUpdateType(None);

  if (!type.isEmpty() && type != "NetCDF4") {
    return;
  }

  _valid = false;
  _maxFrameCount = 0;
  _filename = filename;
  //_config = new Netcdf4Source::Config;
  //_config->read(cfg, filename);
  //if (!e.isNull()) {
  //  _config->load(e);
  //}

  if (init()) {
    _valid = true;
  }

  registerChange();
}



Netcdf4Source::~Netcdf4Source() {
  delete _ncfile;
  _ncfile = 0L;
}


void Netcdf4Source::reset() {
  delete _ncfile;
  _ncfile = 0L;
  _maxFrameCount = 0;
  _valid = init();
  Object::reset();
}

void Netcdf4Source::add_variables(std::multimap<std::string, NcVar> varMap, std::string prefix) {

  std::multimap<std::string, NcVar>::iterator iter;
  std::string var_name;
  NcVar temp_var;

  for(iter=varMap.begin(); iter != varMap.end(); iter++) {
    var_name = prefix + ":" + iter->first;
    //We always in this setup have an extra : at the start
    //remove that then we have a good name
    var_name = var_name.substr(1);

    //Just deal with single dimensions first
    //but keep the current style of field/matrix etc
    if (iter->second.getDimCount() == 0) {
      _scalarList << QString(var_name.c_str());
    } else if (iter->second.getDimCount() == 1) {
      _fieldList << QString(var_name.c_str());
      NcDim temp_dim;
      temp_dim = iter->second.getDim(0); //Only one dim
      int fc = temp_dim.getSize();
      std::cout << var_name << " " << iter->second.getDimCount() << 
	" " << _maxFrameCount << " " << fc <<  std::endl;
      _maxFrameCount = qMax(_maxFrameCount, fc);
      _frameCounts[var_name.c_str()] = fc;
    } else if (iter->second.getDimCount() == 2) {
      //We are going to fudge it here and assume that we actually
      //have a single vector object and not 2d. We also have
      //To assume at the moment that the last dimension is the
      //The data container.
      NcDim dim1 = iter->second.getDim(1);
      int num_elements = dim1.getSize();
      for(int i=0; i<num_elements; i++) {
	//We add vector item onto the name
	std::stringstream ss;
	std::string temp_name;
	ss << var_name << "[" << i << "]";
	temp_name = ss.str();
	_fieldList << QString(temp_name.c_str());
	//And frame count is in second
	NcDim temp_dim;
	temp_dim = iter->second.getDim(0);
	int fc = temp_dim.getSize();
	_maxFrameCount = qMax(_maxFrameCount, fc);
	_frameCounts[temp_name.c_str()] = fc;
      }
      //_matrixList += var_name.c_str();
    } else {
      //Should be simple to implement even deeper nests of dims
      //So long as we don't actually want a 2d image.
      qDebug() << var_name.c_str() << iter->second.getDimCount() << "> 2 dims - not yet implemented";
    }
  }
}

void Netcdf4Source::traverse_groups(NcGroup temp_group, std::string prefix) {
  if(temp_group.getVarCount() > 1) {
    std::string temp_prefix;
    temp_prefix = prefix + ":" + temp_group.getName();
    add_variables(temp_group.getVars(),temp_prefix);
  }

  int n_groups = temp_group.getGroupCount();
  if(n_groups != 0)
    {
      std::string temp_prefix = "";
      std::multimap<std::string,NcGroup> groupMap;
      std::multimap<std::string,NcGroup>::iterator iter;
      groupMap = temp_group.getGroups();

      for(iter=groupMap.begin(); iter !=groupMap.end(); iter++) {
	temp_prefix = prefix + ":" + temp_group.getName();
	traverse_groups(iter->second, temp_prefix);
      }
    }
}


// If the datasource has any predefined fields they should be populated here.
bool Netcdf4Source::init() {
  _ncfile = new NcFile(_filename.toUtf8().data(), NcFile::read);
  if (_ncfile->isNull()) {
    qDebug() << _filename << ": failed to open in initFile()" << endl;
    return false;
  }

  
  _scalarList.clear();
  _fieldList.clear();
  _matrixList.clear();
  _strings.clear();

  _fieldList += "INDEX";
  _maxFrameCount = 0;

  qDebug() << "Adding Variables";

  //First list any varibles in the top level
  if(_ncfile->getVarCount() > 1) {
    add_variables(_ncfile->getVars(), "");
  }

  if(_ncfile->getGroupCount() != 0) {
    //Test getting all nested groups and print them out

    std::multimap<std::string,NcGroup> groupMap;
    std::multimap<std::string,NcGroup>::iterator iter;
    groupMap = _ncfile->getGroups();

    for(iter=groupMap.begin(); iter !=groupMap.end(); iter++) {
      traverse_groups(iter->second, ""); //No prefix for start
    }
  }

  /*
  // Get strings - Not yet implemented
  int globalAttributesNb = _ncfile->num_atts();
  qDebug() << " rr " << globalAttributesNb << endl;
  for (int i = 0; i < globalAttributesNb; ++i) {
  // Get only first value, should be enough for a start especially as strings are complete
  NcAtt *att = _ncfile->get_att(i);
  if (att) {
  QString attrName = QString(att->name());
  char *attString = att->as_string(0);
  QString attrValue = QString(att->as_string(0));
  delete[] attString;
  //TODO port
  //KstString *ms = new KstString(KstObjectTag(attrName, tag()), this, attrValue);
  _strings[attrName] = attrValue;
  }
  delete att;
  }
  */

  QStringList::iterator iter;
  for(iter = _fieldList.begin(); iter != _fieldList.end(); iter++) {
    std::cout << (*iter).toLocal8Bit().constData() << std::endl;
  }
  registerChange();
  return true; // false if something went wrong
}

Kst::Object::UpdateType Netcdf4Source::internalDataSourceUpdate() {
  //TODO port
  /*
    if (KstObject::checkUpdateCounter(u)) {
    return lastUpdateResult();
    }
  */

  //Sync function not yet implemented in c++4 API
  //Have to do the following
  //Sync does not work! try something drastic and bad
  //delete the pointer and reopen the file (arghhhh)
  nc_sync(_ncfile->getId());
  try {
    //delete _ncfile;
    //_ncfile = new NcFile(_filename.toUtf8().data(), NcFile::read);

    bool updated = false;

    /* Update member variables _ncfile, _maxFrameCount,
       and _frameCounts and indicate that an update is needed */

    //Easiest way is to use our internal record of variables
    //Rather than do a traverse of the netcdf4 file
    //We will of course not pick up any new varibles.
    //Initally only do _fieldlist
    //Actually for real efficiency we should only check any
    //unlimited dimensions or just the ones we are moitoring
    //but don't know how to do that

    QStringList::iterator iter;

    for(iter = _fieldList.begin(); iter != _fieldList.end(); iter++) {
      NcVar var = getVariable(iter->toStdString());
      if (var.isNull()) {
	continue;
      }
      //unlimited is always the one we are intersted in
      //So nothing fancy here just check that should work
      NcDim temp_dim;
      temp_dim = var.getDim(0); //Only one dim
      int fc = temp_dim.getSize();
      _maxFrameCount = qMax(_maxFrameCount, fc);
      updated = updated || (_frameCounts[*iter] != fc);
      _frameCounts[*iter] = fc;
    }
    //qDebug() << "Trying to update the data file" << updated;
    return updated ? Object::Updated : Object::NoChange;

  } catch(NcException& e) {
    qDebug() << "CAUGHT EXCEPTION";
    std::cerr<< e.what();
  }
  return Object::NoChange;
}


NcVar Netcdf4Source::getVariable(std::string name)
{
  //Varible name is made up of nested groups
  //Need to split on : and put into elems.
  std::vector<std::string> elems;
  std::string item;
  NcGroup temp_group;
  NcVar temp_var;
  //First remove any vector identifier "[n]"
  //We work this out later

  if((name[name.size()-1] == ']') && (name.rfind('[') != name.npos))
    name.resize(name.rfind('['));

  std::stringstream ss(name);

  while(std::getline(ss,item,':'))
    elems.push_back(item);

  if(elems.size() == 0) {
    qDebug() << "Invalid register name";
  } else if(elems.size() == 1) {
    //Simple top level so take varible off file
    temp_var = _ncfile->getVar(elems[0]);
  } else {
    //We have groups - work our way through them
    temp_group = _ncfile->getGroup(elems[0]);
    for(unsigned int i=1; i<elems.size()-1; i++)
      temp_group = temp_group.getGroup(elems[i]);
    temp_var = temp_group.getVar(elems[elems.size()-1]);
  }
  //Get here then return null variable

  return temp_var;
}

int Netcdf4Source::readScalar(double *v, const QString& field)
{
  // TODO error handling
  NcVar var = getVariable(field.toStdString());
  if (!var.isNull()) {
    var.getVar(v);
    return 1;
  }
  return 0;
}

int Netcdf4Source::readString(QString *stringValue, const QString& stringName)
{
  // TODO more error handling?
  /* NcAtt *att = _ncfile->get_att((NcToken) stringName.toLatin1().data());
     if (att) {
     *stringValue = QString(att->as_string(0));
     delete att;
     return 1;
     }
     return 0;*/
  return 0;
}

int Netcdf4Source::extractRow(QString field) {
  int start_index,end_index,count;
  start_index = field.lastIndexOf("[")+1;
  end_index = field.lastIndexOf("]");
  count = end_index-start_index;

  if ((start_index == -1) || (end_index == -1))
    return -1;

  return field.mid(start_index,count).toInt();
}

int Netcdf4Source::readField(double *v, const QString& field, int s, int n) {
  /* Values for one record */
  KST_DBG qDebug() << "Entering Netcdf4Source::readField with params: " << field << ", from " << s << " for " << n << " frames" << endl;

  /* For INDEX field */
  if (field.toLower() == "index") {
    if (n < 0) {
      v[0] = double(s);
      return 1;
    }
    for (int i = 0; i < n; ++i) {
      v[i] = double(s + i);
    }
    return n;
  }

  /* For a variable from the netCDF4 file */
  NcVar var = getVariable(field.toStdString());
  if (var.isNull()) {
    KST_DBG qDebug() << "Queried field " << field << " which can't be read" << endl;
    return -1;
  }

  NcType dataType(var.getType()); /* netCDF4 data type */
  
  std::vector<size_t> start_p,count_p;

  NcDim temp_dim;
  temp_dim = var.getDim(0); //Fast Dim check
  int fc = temp_dim.getSize();
  if (s >= fc) {
    return 0;
  }

  //check the number of dims for arrays to vectors.
  if (var.getDimCount() == 0) {
    qDebug() << "Should not get here - Scalars not implemented";
  } else if (var.getDimCount() == 1) {
    start_p.push_back(s);
    count_p.push_back(n);
  } else if (var.getDimCount() == 2) {
    int index = extractRow(field);
    start_p.push_back(s); //record
    start_p.push_back(index); //row
    count_p.push_back(n);
    count_p.push_back(1); //Only one
  } else {
    qDebug() << "Dimensions > 2 not yet implemented";
    return 0;
  }

  try {
    var.getVar(start_p,count_p,v); //Get the data
  } catch (NcException& e) {
    qDebug() << "EXCEPTION";
    e.what();
  }


  KST_DBG qDebug() << "Finished reading " << field << endl;

  // return oneSample ? 1 : n * recSize;
  return n;
}




int Netcdf4Source::readMatrix(double *v, const QString& field)
{
  /* For a variable from the netCDF4 file */
  NcVar var = getVariable(field.toStdString());

  if (var.isNull()) {
    KST_DBG qDebug() << "Queried field " << field << " which can't be read" << endl;
    return -1;
  }

  int xSize = var.getDim(0).getSize();
  int ySize = var.getDim(1).getSize();

  //var->get(v, xSize, ySize); //!!!Not implemented
  return  xSize * ySize;
}

int Netcdf4Source::samplesPerFrame(const QString& field) {
  if (field.toLower() == "index") {
    return 1;
  }

  NcVar var = getVariable(field.toStdString());
  if(var.isNull()) {
    KST_DBG qDebug() << "Queried field " << field << " which can't be read" << endl;
    return 0;
  }

  NcDim temp_dim;
  temp_dim = var.getDim(0); //Only one dim
  //int fc = temp_dim.getSize();

  //BUT the above is just there in case we edit this in the future
  //To include multi-dim etc with more fancy netcdf4 loading
  //In reality we will only have one sample per frame
  //So return 1
  return 1;
}

int Netcdf4Source::frameCount(const QString& field) const {
  if (field.isEmpty() || field.toLower() == "index") {
    return _maxFrameCount;
  } else {
    return _frameCounts[field];
  }
}

QString Netcdf4Source::fileType() const {
  return "NetCDF4 Datasource";
}


void Netcdf4Source::save(QXmlStreamWriter &streamWriter) {
  Kst::DataSource::save(streamWriter);
}



/*-------------------------------------------------------*/
/*------------------ PLUGIN CLASS -----------------------*/
/*-------------------------------------------------------*/

// Name used to identify the plugin.  Used when loading the plugin.
QString Netcdf4Plugin::pluginName() const { return "NetCDF4 Reader"; }
QString Netcdf4Plugin::pluginDescription() const { return "NetCDF4 Reader"; }



Kst::DataSource *Netcdf4Plugin::create(Kst::ObjectStore *store,
				       QSettings *cfg,
				       const QString &filename,
				       const QString &type,
				       const QDomElement &element) const {

  return new Netcdf4Source(store, cfg, filename, type, element);
}


// Provides the matrix list that this dataSource can provide from the provided filename.
// This function should use understands to validate the file and then open and calculate the 
// list of matrices.
QStringList Netcdf4Plugin::matrixList(QSettings *cfg,
				      const QString& filename,
				      const QString& type,
				      QString *typeSuggestion,
				      bool *complete) const {


  if (typeSuggestion) {
    *typeSuggestion = "NetCDF4 Datasource";
  }
  if ((!type.isEmpty() && !provides().contains(type)) ||
      0 == understands(cfg, filename)) {
    if (complete) {
      *complete = false;
    }
    return QStringList();
  }
  QStringList matrixList;

  return matrixList;

}


// Provides the scalar list that this dataSource can provide from the provided filename.
// This function should use understands to validate the file and then open and calculate the 
// list of scalars if necessary.
QStringList Netcdf4Plugin::scalarList(QSettings *cfg,
				      const QString& filename,
				      const QString& type,
				      QString *typeSuggestion,
				      bool *complete) const {

  QStringList scalarList;

  if ((!type.isEmpty() && !provides().contains(type)) || 0 == understands(cfg, filename)) {
    if (complete) {
      *complete = false;
    }
    return QStringList();
  }

  if (typeSuggestion) {
    *typeSuggestion = "NetCDF4 Datasource";
  }

  scalarList.append("FRAMES");
  return scalarList;

}


// Provides the string list that this dataSource can provide from the provided filename.
// This function should use understands to validate the file and then open and calculate the 
// list of strings if necessary.
QStringList Netcdf4Plugin::stringList(QSettings *cfg,
                                      const QString& filename,
                                      const QString& type,
                                      QString *typeSuggestion,
                                      bool *complete) const {

  QStringList stringList;

  if ((!type.isEmpty() && !provides().contains(type)) || 0 == understands(cfg, filename)) {
    if (complete) {
      *complete = false;
    }
    return QStringList();
  }

  if (typeSuggestion) {
    *typeSuggestion = "NetCDF4 Datasource";
  }

  stringList.append("FILENAME");
  return stringList;

}


// Provides the field list that this dataSource can provide from the provided filename.
// This function should use understands to validate the file and then open and calculate the 
// list of fields if necessary.
QStringList Netcdf4Plugin::fieldList(QSettings *cfg,
				     const QString& filename,
				     const QString& type,
				     QString *typeSuggestion,
				     bool *complete) const {
  Q_UNUSED(cfg)
    Q_UNUSED(filename)
    Q_UNUSED(type)

    if (complete) {
      *complete = true;
    }

  if (typeSuggestion) {
    *typeSuggestion = "NetCDF4 Datasource";
  }

  QStringList fieldList;
  return fieldList;
}


// The main function used to determine if this plugin knows how to process the provided file.
// Each datasource plugin should check the file and return a number between 0 and 100 based 
// on the likelyhood of the file being this type.  100 should only be returned if there is no way
// that the file could be any datasource other than this one.
int Netcdf4Plugin::understands(QSettings *cfg, const QString& filename) const {
  Q_UNUSED(cfg);

  QFile f(filename);

  qDebug() << filename;
  if (!f.open(QFile::ReadOnly)) {
    KST_DBG qDebug() << "Unable to read file !" << endl;
    return 0;
  }

  //Check for correct extenstion
  QFileInfo fileInfo(f.fileName());
  QString fname(fileInfo.suffix());

  qDebug() << fname;

  if(fname == "nc4") {
    qDebug() << "Correct extension";
  } else {
    qDebug() << "Wrong extension";
    return 0;
  }

  NcFile *ncfile;
  try {
    ncfile = new NcFile(filename.toUtf8().data(), NcFile::read);
  } catch (NcException& e) {
    qDebug() << "EXCEPTION";
    e.what();
    delete ncfile;
    return 0;
  }

  if (!ncfile->isNull()) {
    KST_DBG qDebug() << filename << " looks like netCDF !" << endl;
    delete ncfile;
    return 80;
  } else {
    delete ncfile;
    return 0;
  }
  return 0;
}



bool Netcdf4Plugin::supportsTime(QSettings *cfg, const QString& filename) const {
  //FIXME
  Q_UNUSED(cfg)
    Q_UNUSED(filename)
    return true;
}


QStringList Netcdf4Plugin::provides() const {
  QStringList rc;
  rc += "NetCDF4 Datasource";
  return rc;
}


// Request for this plugins configuration widget.  
Kst::DataSourceConfigWidget *Netcdf4Plugin::configWidget(QSettings *cfg, const QString& filename) const {
  Q_UNUSED(cfg)
    Q_UNUSED(filename)
    return 0;;

}

#ifndef QT5
Q_EXPORT_PLUGIN2(kstdata_NetCDF4, Netcdf4Plugin)
#endif

// vim: ts=2 sw=2 et
