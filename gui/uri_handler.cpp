#include "uri_handler.h"
#include "sbml_import.h"
#include "widgets/webviewer.h"


bool parseCmdLine(QCommandLineParser& parser, const QStringList& arguments) {
	parser.setApplicationDescription("Modelling environment for multicellular systems biology");
	parser.addHelpOption();
	parser.addVersionOption();
	parser.addOptions( {
		{ "convert", "Convert SBML model to MorpheusML", "model_file [, output_file]"},
		{ "import", "Import SBML model", "model_file" },
		{ {"url","u"}, "Open model referred by the URL. May also include processing instructions.", "url"},
		{ "model-path", "Base path to load local models", "path" }
	} );
	parser.addOptions(WebViewer::commandLineOptions());
	parser.addPositionalArgument("model", "Open local model file or url");
	
	parser.process(arguments);
	QString errorMessage;
// 	if (!) {
// 		errorMessage = parser.errorText();
// 		QMessageBox::warning(0, QGuiApplication::applicationDisplayName(),
// 							"<html><head/><body><h2>" + errorMessage + "</h2><pre>"
// 							+ parser.helpText() + "</pre></body></html>");
// 		return false;
// 	}
	
	return true;
}

int handleSBMLConvert(QString arg) {
	// No GUI conversion mode ...
	auto handler = new uriOpenHandler(qApp);
	
	/// create an URITask
	uriOpenHandler::URITask task;
	task.method = uriOpenHandler::URITask::Convert;
	
	QStringList model_names = arg.split(",");
	QRegularExpression scheme("^\\w+:");
	if (scheme.match(model_names[0].trimmed()).hasMatch()) {
		task.m_model_url = model_names[0];
	}
	else {
		task.m_model_url = QUrl::fromLocalFile( model_names[0].trimmed() );
	}
		
	if (model_names.size()>1) {
		task.output_file = model_names[1].trimmed();
	}
	
	handler -> processTask(task, false);
	return 0;
}




bool uriOpenHandler::isValidUrl(const QUrl& url) {
	if ( url.scheme() == "morpheus" || url.fileName().endsWith(".xml") || url.fileName().endsWith(".xml.gz") || url.fileName().endsWith(".xmlz") ) return true;
	else return false;
}

	

void uriOpenHandler::processUri(const QUrl &uri) {
	// either download the file referred
	// handle process tasks ...
	processTask( parseUri(uri), true);
}
	
	
void uriOpenHandler::processTask(URITask task, bool asynch) {
	if (task.method == URITask::None) {
		return;
	}
	qDebug() << "Processing " << task.m_model_url;
	if (task.m_model_url.isLocalFile()) {
		processFile(task);
	}
	else {
		auto net = config::getNetwork();
		if (net) {
			auto reply = net->get(QNetworkRequest(task.m_model_url));
			if (asynch) {
				// Load network ressources asynchronously
				connect(reply, &QNetworkReply::finished, this, [task, reply, this]() { processNetworkReply(task, reply); } );
			}
			else {
				QEventLoop loop;
				connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
				QTimer::singleShot(30000, &loop, &QEventLoop::quit );
				loop.exec();
				processNetworkReply(task,reply);
			}
		}
		else {
			qDebug() << "Could not connect to the network to retrieve " << task.m_model_url;
			return;
		}
	}
}

void uriOpenHandler::processOverrides(URITask& task) {
	if (!task.model) return;
	
	auto global = task.model->find({"MorpheusModel","Global"},true)->getChilds();
	for ( auto par = task.parameter_overrides.cbegin(); par != task.parameter_overrides.cend(); par++ ) {
		bool found = false;
		for (auto& comp : global) {
			if ( comp->attribute("symbol") && comp->attribute("symbol")->get() == par.key() ) {
				if ( comp->attribute("value")) { 
					comp->attribute("value")->set(par.value() );
					found = true;
				}
			}
		}
		if (found) 
			qDebug() << "Overriding parameter " << par.key() << " = " << par.value();
		else 
			qDebug() << "Overriding parameter " << par.key() << " failed ";
	}
}

