#include "morpheus_xml.h"

MorpheusXML::MorpheusXML() {
    name = getNewModelName();
    path ="";
    is_plain_model = true;
    QDomDocument cpmDoc( "cpmDocument" );
    QString str("<?xml version=\"1.0\" encoding=\"UTF-8\" ?><MorpheusModel version=\"95782\"></MorpheusModel>");
    cpmDoc.setContent(str);
    xmlDocument = cpmDoc;
};

MorpheusXML::MorpheusXML(QDomDocument model) {
    name = getNewModelName();
    path ="";
    is_plain_model = false;
    xmlDocument = model;
};

//------------------------------------------------------------------------------

MorpheusXML::MorpheusXML(QString xmlFile) {
    QDomDocument cpmDoc( "cpmDocument" );
	
	if (QFileInfo(xmlFile).suffix() == "gz") {
		gzFile gzDoc;
		gzDoc = gzopen(xmlFile.toStdString().c_str(), "rb");
		if (gzDoc == NULL ) {
			throw QString("Cannot open file '%1', possibly not a valid gzip file.").arg(qPrintable(xmlFile));
		}
		
		QByteArray data;
		char buff[4097];
		int i; 
		while ((i = gzread(gzDoc, &buff, 4096)) > 0) {
			buff[i] = '\0';
			data.append(buff);
		}
		gzclose(gzDoc);
		
		QString error_msg; int error_line, error_column;
		if ( ! cpmDoc.setContent(data,true, &error_msg, &error_line, &error_column)) {
			throw QString("Unable to create internal DOM structure from \"%1\"!\n\n%2 at line %3, column %4.").arg(xmlFile,error_msg).arg(error_line).arg(error_column);
		}
		path = xmlFile;
		name = QFileInfo(path).fileName();
	}
	else {
		QFile file(xmlFile);
		if ( ! file.open(QIODevice::ReadOnly | QIODevice::Text) ) {
			throw QString("Cannot open file %1.").arg(xmlFile);
		}
		QString error_msg; int error_line, error_column;
		if ( ! cpmDoc.setContent(&file,true, &error_msg, &error_line, &error_column)) {
			file.close();
			throw QString("Unable to create internal DOM structure from \"%1\"!\n\n%2 at line %3, column %4.").arg(xmlFile,error_msg).arg(error_line).arg(error_column);
		}
		file.close();
		path = QFileInfo(xmlFile).absoluteFilePath();
		name = QFileInfo(path).fileName();
	}
	qDebug() << "Reading of XML-Document succesfully finished!" << endl;

	xmlDocument = cpmDoc;
	is_plain_model = false;
};

//------------------------------------------------------------------------------

bool MorpheusXML::save(QString fileName) {
	QString outputXML = fileName;
	// pull the focus, such that all edit operations finish
// 	if (qApp->focusWidget())
// 		qApp->focusWidget()->clearFocus();

	QFile file(outputXML);
	if(file.open(QIODevice::WriteOnly | QIODevice::Truncate) )
	{
		QTextStream ts( &file );
		ts << domDocToText();
		ts.setCodec("UTF-8");
		file.close();
		qDebug() << "Saved to " << outputXML << endl;
		return true;
	}
	else
	{
		qDebug() << "Can't open xml-file for saving!" << endl;
		return false;
	}
}

//------------------------------------------------------------------------------

bool MorpheusXML::saveAsDialog()
{

    QString directory = ".";
    if ( QSettings().contains("FileDialog/path") ) {
        directory = QSettings().value("FileDialog/path").toString();
    }
    QString fileName = QFileDialog::getSaveFileName(qApp->activeWindow(), "Select xml-file to save configuration!", directory, "Configuration Files (*.xml)");
	if (fileName.isEmpty())
		return false;
    // pull the focus, such that all edit operations finish
//    qApp->activeWindow()->setFocus();
	if (!fileName.endsWith(".xml")) {
		fileName.append(".xml");
	}
    QString outputXML = fileName;
    QFile file(outputXML);
    if( !file.open(QIODevice::WriteOnly) )
    {
        return false;
    }
    else
    {
        QTextStream ts( &file );
		ts.setCodec("UTF-8");
        ts << domDocToText();
        file.close();
        this->path = QFileInfo(fileName).filePath();
        this->name = QFileInfo(fileName).fileName();
		qDebug() << "1. FileDialog/path = " << QFileInfo(fileName).dir().path() << endl;
		qDebug() << "2. Writing of XML-file completed!" << endl;
		QSettings().setValue("FileDialog/path", QFileInfo(fileName).dir().path());
		return true;
    }
}

//------------------------------------------------------------------------------

QString MorpheusXML::domDocToText() {
    QDomDocument doc("");
    doc.appendChild(xmlDocument);

    return doc.toString(4); // subelements are indented with 6 whitespaces
}

//------------------------------------------------------------------------------

QString MorpheusXML::getNewModelName() {
    static int new_file_counter = 0;

    QString s = "untitled";
    if (new_file_counter > 0 )
        s .append(QString("_%1").arg(new_file_counter));

    new_file_counter++;
    return s;
}

