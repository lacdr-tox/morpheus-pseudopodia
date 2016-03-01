#include <QtGui/QApplication>
#include <QTextCodec>
#include "qtsingleapp/qtsingleapplication.h"
#include "mainwindow.h"
using namespace std;

//hauptfunktion für das programm
int main(int argc, char *argv[])
{
	//QApplication a(argc, argv);
	
	//only allow a single instance of Morpheus
	QtSingleApplication a(argc, argv);
	
	// if another instance of Morpheus is already running, 
	//  send the arguments to that instance 
	QString message;
	if(a.isRunning()){
		for (int i = 1; i < argc; ++i) {
			message = QDir::currentPath() + " ";
			message += argv[i];
			if (i < argc-1)
				message += " ";
		}
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