bool uriOpenHandler::processFile(URITask& task) {
	if (task.method == URITask::Import) {
		task.model = SBMLImporter::importSBML(task.m_model_url.path());
		if (! task.model) {
// 			QMessageBox::critical(nullptr,"URL open", QString("Failed to import the SBML model '%1' to MorpheusML.").arg(task.m_model_url.toString()));
// 			qDebug() << "unable to import " << task.m_model_url;
			return false;
		}
		task.model->xml_file.name = task.m_model_url.fileName();
		processOverrides(task);
		config::importModel(task.model);
		return true;
	}
	else if (task.method == URITask::Convert) {
		// No GUI conversion mode ...
		if (! SBMLImporter::supported) {
			cout << "Cannot convert the SBML file. Morpheus GUI was compiled without SBML support";
			return false;
		}
		task.model = SBMLImporter::convertSBML(task.m_model_url.path());
		if (!task.model) return false;
// 		QStringList model_names = arg.split(",");
// 		auto morpheus_model = SBMLImporter::importSBMLTest( model_names[0] );
// 		if ( ! morpheus_model) {
// 			cerr << "Failed to convert the SBML model to Morpheus." << endl;
// 			return;
// 		}
		auto filename = task.m_model_url.fileName();
		if (task.output_file.isEmpty()) {
			auto idx = filename.lastIndexOf('.');
			if (idx<=0) idx=filename.length();
			task.output_file = filename.left(idx) + "-morpheus-v" + QString::number(task.model->morpheus_ml_version) + ".xml" ;
		}
		
		processOverrides(task);
		task.model->xml_file.save( task.output_file, false);
		return true;
		
	}
	else if (task.method == URITask::Open ) {
		auto model_id = config::openModel(task.m_model_url.path());
		if (model_id<0) {
			qDebug() << "Could not open model " << task.m_model_url;
			return false;
		}
		task.model = config::getOpenModels()[model_id];
		processOverrides(task);
		return true;
	}
	return false;
}


	
void uriOpenHandler::processNetworkReply(URITask task, QNetworkReply* reply) {

	if (reply->error() != QNetworkReply::NoError) {
		if (task.method == URITask::Convert) {
			std::cout << "Failed to fetch the URL '" << task.m_model_url.toString().toStdString();
		}
		else {
			QMessageBox::critical(nullptr,"URL open", QString("Failed to fetch the URL.\n%1").arg(reply->errorString()));
				qDebug() << "unable to import " << reply->request().url();
		}
		return;
	}

	auto data = reply->readAll();
	QString filename;
// 	qDebug() << reply->header(QNetworkRequest::ContentDispositionHeader);
	auto disposition = reply->header(QNetworkRequest::ContentDispositionHeader).toString();
	QRegExp fn_reg("filename=\\\"(.*)\\\""); fn_reg.setMinimal(true);
	if (fn_reg.indexIn(disposition) >= 0) {
		filename = fn_reg.cap(1);
	}
	else {
		filename= task.m_model_url.fileName();
	}
	
	qDebug() << "Received " << data.size() << " bytes from " << task.m_model_url;
	
	//
	int model_id = -1;
	if (task.method == URITask::Import) {
		task.model = SBMLImporter::importSBML(data);
		if (! task.model) {
			QMessageBox::critical(nullptr,"URL open", QString("Failed to import the SBML model '%1' to MorpheusML.").arg(task.m_model_url.toString()));
			qDebug() << "unable to import " << reply->request().url();
			return;
		}
		task.model->xml_file.name = task.m_model_url.fileName();
		processOverrides(task);
		model_id = config::importModel(task.model);
		
	}
	if (task.method == URITask::Convert) {
		// No GUI conversion mode ...
		if (! SBMLImporter::supported) {
			cout << "Cannot convert the SBML file. Morpheus GUI was compiled without SBML support";
			return;
		}
		
		task.model = SBMLImporter::convertSBML(data);
		
// 		QStringList model_names = arg.split(",");
// 		auto morpheus_model = SBMLImporter::importSBMLTest( model_names[0] );
// 		if ( ! morpheus_model) {
// 			cerr << "Failed to convert the SBML model to Morpheus." << endl;
// 			return;
// 		}
		
		if (task.output_file.isEmpty()) {
			auto idx = filename.lastIndexOf('.');
			if (idx<=0) idx=filename.length();
			task.output_file = filename.left(idx) + "-morpheus-v" + QString::number(task.model->morpheus_ml_version) + ".xml" ;
		}
		
		processOverrides(task);
		task.model->xml_file.save( task.output_file, false);
		return;
		
	}
	else if (task.method == URITask::Open ) {
		try {
			task.model = SharedMorphModel( new MorphModel(data));
		}
		catch (...) {
			task.model = nullptr;
		}
		if (! task.model) {
			QMessageBox::critical(nullptr,"URL open", QString("Failed to open the MorpheusML model '%1'.").arg(task.m_model_url.toString()));
			qDebug() << "unable to open " << reply->request().url();
			return;
		}
		task.model->xml_file.name = filename;
		processOverrides(task);
		model_id = config::importModel(task.model);
	}
	
}
	
