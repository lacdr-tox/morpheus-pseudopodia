#include <QtCore/QCoreApplication>
#include <QtGui/QApplication>
#include <QTextCodec>
#include "qtsingleapp/qtsingleapplication.h"
#include "mainwindow.h"
#include "sbml_import.h"


int main(int argc, char *argv[])
{
	QCoreApplication::setAttribute(Qt::AA_X11InitThreads);
	//only allow a single instance of Morpheus
	QtSingleApplication a(argc, argv);
	//QApplication a(argc, argv);
	
	 QStringList args = QApplication::arguments();
	 args.pop_front();
	 
	 if ( ! args.empty() && args[0] == "--convert" ) {
		if (! SBMLImporter::supported) {
			cout << "Cannot convert the SBML file. Morpheus GUI was compiled without SBML support";
			return -1;
		}
		if (args.size()<2) {
			cout << "Need an SBML file to be defined for conversion";
		}
		// No GUI conversion mode ...
		auto morpheus_model = SBMLImporter::importSBMLTest(args[1]);
		if ( ! morpheus_model) {
			cerr << "Failed to convert the SBML model to Morpheus." << endl;
			return -1;
		}
		if (args.size()<3)
			morpheus_model->xml_file.save(args[1].left(5) + "-morpheus-v" + QString::number(morpheus_model->morpheus_ml_version) + ".xml");
		else
			morpheus_model->xml_file.save(args[2]);
		return 0;
	 }
	
	// if another instance of Morpheus is already running, 
	//  send the arguments to that instance 
	QString message;
	if(a.isRunning()){
		message = QDir::currentPath() + " " + args.join(" ");
		if (a.sendMessage(message))
			return 0;
	}
	
	QTextCodec::setCodecForCStrings( QTextCodec::codecForName("UTF-8") );

	
	//anlegen des hauptfensters
	MainWindow w;
	if(a.isRunning()){
		w.handleMessage(message);
	}
	
	w.show();
	QObject::connect(&a, SIGNAL(messageReceived(const QString&)),
					 &w, SLOT(handleMessage(const QString&)));
	
	w.move(300, 200);
    w.readSettings();
    w.show();
    return a.exec();
}
