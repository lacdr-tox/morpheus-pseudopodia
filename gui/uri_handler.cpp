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
		if (! SBMLImporter::supported) {
			cout << "Cannot convert the SBML file. Morpheus GUI was compiled without SBML support";
			return -1;
		}
		// No GUI conversion mode ...
		QStringList model_names = arg.split(",");
		auto morpheus_model = SBMLImporter::importSBMLTest( model_names[0] );
		if ( ! morpheus_model) {
			cerr << "Failed to convert the SBML model to Morpheus." << endl;
			return -1;
		}
		
		if (model_names.size()<2) {
			model_names.append( model_names[0].left(5) + "-morpheus-v" + QString::number(morpheus_model->morpheus_ml_version) + ".xml" );
		}
		morpheus_model->xml_file.save( model_names[1], false);
		return 0;
}


// namespace bopt = boost::program_options;
// 
// bopt::variables_map parseCMDArgs(int argc, char *argv[]) {
// 	
// 	bopt::options_description desc("Supported options");
// 	desc.add_options()
// 		("convert", bopt::value<string>(), "SBML model to converted to MorpheusML")
// 		("import", bopt::value<string>(), "SBML model to be imported")
// 		("url", bopt::value<vector<string>>(),"Url or path referring to a model. May also include processing instructions.")
// 		("help,h", "show this help page.");
// 	
// 	// all positional options are treated as url referring a model
// 	bopt::positional_options_description pos_desc;
// 	pos_desc.add("url",-1);
// 	
// 	bopt::variables_map opts;
// 	bopt::store(bopt::command_line_parser(argc, argv).options(desc).positional(pos_desc).run(), opts);
// 	bopt::notify(opts);
// 	return opts;
// }


bool uriOpenHandler::isValidUrl(const QUrl& url) {
	if ( url.scheme() == "morpheus" || url.fileName().endsWith(".xml") || url.fileName().endsWith(".xml.gz") || url.fileName().endsWith(".xmlz") ) return true;
	else return false;
}

void uriOpenHandler::processUri(const QUrl &uri) {
	// either download the file referred
	// handle process tasks ...
	URITask task = parseUri(uri);
	if (task.m_model_url.isLocalFile()) {
		auto model_id = config::openModel(task.m_model_url.path());
		if (model_id<0) {
			qDebug() << "Could not open model " << task.m_model_url;
			return;
		}
		task.model = config::getOpenModels()[model_id];
		processTask(task);
	}
	else {
		// Load network ressources asynchronously
		auto net = config::getNetwork();
		if (net) {
			emit uriOpenProgress(task.m_model_url,0);
			auto reply = net->get(QNetworkRequest(task.m_model_url));
			connect(reply, &QNetworkReply::finished, [=](){ this->uriFetchFinished(task, reply); });
			connect(reply, &QNetworkReply::downloadProgress,
					[=](qint64 bytesReceived, qint64 bytesTotal) { 
// 						this->uriOpenProgress(task.m_model_url, (bytesReceived*100)/bytesTotal) ;
					});
		}
		else {
			qDebug() << "Could not connect to the network to retrieve " << task.m_model_url;
		}
	}
}

void uriOpenHandler::processTask(URITask task) {
}
	
void uriOpenHandler::uriFetchFinished(URITask task, QNetworkReply* reply) {
	auto data = reply->readAll();
	qDebug() << "Received " << data.size() << " bytes from " << task.m_model_url;
	
	//
	int model_id;
	if (task.method == URITask::Import) {
		task.model = SBMLImporter::importSBML(data);
		if (! task.model) {
			qDebug() << "unable to import " << reply->request().url();
			return;
		}
		model_id = config::importModel(task.model);
		
	}
	else if (task.method == URITask::Open ) {
		task.model = SharedMorphModel( new MorphModel(data));
		model_id = config::importModel(task.model);
	}
	
	if (model_id<0) {
		qDebug() << "Could not open model " << task.m_model_url;
		return;
	}
	
	processTask(task);
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
	if (uri.scheme() == "https" || uri.scheme() == "https" || uri.scheme() == "file") {
		task.m_model_url = uri;
		task.method = URITask::Open;
	}
	else if (uri.scheme() == "morpheus") {
		// unpack the uri here
		if (uri.hasQuery()) {
			QUrlQuery query(uri);
			if (uri.fileName() == "importSBML") {
				if (query.hasQueryItem("url")) {
					task.m_model_url = query.queryItemValue("url");
				}
				else {
					qDebug() << "Missing url for import SBML from schema";
				}
				task.method = URITask::Import;
			}
			else if (uri.fileName() == "process" || uri.fileName() == "open") {
				if (query.hasQueryItem("url")) {
					task.m_model_url = query.queryItemValue("url");
				}
				else {
					qDebug() << "Missing url for import SBML from schema";
				}
				task.method = URITask::Open;
			}
			else {
				task.m_model_url = uri;
				task.m_model_url.setScheme("https");
				task.m_model_url.setQuery("");
				task.method = URITask::Open;
				// assume it's an https file link and 
			}
			
			// parse Parameter overrides
			auto items = query.queryItems();
			for (const auto& item : items) {
				if (item.first.startsWith("Param_")) {
					task.parameter_overrides[item.first.mid(6)] = item.second;
				}
			}
			
		}
		else  {
			task.m_model_url = uri;
			task.m_model_url.setScheme("https");
			task.method = URITask::Open;
		}
	}
	
	
	return task;
}