bool uriOpenHandler::eventFilter(QObject *obj, QEvent *event)
{
	if (event->type() == QEvent::FileOpen)
	{
		QFileOpenEvent* fileEvent = static_cast<QFileOpenEvent*>(event);
		if (!fileEvent->url().isEmpty())
		{
			processUri( fileEvent->url().toString() );
		}
		else if (!fileEvent->file().isEmpty())
		{
			/*auto model_id =*/ config::openModel(fileEvent->file());
		}

		return false;
	}

	return false;
}
	
uriOpenHandler::URITask uriOpenHandler::parseUri(const QUrl& uri)
{
	URITask task;
	task.m_uri = uri;
	if (uri.scheme() == "https" || uri.scheme() == "https" || uri.scheme() == "file" || uri.scheme() == "qrc") {
		task.m_model_url = uri;
		task.method = URITask::Open;
	}
	else if (uri.scheme() == "morpheus") {
		// unpack the uri here
		QUrlQuery query(uri);
		if (uri.host().isEmpty()) {
			if (uri.fileName() == "importSBML" || uri.fileName() == "import") {
				if ( ! query.hasQueryItem("url")) {
					QMessageBox::critical(nullptr,"URL open", QString("Failed to import the SBML model. No 'url' parameter specified."));
					qDebug() << "Missing url for import SBML from schema";
					task.method = URITask::URITask::None;
					return task;
				}
				task.m_model_url = query.queryItemValue("url");
				query.removeAllQueryItems("url");
				task.method = URITask::Import;
			}
			else if (uri.fileName() == "process" || uri.fileName() == "open") {
				
				if ( ! query.hasQueryItem("url")) {
					QMessageBox::critical(nullptr,"URL open", QString("Failed to open MorpheusML model. No 'url' parameter specified."));
					qDebug() << "Missing url for import SBML from schema";
					task.method = URITask::URITask::None;
					return task;
				}
				task.m_model_url = query.queryItemValue("url");
				query.removeAllQueryItems("url");
				task.method = URITask::Open;
			}
			else {
				task.m_model_url = uri;
				task.m_model_url.setScheme("file");
				task.method = URITask::Open;
			}
		}
		else {
			// assume it's an https model link
			task.m_model_url = uri;
			task.m_model_url.setScheme("https");
			task.method = URITask::Open;
		}
		
		if (! query.isEmpty()) {
			// extract Parameter overrides
			auto items = query.queryItems();
			for (auto item = items.begin(); item != items.end(); ) {
				if ( (item->first.startsWith("P(") || item->first.startsWith("p(") ) && item->first.endsWith(")")) {
					task.parameter_overrides[item->first.mid(2,item->first.size()-3)] = item->second;
					item = items.erase(item);
				}
				else {
					item++;
				}
			}
			query.setQueryItems(items);
		}
	}
	
	return task;
}
