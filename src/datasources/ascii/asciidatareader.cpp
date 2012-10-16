/***************************************************************************
 *                                                                         *
 *   Copyright : (C) 2003 The University of Toronto                        *
 *   email     : netterfield@astro.utoronto.ca                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "asciidatareader.h"
#include "asciisourceconfig.h"

#include "math_kst.h"
#include "kst_inf.h"
#include "kst_i18n.h"
#include "kst_atof.h"
#include "measuretime.h"

#include <QFile>
#include <QDebug>
#include <ctype.h>
#include <stdlib.h>


using namespace AsciiCharacterTraits;


// Enable QASSERT in QVarLengthArray  when using [] on data
#if 0
#define constData constArray
#else
#define constData constPointer // loads faster in debug mode
#endif


//-------------------------------------------------------------------------------------------
AsciiDataReader::AsciiDataReader(AsciiSourceConfig& config) :
  _config(config),
  isDigit(),
  isWhiteSpace()
{
}

//-------------------------------------------------------------------------------------------
AsciiDataReader::~AsciiDataReader()
{
}

//-------------------------------------------------------------------------------------------
void AsciiDataReader::clear()
{
  _rowIndex.clear();
  setRow0Begin(-1);
  _numFrames = 0;
}

//-------------------------------------------------------------------------------------------
void AsciiDataReader::setRow0Begin(int begin)
{
  _rowIndex.resize(1);
  _rowIndex[0] = begin;
}

//-------------------------------------------------------------------------------------------
void AsciiDataReader::detectLineEndingType(QFile& file)
{
  QByteArray line;
  int line_size = 0;
  while (line_size < 2 && !file.atEnd()) {
    line = file.readLine();
    line_size = line.size();
  }
  file.seek(0);
  if (line_size < 2) {
    _lineending = LineEndingType();
  } else {
    _lineending.is_crlf = line[line_size - 2] == '\r' && line[line_size - 1] == '\n' ;
    _lineending.character =  _lineending.is_crlf ? line[line_size - 2] : line[line_size - 1];
  }
}

//-------------------------------------------------------------------------------------------
void AsciiDataReader::toDouble(const LexicalCast& lexc, const char* buffer, int bufread, int ch, double* v, int)
{
  if (   isDigit(buffer[ch])
         || buffer[ch] == '-'
         || buffer[ch] == '.'
         || buffer[ch] == '+'
         || isWhiteSpace(buffer[ch])) {
    *v = lexc.toDouble(&buffer[0] + ch);
  } else if ( ch + 2 < bufread
              && tolower(buffer[ch]) == 'i'
              && tolower(buffer[ch + 1]) == 'n'
              && tolower(buffer[ch + 2]) == 'f') {
    *v = INF;
  }

#if 0
  // TODO enable by option: "Add unparsable lines as strings"
  else {
    if (_rowIndex.size() > row + 1) {
      QString unparsable = QString::fromAscii(&buffer[_rowIndex[row]], _rowIndex[row + 1] - _rowIndex[row]);
      _strings[QString("Unparsable %1").arg(row)] = unparsable.trimmed();
    }
  }
#endif
}

//-------------------------------------------------------------------------------------------
bool AsciiDataReader::findDataRows(bool read_completely, QFile& file, int _byteLength)
{
  detectLineEndingType(file);

  bool new_data = false;
  AsciiFileData buf;
  do {
    // Read the tmpbuffer, starting at row_index[_numFrames]
    buf.clear();

    // always read from the start of a line
    buf.read(file, _rowIndex[_numFrames], _byteLength - buf.begin(), AsciiFileData::Prealloc - 1);
    if (buf.bytesRead() == 0) {
      return false;
    }
    
    if (_config._delimiters.value().size() == 0) {
      const NoDelimiter comment_del;
      if (_lineending.isLF()) {
        new_data = findDataRows(buf.constData(), buf.begin(), buf.bytesRead(), IsLineBreakLF(_lineending), comment_del);
      } else {
        new_data = findDataRows(buf.constData(), buf.begin(), buf.bytesRead(), IsLineBreakCR(_lineending), comment_del);
      }
    } else if (_config._delimiters.value().size() == 1) {
      const IsCharacter comment_del(_config._delimiters.value()[0].toLatin1());
      if (_lineending.isLF()) {
        new_data = findDataRows(buf.constData(), buf.begin(), buf.bytesRead(), IsLineBreakLF(_lineending), comment_del);
      } else {
        new_data = findDataRows(buf.constData(), buf.begin(), buf.bytesRead(), IsLineBreakCR(_lineending), comment_del);
      }
    } else if (_config._delimiters.value().size() > 1) {
      const IsInString comment_del(_config._delimiters.value());
      if (_lineending.isLF()) {
        new_data = findDataRows(buf.constData(), buf.begin(), buf.begin(), IsLineBreakLF(_lineending), comment_del);
      } else {
        new_data = findDataRows(buf.constData(), buf.begin(), buf.bytesRead(), IsLineBreakCR(_lineending), comment_del);
      }
    }
  } while (buf.bytesRead() == AsciiFileData::Prealloc - 1  && read_completely);

  _rowIndex.resize(_numFrames + 1);

  return new_data;
}

//-------------------------------------------------------------------------------------------
template<class Buffer, typename IsLineBreak, typename CommentDelimiter>
bool AsciiDataReader::findDataRows(const Buffer& buffer, int bufstart, int bufread, const IsLineBreak& isLineBreak, const CommentDelimiter& comment_del)
{
  const IsWhiteSpace isWhiteSpace;
  bool new_data = false;
  bool row_has_data = false;
  bool is_comment = false;
  const int row_offset = bufstart + isLineBreak.size;
  int row_start = bufstart;

  for (int i = 0; i < bufread; i++) {
    if (comment_del(buffer[i])) {
      is_comment = true;
    } else if (isLineBreak(buffer[i])) {
      if (row_has_data) {
        _rowIndex[_numFrames] = row_start;
        ++_numFrames;
        if (_numFrames >= _rowIndex.size()) {
          _rowIndex.resize(_rowIndex.size() + AsciiFileData::Prealloc - 1);
        }
        new_data = true;
        row_start = row_offset+i;
      } else if (is_comment) {
        row_start = row_offset+i;
      }
      row_has_data = false;
      is_comment = false;
    } else if (!row_has_data && !isWhiteSpace(buffer[i]) && !is_comment) {
      row_has_data = true;
    }
  }
  _rowIndex[_numFrames] = row_start;
  return new_data;
}

//-------------------------------------------------------------------------------------------
int AsciiDataReader::readField(const AsciiFileData& buf, int col, double *v, const QString& field, int s, int n)
{
  if (_config._columnType == AsciiSourceConfig::Fixed) {
    MeasureTime t("AsciiSource::readField: same width for all columns");
    LexicalCast lexc;
    lexc.setDecimalSeparator(_config._useDot);
    // &buffer[0] points to first row at _rowIndex[0] , so if we wanna find
    // the column in row i by adding _rowIndex[i] we have to start at:
    const char* col_start = &buf.constData()[0] - _rowIndex[0] + _config._columnWidth * (col - 1);
    for (int i = 0; i < n; ++i) {
      v[i] = lexc.toDouble(_rowIndex[i] + col_start);
    }
    return n;
  } else if (_config._columnType == AsciiSourceConfig::Custom) {
    if (_config._columnDelimiter.value().size() == 1) {
      MeasureTime t("AsciiSource::readField: 1 custom column delimiter");
      const IsCharacter column_del(_config._columnDelimiter.value()[0].toLatin1());
      return readColumns(v, buf.constData(), buf.begin(), buf.bytesRead(), col, s, n, _lineending, column_del);
    } if (_config._columnDelimiter.value().size() > 1) {
      MeasureTime t(QString("AsciiSource::readField: %1 custom column delimiters").arg(_config._columnDelimiter.value().size()));
      const IsInString column_del(_config._columnDelimiter.value());
      return readColumns(v, buf.constData(), buf.begin(), buf.bytesRead(), col, s, n, _lineending, column_del);
    }
  } else if (_config._columnType == AsciiSourceConfig::Whitespace) {
    MeasureTime t("AsciiSource::readField: whitespace separated columns");
    const IsWhiteSpace column_del;
    return readColumns(v, buf.constData(), buf.begin(), buf.bytesRead(), col, s, n, _lineending, column_del);
  }
  return 0;
}

//
// template instantiation chain to generate optimal code for all possible data configurations
//

//-------------------------------------------------------------------------------------------
template<class Buffer, typename ColumnDelimiter>
int AsciiDataReader::readColumns(double* v, const Buffer& buffer, int bufstart, int bufread, int col, int s, int n,
                                 const LineEndingType& lineending, const ColumnDelimiter& column_del)
{
  if (_config._delimiters.value().size() == 0) {
    const NoDelimiter comment_del;
    return readColumns(v, buffer, bufstart, bufread, col, s, n, lineending, column_del, comment_del);
  } else if (_config._delimiters.value().size() == 1) {
    const IsCharacter comment_del(_config._delimiters.value()[0].toLatin1());
    return readColumns(v, buffer, bufstart, bufread, col, s, n, lineending, column_del, comment_del);
  } else if (_config._delimiters.value().size() > 1) {
    const IsInString comment_del(_config._delimiters.value());
    return readColumns(v, buffer, bufstart, bufread, col, s, n, lineending, column_del, comment_del);
  }
  return 0;
}

//-------------------------------------------------------------------------------------------
template<class Buffer, typename ColumnDelimiter, typename CommentDelimiter>
int AsciiDataReader::readColumns(double* v, const Buffer& buffer, int bufstart, int bufread, int col, int s, int n,
                                 const LineEndingType& lineending, const ColumnDelimiter& column_del, const CommentDelimiter& comment_del)
{
  if (_config._columnWidthIsConst) {
    const AlwaysTrue column_withs_const;
    if (lineending.isLF()) {
      return readColumns(v, buffer, bufstart, bufread, col, s, n, IsLineBreakLF(lineending), column_del, comment_del, column_withs_const);
    } else {
      return readColumns(v, buffer, bufstart, bufread, col, s, n, IsLineBreakCR(lineending), column_del, comment_del, column_withs_const);
    }
  } else {
    const AlwaysFalse column_withs_const;
    if (lineending.isLF()) {
      return readColumns(v, buffer, bufstart, bufread, col, s, n, IsLineBreakLF(lineending), column_del, comment_del, column_withs_const);
    } else {
      return readColumns(v, buffer, bufstart, bufread, col, s, n, IsLineBreakCR(lineending), column_del, comment_del, column_withs_const);
    }
  }
}

//-------------------------------------------------------------------------------------------
template<class Buffer, typename IsLineBreak, typename ColumnDelimiter, typename CommentDelimiter, typename ColumnWidthsAreConst>
int AsciiDataReader::readColumns(double* v, const Buffer& buffer, int bufstart, int bufread, int col, int s, int n,
                                 const IsLineBreak& isLineBreak,
                                 const ColumnDelimiter& column_del, const CommentDelimiter& comment_del,
                                 const ColumnWidthsAreConst& are_column_widths_const)
{
  LexicalCast lexc;
  lexc.setDecimalSeparator(_config._useDot);
  const QString delimiters = _config._delimiters.value();

  bool is_custom = (_config._columnType.value() == AsciiSourceConfig::Custom);

  int col_start = -1;
  for (int i = 0; i < n; i++, s++) {
    bool incol = false;
    int i_col = 0;
    
    if (are_column_widths_const()) {
      if (col_start != -1) {
        v[i] = lexc.toDouble(&buffer[0] + _rowIndex[s] + col_start);
        continue;
      }
    }

    v[i] = Kst::NOPOINT;
    for (int ch = _rowIndex[s] - bufstart; ch < bufread; ++ch) {
      if (isLineBreak(buffer[ch])) {
        break;
      } else if (column_del(buffer[ch])) { //<- check for column start
        if ((!incol) && is_custom) {
          ++i_col;
          if (i_col == col) {
            v[i] = NAN;
          }
        }
        incol = false;
      } else if (comment_del(buffer[ch])) {
        break;
      } else {
        if (!incol) {
          incol = true;
          ++i_col;
          if (i_col == col) {
            toDouble(lexc, &buffer[0], bufread, ch, &v[i], i);
            if (are_column_widths_const()) {
              if (col_start == -1) {
                col_start = ch - _rowIndex[s];
              }
            }
            break;
          }
        }
      }
    }
  }
  return n;
}


// vim: ts=2 sw=2 et
