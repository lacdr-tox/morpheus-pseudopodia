//////
//
// This file is part of the modelling and simulation framework 'Morpheus',
// and is made available under the terms of the BSD 3-clause license (see LICENSE
// file that comes with the distribution or https://opensource.org/licenses/BSD-3-Clause).
//
// Authors:  Joern Starruss and Walter de Back
// Copyright 2009-2016, Technische Universität Dresden, Germany
//
//////

#ifndef MOPRHEUS_XML_H
#define MOPRHEUS_XML_H

#include <QDomDocument>
#include <QFileDialog>
#include "xsd.h"
#include "zlib.h"

class MorpheusXML {
public:
	MorpheusXML();
	MorpheusXML(QString path);
	MorpheusXML(QDomDocument model);

    QString path;  /*! full path of the file */
    QString name;  /*! plain filename */
	bool is_zipped;
    QDomDocument xmlDocument; /*!< root xml-node of the xml-file. */

    bool save(QString name, bool zip) const;
    /*!<
      Trys to save the current xml-model to file 'fileName'.
      Returns true on success and false if fail.
      */
    bool save() const { return save(path,is_zipped); }
    
    bool saveAsDialog();
    /*!<
      Opens a save-dialog to choose a directory and filename.
      If the current xml-model was successfully written to it, the function returns true
      otherwise it returns false.
      */

    QString domDocToText() const;
    /*!< Returns the structure of the current loaded xml-model as QString. */

    static QString getNewModelName();
    /*!< Returns a name for a new model. */

    bool is_plain_model;

};


#endif // MOPRHEUS_XML_H
