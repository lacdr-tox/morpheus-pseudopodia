#include "morpheus_xml.h"

MorpheusXML::MorpheusXML() {
    name = getNewModelName();
    path ="";
    is_plain_model = true;
	is_zipped = false;
    QDomDocument cpmDoc( "cpmDocument" );
    QString str("<?xml version=\"1.0\" encoding=\"UTF-8\" ?><MorpheusModel version=\"95782\"></MorpheusModel>");
    cpmDoc.setContent(str);
    xmlDocument = cpmDoc;
};

MorpheusXML::MorpheusXML(QDomDocument model) {
    name = getNewModelName();
    path ="";
    is_plain_model = false;
	is_zipped = false;
    xmlDocument = model;
};

//------------------------------------------------------------------------------

MorpheusXML::MorpheusXML(QString xmlFile) {
    QDomDocument cpmDoc( "cpmDocument" );
	
	
	QMimeDatabase db;
    QMimeType type = db.mimeTypeForFile(xmlFile);
	
	if (type.name()=="application/gzip" || QFileInfo(xmlFile).suffix() == "gz" || QFileInfo(xmlFile).suffix() == "xmlz" ) {
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
		is_zipped = true;
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
		is_zipped = false;
	}
	qDebug() << "Reading of XML-Document succesfully finished!" << endl;

	xmlDocument = cpmDoc;
	is_plain_model = false;
};

//------------------------------------------------------------------------------

bool MorpheusXML::save(QString fileName, bool zip) const {
	QString outputXML = fileName;
	// pull the focus, such that all edit operations finish
// 	if (qApp->focusWidget())
// 		qApp->focusWidget()->clearFocus();

	if (zip) {
		gzFile gzDoc;
		gzDoc = gzopen(outputXML.toStdString().c_str(), "wb");
		if (!gzDoc) {
			qDebug() << "Can't open xml-file " << outputXML << " for saving!";
			return false;
		}
		
		auto raw_text = domDocToText();
		auto written = gzwrite(gzDoc, raw_text.begin(), raw_text.size());
		if (written != raw_text.size()) {
			qDebug() << "Unable to write xml-file " << outputXML << " to disc! (" << written << "!=" << raw_text.size() << ")";
			return false;
		}
		
		gzclose(gzDoc);
	}
	else {
		QFile file(outputXML);
		if ( ! file.open(QIODevice::WriteOnly | QIODevice::Truncate) ) {
			qDebug() << "Can't open xml-file " << outputXML << " for saving!" << endl;
			return false;
		}
		
		QTextStream ts( &file );
		ts.setCodec("UTF-8");
		ts << domDocToText();
		file.close();
	}
	qDebug() << "Saved to " << outputXML << endl;
	return true;
}

//------------------------------------------------------------------------------

bool MorpheusXML::saveAsDialog()
{
	QString directory = ".";
	if ( QSettings().contains("FileDialog/path") ) {
		directory = QSettings().value("FileDialog/path").toString();
	}
	QString fileName = QFileDialog::getSaveFileName(nullptr, "Select xml-file to save configuration!", directory, "Morpheus Model (*.xml);;Compressed Morpheus Model (*.xml.gz)");
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

QByteArray MorpheusXML::domDocToText() const {
    return xmlDocument.toByteArray(4); // subelements are indented with 6 whitespaces
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

