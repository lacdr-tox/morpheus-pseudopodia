#include <QApplication>
#include <QTextCodec>
#define USE_SINGLE_APP
#ifdef USE_SINGLE_APP
#include "qtsingleapp/qtsingleapplication.h"
#endif
#include "mainwindow.h"
#include "sbml_import.h"
#include "uri_handler.h"
#include "version.h"

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
	
	QCoreApplication::setOrganizationName("Morpheus");
	QCoreApplication::setOrganizationDomain("morpheus.org");
	QCoreApplication::setApplicationName("Morpheus");
	QCoreApplication::setApplicationVersion(MORPHEUS_VERSION_STRING);
	
#ifdef WIN32
	//QString app_path = QFileInfo(QApplication::applicationFilePath()).canonicalPath();
// 	QString app_path = ".";
	// QApplication::addLibraryPath(app_path);
	// QApplication::addLibraryPath(app_path + "/PlugIns");
#endif
	// Only allow a single instance of Morpheus
#ifdef USE_SINGLE_APP
	QtSingleApplication a(MORPHEUS_VERSION_STRING, argc, argv);
#else 
	QApplication a(argc, argv);
#endif
	// Handle no gui command line options
// 	QStringList args = QApplication::arguments();
// 	args.pop_front();
	
// 	auto args = parseCMDArgs(argc,argv);
	QCommandLineParser cmd_line;
	parseCmdLine(cmd_line, QApplication::arguments());
	

	 if ( cmd_line.isSet("convert") ) {
		return handleSBMLConvert(cmd_line.value("convert"));
	 }
	
	// Instance forwarding 
	// 
	// if another instance of Morpheus is already running, forward the arguments to that instance
	
#ifdef USE_SINGLE_APP
	QString message;
	if(a.isRunning()){
		// convert all relative paths to absolute paths
		QStringList arguments =  QApplication::arguments();
		arguments.insert(1,"--model-path");
		arguments.insert(2,QDir::currentPath());
		message = arguments.join("@@");
		if (a.sendMessage(message))
			return 0;
	}
#endif

//  Global application configurations
// 	QTextCodec::setCodecForCStrings( QTextCodec::codecForName("UTF-8") );

	QStringList libpaths = QApplication::libraryPaths();
	qDebug() << "Using library Path (should include Qt plugins dir): " << libpaths;

	auto os = QSysInfo::productType();
	if (os=="osx" || os=="macOS" || os=="ios" || os=="windows" || os=="winrt" ) {
		if (a.palette("QWidget").color(QPalette::Base).lightness()>127)
			QIcon::setThemeName("EX-Impression");
		else 
			QIcon::setThemeName("EX-Impression-dark");
	}

    QApplication::setWindowIcon(QIcon(":/morpheus.png") );

#if (QT_VERSION >= QT_VERSION_CHECK(5,12,0))
	QIcon::setFallbackSearchPaths({{":/icons/"}});
	qDebug() << "Using Icon fallback paths" << QIcon::fallbackSearchPaths();
#if (defined USE_QWebEngine)  
	QWebEngineUrlScheme scheme(HelpNetworkScheme::scheme());
	scheme.setSyntax(QWebEngineUrlScheme::Syntax::HostAndPort);
	scheme.setDefaultPort(80);
	scheme.setFlags(QWebEngineUrlScheme::LocalScheme | QWebEngineUrlScheme::LocalAccessAllowed);
	QWebEngineUrlScheme::registerScheme(scheme);
#endif
#endif
	
	
// Create main windows
	QScopedPointer<MainWindow> w(new MainWindow(cmd_line));
	
	w->show();
#ifdef USE_SINGLE_APP
	QObject::connect(&a, SIGNAL(messageReceived(const QString&)),
					 w.data(), SLOT(handleMessage(const QString&)));
#endif
	
	w->move(300, 200);
	w->readSettings();
	
	// These action handlers respond to internal model links openend via QDesktopServices::openUrl().
	auto uri_handler = new uriOpenHandler(&a);
// 	QDesktopServices::setUrlHandler("morph", uri_handler, "processUri");
	QDesktopServices::setUrlHandler("morpheus", uri_handler, "processUri");
	
	// This responds to externally triggered uri open events.
	a.installEventFilter(uri_handler);
	
	auto r = a.exec();
	QDesktopServices::unsetUrlHandler("morpheus");
	return r;
}
