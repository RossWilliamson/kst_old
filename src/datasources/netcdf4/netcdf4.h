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


#ifndef NETCDF4SOURCE_H
#define NETCDF4SOURCE_H

#include <datasource.h>
#include <dataplugin.h>

#include <netcdf>

using namespace netCDF;

class DataInterfaceNetCdf4Scalar;
class DataInterfaceNetCdf4String;
class DataInterfaceNetCdf4Vector;
class DataInterfaceNetCdf4Matrix;

class Netcdf4Source : public Kst::DataSource {
  Q_OBJECT

  public:
    Netcdf4Source(Kst::ObjectStore *store, QSettings *cfg, const QString& filename, const QString& type, const QDomElement& e);

    ~Netcdf4Source();

    bool init();
    virtual void reset();

    Kst::Object::UpdateType internalDataSourceUpdate();
   
    int readScalar(double *v, const QString& field);
    int readString(QString *stringValue, const QString& stringName);
    int readField(double *v, const QString& field, int s, int n);
    int readMatrix(double *v, const QString& field);
 
    int samplesPerFrame(const QString& field);
    int frameCount(const QString& field = QString()) const;
    NcVar getVariable(std::string);
    void add_variables(std::multimap<std::string, NcVar>, std::string);
    void traverse_groups(NcGroup, std::string);
    int extractRow(QString);

    QString fileType() const;

    void save(QXmlStreamWriter &streamWriter);

    class Config;


  private:
    NcFile *_ncfile;
    mutable Config *_config;
    int _maxFrameCount;

    QMap<QString, QString> _strings;
    QMap<QString, int> _frameCounts;

    QStringList _scalarList;
    QStringList _fieldList;
    QStringList _matrixList;

    friend class DataInterfaceNetCdf4Scalar;
    friend class DataInterfaceNetCdf4String;
    friend class DataInterfaceNetCdf4Vector;
    friend class DataInterfaceNetCdf4Matrix;
    DataInterfaceNetCdf4Scalar* is;
    DataInterfaceNetCdf4String* it;
    DataInterfaceNetCdf4Vector* iv;
    DataInterfaceNetCdf4Matrix* im;
};


class Netcdf4Plugin : public QObject, public Kst::DataSourcePluginInterface {
    Q_OBJECT
    Q_INTERFACES(Kst::DataSourcePluginInterface)
    Q_PLUGIN_METADATA(IID "com.kst.DataSourcePluginInterface/2.0")
  public:
    virtual ~Netcdf4Plugin() {}

    virtual QString pluginName() const;
    virtual QString pluginDescription() const;

    virtual bool hasConfigWidget() const { return false; }

    virtual Kst::DataSource *create(Kst::ObjectStore *store,
                                  QSettings *cfg,
                                  const QString &filename,
                                  const QString &type,
                                  const QDomElement &element) const;

    virtual QStringList matrixList(QSettings *cfg,
                                  const QString& filename,
                                  const QString& type,
                                  QString *typeSuggestion,
                                  bool *complete) const;

    virtual QStringList fieldList(QSettings *cfg,
                                  const QString& filename,
                                  const QString& type,
                                  QString *typeSuggestion,
                                  bool *complete) const;

    virtual QStringList scalarList(QSettings *cfg,
                                  const QString& filename,
                                  const QString& type,
                                  QString *typeSuggestion,
                                  bool *complete) const;

    virtual QStringList stringList(QSettings *cfg,
                                  const QString& filename,
                                  const QString& type,
                                  QString *typeSuggestion,
                                  bool *complete) const;

    virtual int understands(QSettings *cfg, const QString& filename) const;

    virtual bool supportsTime(QSettings *cfg, const QString& filename) const;

    virtual QStringList provides() const;

    virtual Kst::DataSourceConfigWidget *configWidget(QSettings *cfg, const QString& filename) const;
};


#endif
// vim: ts=2 sw=2 et
