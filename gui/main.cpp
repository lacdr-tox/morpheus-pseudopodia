#include <QApplication>
#include <QTextCodec>
// #define USE_SINGLE_APP
#ifdef USE_SINGLE_APP
#include "qtsingleapp/qtsingleapplication.h"
#endif
#include "mainwindow.h"
#include "sbml_import.h"

#if (defined USE_QWebEngine) && (QT_VERSION >= QT_VERSION_CHECK(5,12,0))
// #include "network_schemes.h"
#include <QWebEngineUrlScheme>
#endif


int main(int argc, char *argv[])
{
	// Properly initialize X11 multithreading. Fixes problems when x-forwarding the gui to a remote computer.
	QCoreApplication::setAttribute(Qt::AA_X11InitThreads);
	QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
	QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
	
	// Only allow a single instance of Morpheus
#ifdef WIN32
	//QString app_path = QFileInfo(QApplication::applicationFilePath()).canonicalPath();
	QString app_path = ".";
	// QApplication::addLibraryPath(app_path);
	// QApplication::addLibraryPath(app_path + "/PlugIns");
#endif
#ifdef USE_SINGLE_APP
	QtSingleApplication a(argc, argv);
#else 
	QApplication a(argc, argv);
#endif
	// Handle no gui command line options
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
			morpheus_model->xml_file.save(args[1].left(5) + "-morpheus-v" + QString::number(morpheus_model->morpheus_ml_version) + ".xml", false);
		else
			morpheus_model->xml_file.save(args[2], false);
		return 0;
	 }
	
	// Instance forwarding 
	// 
	// if another instance of Morpheus is already running, forward the arguments to that instance
	
#ifdef USE_SINGLE_APP
	QString message;
	if(a.isRunning()){
		message = QDir::currentPath() + " " + args.join(" ");
		if (a.sendMessage(message))
			return 0;
	}
#endif

//  Global application configurations
// 	QTextCodec::setCodecForCStrings( QTextCodec::codecForName("UTF-8") );

	QStringList libpaths = QApplication::libraryPaths();
	qDebug() << "Using library Path (should include Qt plugins dir): " << libpaths;
	QIcon::setFallbackSearchPaths({{":/icons/"}});
	qDebug() << "Using Icon fallback paths" << QIcon::fallbackSearchPaths();

	auto os = QSysInfo::productType();
	if (os=="osx" || os=="macOS" || os=="ios" || os=="windows" || os=="winrt" ) {
		if (a.palette("QWidget").color(QPalette::Base).lightness()>127)
			QIcon::setThemeName("McMojave");
		else 
			QIcon::setThemeName("McMojave-dark");
	}

	QCoreApplication::setOrganizationName("Morpheus");
    QCoreApplication::setOrganizationDomain("morpheus.org");
    QCoreApplication::setApplicationName("Morpheus");
    QApplication::setWindowIcon(QIcon(":/morpheus.png") );

#if (defined USE_QWebEngine) &&  (QT_VERSION >= QT_VERSION_CHECK(5,12,0))
	QWebEngineUrlScheme scheme(HelpNetworkScheme::scheme());
	scheme.setSyntax(QWebEngineUrlScheme::Syntax::HostAndPort);
	scheme.setDefaultPort(80);
	scheme.setFlags(QWebEngineUrlScheme::LocalScheme | QWebEngineUrlScheme::LocalAccessAllowed);
	QWebEngineUrlScheme::registerScheme(scheme);
#endif

	
	
// Create main windows
	MainWindow w;
	
	w.show();
#ifdef USE_SINGLE_APP
	QObject::connect(&a, SIGNAL(messageReceived(const QString&)),
					 &w, SLOT(handleMessage(const QString&)));
#endif
	
	w.move(300, 200);
	w.readSettings();
	w.show();
	return a.exec();
}
