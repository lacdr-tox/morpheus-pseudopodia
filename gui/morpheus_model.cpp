#include "morpheus_model.h"
#include "config.h"

QList<QString> model_parts() {
	QList<QString> parts;
	parts << "Description" << "Space" << "Time" << "Global" << "CellTypes" << "CPM";
	parts << "CellPopulations" << "Analysis";
	return parts;
}
const QList<QString> MorphModelPart::all_parts_sorted = model_parts();

QMap<QString,int> model_part_indices() {
	QMap<QString,int> indices;
	auto parts = model_parts();
	for (uint i=0; i<parts.size(); i++) {
		indices[parts[i]] = i;
	}
	return indices;
}

const QMap<QString,int> MorphModelPart::all_parts_index = model_part_indices();

//------------------------------------------------------------------------------

MorphModel::MorphModel(QObject *parent) :
    QAbstractItemModel(parent)
{
    sweep_lock = false;

    //XML in datenstruktur einlesen (nodeController)+
	// xml_file by default contains an empty model ...
	try {
		rootNodeContr = new nodeController(NULL, xml_file.xmlDocument.documentElement());
		rootNodeContr->setParent(this);
	}
	catch (...) {
		qDebug() << "Error creating MorpheusModel from plain template ... ";
		throw;
	}
	rootNodeContr->attribute("version")->set(QString::number(morpheus_ml_version));

    // nodeControllers created all required elements
    // Now we clear the history of changes ...
    rootNodeContr->saved();
    rootNodeContr->clearTrackedChanges();
	
    initModel();
}

//------------------------------------------------------------------------------

MorphModel::MorphModel(QString xmlFile, QObject *parent)
try : QAbstractItemModel(parent),  xml_file(xmlFile) 
{
    sweep_lock = false;

	if (xml_file.xmlDocument.documentElement().nodeName() != "MorpheusModel")
        throw ModelException(ModelException::UndefinedNode, QString("File \"%1\" has wrong wrong root node '%2'.\nExpected 'MorpheusModel'!").arg(xmlFile,xml_file.xmlDocument.nodeName()));

    QList<MorphModelEdit> edits = applyAutoFixes(xml_file.xmlDocument);

    //XML in datenstruktur einlesen (nodeController)
    rootNodeContr = new nodeController(NULL, xml_file.xmlDocument.documentElement());
	rootNodeContr->setParent(this);

    ModelDescriptor& desc = const_cast<ModelDescriptor&>(rootNodeContr->getModelDescr());
    for (int i=0; i<edits.size();i++) {
        desc.auto_fixes.append(edits[i]);
        desc.change_count++;
    }

    initModel();
	if (!edits.empty()) 
		QMessageBox::warning(qApp->activeWindow(),
			"Auto Fixes",
			QString("Your Morpheus model was upgraded to the current version %1.\nSee the FixBoard for details.\nModel elements that could not be converted were removed.").arg(morpheus_ml_version) );
}
catch (QString e) {
	throw ModelException(ModelException::FileIOError, e);
}

MorphModel::MorphModel(QDomDocument model, QObject *parent) :
QAbstractItemModel(parent), xml_file(model)
{
    sweep_lock = false;

    if (xml_file.xmlDocument.documentElement().nodeName() != "MorpheusModel")
        throw ModelException(ModelException::UndefinedNode, QString("Imported Document has wrong wrong root node '%1'.\nExpected 'MorpheusModel'!").arg(xml_file.xmlDocument.nodeName()));
	
    QList<MorphModelEdit> edits = applyAutoFixes(xml_file.xmlDocument);

	

    //XML in datenstruktur einlesen (nodeController)
    rootNodeContr = new nodeController(NULL, xml_file.xmlDocument.documentElement());
	rootNodeContr->setParent(this);

    ModelDescriptor& desc = const_cast<ModelDescriptor&>(rootNodeContr->getModelDescr());
    for (int i=0; i<edits.size();i++) {
        desc.auto_fixes.append(edits[i]);
        desc.change_count++;
    }
    
	initModel();

	if (!edits.empty()) 
		QMessageBox::warning(qApp->activeWindow(),
			"Auto Fixes",
			QString("Your Morpheus model was upgraded to the current version %1.\nSee the FixBoard for details.\nModel elements that could not be converted were removed.").arg(morpheus_ml_version) );
}

MorphModel::~MorphModel()
{
	qDebug() << "Cleaning up model " << this->rootNodeContr->getModelDescr().title;
	qDebug() << "Cleaning up path " << temp_folder;
	if (temp_folder.exists()) {
		qDebug() << "Deleting " << temp_folder;
		removeDir(temp_folder.absolutePath());
	}
}


bool MorphModel::removeDir(QString dir_path)
{
	bool result = true;
	QDir dir (dir_path);
	if (dir.exists()) {
		auto dir_list = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst);
		for (const auto& info : dir_list) {
			if (info.isDir()) {
				result = removeDir(info.absoluteFilePath());
			}
			else {
				result = QFile::remove(info.absoluteFilePath());
			}

			if (!result) {
				return result;
			}
		}
		result = dir.rmdir(dir.absolutePath());
	}
	return result;
}

QString MorphModel::getDependencyGraph()
{
	if (!temp_folder.exists()) {
		temp_folder.mkpath(temp_folder.absolutePath());
		if (!temp_folder.exists()) {
			qDebug() << "Cannot create temporary path " << temp_folder.absolutePath();
			return "";
		}
	}
	// If the model did not change the model since the last rendering, just take the old rendering.
	if (dep_graph_model_edit_stamp == rootNodeContr->getModelDescr().edits) {
		QString graph_file = "dependency_graph.svg";
		if (temp_folder.exists(graph_file)) {
			return temp_folder.absoluteFilePath(graph_file);
		}
		graph_file = "dependency_graph.png";
		if (temp_folder.exists(graph_file)) {
			return temp_folder.absoluteFilePath(graph_file);
		}
	}
	
	QString model_file = temp_folder.absoluteFilePath("model.xml");

	auto system_files = this->rootNodeContr->getModelDescr().sys_file_refs;
	QList<QString> original_paths;
	QDir model_dir = QFileInfo(this -> xml_file.path).absoluteDir();
	
	// Convert file references into absolute paths
	for (uint i=0; i<system_files.size(); i++ ) {
		original_paths.push_back(system_files[i]->get());
		if (system_files[i]->isActive() && ! system_files[i]->isDisabled()) {
			QFileInfo file(model_dir, system_files[i]->get().trimmed());
			QString file_path = file.absoluteFilePath();
			qDebug() << file_path;
			
			if (file_path.startsWith(":/")) {
				// the file is in the resources, we need to copy it
				QFile::copy(file_path, temp_folder.absoluteFilePath(file.fileName()));
				system_files[i]->set(file.fileName());
			}
			else {
				system_files[i]->set(file_path);
			}
		}
	}
	
	//Run mopsi on it
	xml_file.save(model_file);
	
	// revert file ref absolute paths
	for (uint i=0; i<system_files.size(); i++ ) {
		system_files[i]->set(original_paths[i]);
	}
	
	QProcess process;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("OMP_NUM_THREADS", "1");
    process.setProcessEnvironment(env);

	QString command = config::getPathToExecutable( config::getApplication().local_executable );
    if (command.isEmpty()) {
		return "";
	}
	
	QStringList arguments;
	if ( ! config::getApplication().local_GnuPlot_executable.isEmpty() )
		arguments << "-gnuplot-path" << config::getApplication().local_GnuPlot_executable;
#ifdef Q_OS_WIN32
	else if ( ! config::getPathToExecutable("gnuplot").isEmpty())
		arguments << "-gnuplot-path" << config::getPathToExecutable("gnuplot");
#endif
	
    arguments << "-symbol-graph" << " " << model_file;

    process.setWorkingDirectory(temp_folder.absolutePath());
	process.setStandardOutputFile(temp_folder.absoluteFilePath("model.out"));

    // run morpheus
	process.start(command,arguments);
	if (! process.waitForFinished(2000)) {
		process.close();
		process.kill();
	}
	
	QString graph_file = "dependency_graph.svg";
	if (temp_folder.exists(graph_file)) {
		dep_graph_model_edit_stamp =  rootNodeContr->getModelDescr().edits;
		return temp_folder.absoluteFilePath(graph_file);
	}
	graph_file = "dependency_graph.png";
	if (temp_folder.exists(graph_file)) {
		dep_graph_model_edit_stamp =  rootNodeContr->getModelDescr().edits;
		return temp_folder.absoluteFilePath(graph_file);
	}
	return "";
}


void MorphModel::initModel()
{
	temp_folder = QDir::temp();

	QString name_patt = "morpheus_%1";
	
	int id = 0;
	QString name;
	
	do {
		name = name_patt.arg(QString().setNum(id));
		id ++;
		if (id>10000) break;
	} while (temp_folder.exists(name));
	
	temp_folder.mkpath(name);
	temp_folder.cd(name);
	
	loadModelParts();
	setSupportedDragActions(Qt::CopyAction | Qt::MoveAction);
}


//------------------------------------------------------------------------------

QList<MorphModelEdit>  MorphModel::applyAutoFixes(QDomDocument document) {

	int morpheus_file_version = document.documentElement().attribute("version","1").toInt();
	int fix_version;
	QList<MorphModel::AutoFix> fixes;
	QList<MorphModelEdit> edits;
	
	
	
	if (morpheus_file_version == morpheus_ml_version) {
// 		return edits;
		fix_version=3;
		MorphModel::AutoFix a;
		a.copy = false;
		a.match_path = "MorpheusModel/CPM/ShapeBoundary"; a.move_path = "MorpheusModel/CPM/ShapeSurface"; fixes.append(a);
		a.match_path = "MorpheusModel/Analysis/Gnuplotter/Plot/CellLabels/@symbol-ref";a.move_path = "MorpheusModel/Analysis/Gnuplotter/Plot/CellLabels/@value"; fixes.append(a);
	}
	else if (morpheus_file_version == 2) {
		// return edits;
		fix_version=3;
		MorphModel::AutoFix a;
		a.copy = false;
		a.match_path  = "MorpheusModel/CPM/Interaction/Neighborhood"; a.move_path = "MorpheusModel/CPM/ShapeSurface/Neighborhood"; fixes.append(a);
		
		a.match_path  = "MorpheusModel/CPM/MCSDuration"; a.move_path = "MorpheusModel/CPM/MonteCarloSampler/MCSDuration"; fixes.append(a);
		a.match_path  = "MorpheusModel/CPM/MetropolisKinetics/Neighborhood"; a.move_path = "MorpheusModel/CPM/MonteCarloSampler/Neighborhood"; fixes.append(a);
		a.match_path  = "MorpheusModel/CPM/MetropolisKinetics/@stepper"; a.move_path = "MorpheusModel/CPM/MonteCarloSampler/@stepper"; fixes.append(a);
		a.match_path  = "MorpheusModel/CPM/MetropolisKinetics"; a.move_path = "MorpheusModel/CPM/MonteCarloSampler/MetropolisKinetics"; fixes.append(a);
		a.copy = true;  a.require_path = "MorpheusModel/CPM"; a.match_path  = "MorpheusModel/Space/Lattice/Neighborhood"; a.move_path = "MorpheusModel/CPM/ShapeSurface/Neighborhood"; fixes.append(a); 
		a.copy = false; a.require_path = "";
		
	}
	else if (morpheus_file_version == 1) {
		fix_version=2;
		MorphModel::AutoFix a;
		a.copy = false;
		a.match_path  = "CellularPottsModel"; a.move_path = "MorpheusModel"; fixes.append(a);
		a.match_path  = "MorpheusModel/Lattice/@size"; a.move_path = "MorpheusModel/Space/Lattice/Size/@value";fixes.append(a);
		a.match_path  = "MorpheusModel/Lattice/@structure"; a.move_path = "MorpheusModel/Space/Lattice/@class";fixes.append(a);
		a.match_path  = "MorpheusModel/Lattice/BoundaryConditions"; a.move_path = "MorpheusModel/Space/Lattice/BoundaryConditions";fixes.append(a);
		
		a.match_path  = "MorpheusModel/Simulation/SpaceScale/NodeLength"; a.move_path = "MorpheusModel/Space/Lattice/NodeLength";fixes.append(a);
		a.match_path  = "MorpheusModel/Simulation/SpaceScale/MembraneSize"; a.move_path = "MorpheusModel/Space/MembraneSize";fixes.append(a);

		a.match_path  = "MorpheusModel/Simulation/@start"; a.move_path = "MorpheusModel/Time/StartTime/@value";fixes.append(a);
		a.match_path  = "MorpheusModel/Simulation/@stop"; a.move_path = "MorpheusModel/Time/StopTime/@value";fixes.append(a);
		a.match_path  = "MorpheusModel/Simulation/@save"; a.move_path = "MorpheusModel/Time/SaveInterval/@value";fixes.append(a);
		a.match_path  = "MorpheusModel/Simulation/RandomSeed"; a.move_path = "MorpheusModel/Time/RandomSeed";fixes.append(a);

		a.match_path  = "MorpheusModel/Simulation/MCSDuration"; a.move_path = "MorpheusModel/CPM/MCSDuration";fixes.append(a);
		a.match_path  = "MorpheusModel/Simulation/Title"; a.move_path = "MorpheusModel/Description/Title";fixes.append(a);

		a.match_path  = "MorpheusModel/CellType"; a.move_path = "MorpheusModel/CellTypes/CellType"; fixes.append(a);
		a.match_path  = "MorpheusModel/CellPopulation"; a.move_path = "MorpheusModel/CellPopulations/Population"; fixes.append(a);
		a.match_path  = "MorpheusModel/Interaction"; a.move_path = "MorpheusModel/CPM/Interaction"; fixes.append(a);
		a.match_path  = "MorpheusModel/MetropolisKinetics"; a.move_path = "MorpheusModel/CPM/MetropolisKinetics"; fixes.append(a);
		a.match_path  = "MorpheusModel/Analysis/Logger/Cell"; a.move_path = "MorpheusModel/Analysis/Logger/Input/Cell"; fixes.append(a);
		
// 		a.match_path  = "MorpheusModel/CPM/Interaction/Neighborhood"; a.move_path = "MorpheusModel/Space/Lattice/Neighborhood";fixes.append(a);
		
		a.match_path  = "MorpheusModel/Space/MembraneSize/@value"; a.move_path = "MorpheusModel/Space/MembraneProperty/@resolution";fixes.append(a);
		a.match_path  = "MorpheusModel/Space/MembraneProperty"; a.move_path = "MorpheusModel/Space/MembraneLattice";fixes.append(a);
		
		a.match_path  = "MorpheusModel/CellTypes/CellType/System/Equation"; a.move_path = "MorpheusModel/CellTypes/CellType/System/Rule";fixes.append(a);
		a.match_path  = "MorpheusModel/CellTypes/CellType/Event/Equation"; a.move_path = "MorpheusModel/CellTypes/CellType/Event/Rule";fixes.append(a);

		
		QFile file(":/data/autofix_rules.txt");
		if (file.open(QIODevice::ReadOnly | QIODevice::Text) ) {
			qDebug() << "Autofix rules from file."; 
			
			QTextStream in(&file);
			QStringList paths;
			while(!in.atEnd()) {
				QString line = in.readLine().remove(" ");
				// ignore lines starting with comments
				if( line.startsWith("#") ){
					continue;
				}
				// if encountering empty line, take the last two lines as paths (old, new)
				if( line.isEmpty() ){
					if( paths.length() >= 2 ){
						a.match_path = paths.at( paths.length() - 2 );
						a.move_path  = paths.at( paths.length() - 1 );
						a.copy = false;
						qDebug() << "AutoFix Rule:\nold path: " << a.match_path << "\nnew path: " << a.move_path << "\n\n";
						fixes.append(a);
						paths.clear();
					}
					else{
						qDebug() << "Encountered an empty line without two preceding non-empty lines... I'll assume this is just for layout. However, make sure there are no empty lines in between OLD and NEW path.";
						paths.clear();
					}
				}
				else{
					paths.append( line );
				}
			}
		}
		else{
			qDebug() << "Error: Autofix rules file NOT found."; 
		}
	}
	else {
		throw  ModelException(ModelException::InvalidVersion, QString("Incompatible MorpheusML version %1").arg(morpheus_file_version) );
	}
	qDebug() << QString("Applying fixes from version %1 to %2").arg(morpheus_file_version).arg(fix_version);
	qDebug() << "Number of AutoFix rules = " << fixes.size();
	
	for (int i=0; i<fixes.size(); i++) {
// 		qDebug() << "Autofix rule nr. " << (i+1);
		QList<QDomNode> matches;
		QString search_path = fixes[i].match_path;
		// remove delete 
		search_path.remove(QRegExp("^/|/$| "));
		// replace multiple slashes with single slash
		search_path.replace(QRegExp("/{2,}"),"/");
		qDebug() << "Match path is " << search_path;

		QStringList search_tags = search_path.split("/");
		
		QStringList search_tags_copy = search_tags;
		if (search_tags_copy.first() == document.documentElement().nodeName())
			matches.append( document.documentElement() );
		search_tags_copy.pop_front();
		search_path = QString("/") + search_tags_copy.join("/");
		while ( ! search_tags_copy.empty() && ! matches.empty() ) {
			QList<QDomNode> new_matches;
			if (search_tags_copy.front().startsWith("@")) {
				if (search_tags_copy.front() == "@text") {
					qDebug() << "looking for text ";
					for (int j=0; j<matches.size();j++) {
						Q_ASSERT(matches[j].isElement());
						QDomNode parent = matches[j];
						QDomNode child = parent.firstChild();
						while ( ! child.isNull() ) {
							if (child.isText() &&  ! child.nodeValue().isEmpty() )  {
								new_matches.append(child);
							}
							child = child.nextSibling();
						}
					}
				}
				else {
					qDebug() << "looking for attr " << search_tags_copy.front().replace("@","");
					QString attr_name = search_tags_copy.front().replace("@","");
					for (int j=0; j<matches.size();j++) {
						Q_ASSERT(matches[j].isElement());
						if (matches[j].toElement().hasAttribute(attr_name)) {
							new_matches.append(matches[j].toElement().attributeNode(attr_name));
						}
					}
				}
			}
			else { // if search tag does not start with @
				for (int j=0; j<matches.size();j++) {
					Q_ASSERT(matches[j].isElement());
					QDomNode parent = matches[j];
					QDomNode child = parent.firstChild();
					while ( ! child.isNull() ) {
						if (child.nodeName() == search_tags_copy.front()) {
							new_matches.append(child);
							qDebug() << "Matched " << search_tags_copy.front();
						}
						child = child.nextSibling();
					}
					// we might also check, whether tags==1 and tag is identical to an attribute ...
				}
			}
			matches = new_matches;
			search_tags_copy.pop_front();
		}

		// Creating the new Path and put the nodes there
		QString move_path = fixes[i].move_path;
		move_path.remove(QRegExp("^/|/$| "));
		QStringList move_tags;
		move_tags = move_path.split("/");
		QString require_path = fixes[i].require_path;
		require_path.remove(QRegExp("^/|/$| "));
		QStringList require_tags = require_path.split("/",QString::SkipEmptyParts);
		
		QString new_name = move_tags.back();
		move_tags.pop_back();

		if (move_tags.empty() && matches.size()) {
			// renaming the root node
			if (matches.size()==1 and matches.first() == document.documentElement()) {
				document.documentElement().setTagName(new_name);
				MorphModelEdit ed;
				ed.xml_parent = document.documentElement();
				ed.name = new_name;
				ed.edit_type = NodeRename;
				ed.info = QString("Renamed Root Element into ") + new_name;
				edits.append(ed);
				qDebug() << "AutoFix: Renamed Root Element " << search_path << " into "  << move_path;
			}
			qDebug() << "Cannot move element to root node ";
		} 
		else if (matches.size()) {
			// moving the nodes in tree
			qDebug() << "search "  << search_tags << " move " << move_tags ;
			
			// Creating relative path for checking require_path
			QStringList search_req_tags = search_tags;
			while ( !search_req_tags.isEmpty() && !require_tags.isEmpty() && search_req_tags.front() == require_tags.front()) {
				qDebug() << "Removed leading tag " << search_req_tags.front() << " for relative path";
				search_req_tags.pop_front();
				require_tags.pop_front();
			}
			
			while ( !search_tags.isEmpty() && !move_tags.isEmpty() && search_tags.front() == move_tags.front()) {
				qDebug() << "Removed leading tag " << search_tags.front() << " for relative path";
				search_tags.pop_front();
				move_tags.pop_front();
			}
						
			move_path="";
			for (int i =0; i<search_tags.size()-1; i++) { move_path += "../"; }
			if (move_path.isEmpty()) move_path = "./";
			move_path += move_tags.join("/") + "/" + new_name;
			
			int n_success=0;
			for (int j=0; j<matches.size();j++) {

				MorphModelEdit ed;
				if (!require_tags.isEmpty()) {
					// Check relative required path
					// Relative Path resolution
					QDomElement new_parent = matches[j].parentNode().toElement();
					for (int i =0; i<search_req_tags.size()-1; i++) {
						new_parent = new_parent.parentNode().toElement();
					}
					QStringList require_tags_copy = require_tags;
					bool found_req_path = require_tags_copy.isEmpty();
					while ( ! require_tags_copy.empty() ) {
						if ( new_parent.firstChildElement(require_tags_copy.front()).isNull() ) {
							found_req_path = false;
							break;
						}
						new_parent = new_parent.firstChildElement(require_tags_copy.front());
						require_tags_copy.pop_front();
					}
					if (!found_req_path) {
						qDebug() << "Required path " << require_path << " not found.";
						continue;
					}
				}
				// Relative Path resolution
				QDomElement new_parent = matches[j].parentNode().toElement();
				for (int i =0; i<search_tags.size()-1; i++) {
					new_parent = new_parent.parentNode().toElement();
				}
				
				QStringList move_tags_copy = move_tags;
				while ( ! move_tags_copy.empty() ) {
					if ( new_parent.firstChildElement(move_tags_copy.front()).isNull() )
						new_parent.appendChild(document.createElement(move_tags_copy.front()));
					new_parent = new_parent.firstChildElement(move_tags_copy.front());
					//Q_ASSERT(! dest.isNull());
					move_tags_copy.pop_front();
				}

				if (new_name.startsWith("@")) {
					
					if (new_name == "@text") {
						qDebug() << QString("Text ") + search_path + " was moved to " + move_path;
						// Assign to an attribute ...
						ed.edit_type = AttribRename;
						ed.xml_parent = new_parent;
						ed.info = QString("Text  ") + search_path + " was moved to " + move_path;
						ed.name = new_name;
						ed.name.replace("@","");
						// Create a TextNode Child, if none exists
						QDomText new_text;
						new_text.setNodeValue( matches[j].nodeValue() );
						new_parent.appendChild(new_text);
						
						qDebug() << "new_parent: " << new_parent.tagName();
						qDebug() << "ed.name   : " << ed.name;
						qDebug() << "value     : " << matches[j].nodeValue();
						edits.append(ed);
						
						if (!fixes[i].copy)
							matches[j].parentNode().toElement().removeChild(matches[j]);
					}
					else {
						qDebug() << "New Attribute :" << new_name;
						qDebug() << QString("Attribute  ") + search_path + " was moved to " + move_path;
						// Assign to an attribute ...
						ed.edit_type = AttribRename;
						ed.xml_parent = new_parent;
						ed.info = QString("Attribute  ") + search_path + " was moved to " + move_path;
						ed.name = new_name;
						ed.name.replace("@","");
						new_parent.setAttribute( ed.name, matches[j].nodeValue().replace("\n",""));
						qDebug() << "new_parent: " << new_parent.tagName();
						qDebug() << "ed.name   : " << ed.name;
						qDebug() << "value     : " << matches[j].nodeValue();
						edits.append(ed);
						
						if (!fixes[i].copy)
							matches[j].parentNode().toElement().removeAttribute(matches[j].nodeName());
					}
				}
				else {
					qDebug() << QString("Element ") + search_path + " was moved to " + move_path;
					if (!fixes[i].copy) {
						matches[j].toElement().setTagName(new_name);
						ed.xml_parent = new_parent.appendChild(matches[j]);
						ed.edit_type = NodeMove;
						ed.info = QString("Element  ") + search_path + " was moved to " + move_path;
						ed.name =  matches[j].attributes().namedItem("name").nodeValue();
					}
					else {
						ed.xml_parent = new_parent.appendChild(matches[j].cloneNode());
						ed.xml_parent.toElement().setTagName(new_name);
						ed.edit_type = NodeAdd;
						ed.info = QString("Element  ") + search_path + " was copied to " + move_path;
						ed.name =  matches[j].attributes().namedItem("name").nodeValue();
					}
					
					if (ed.name.isEmpty())
						ed.name = new_name;
					
					edits.append(ed);
					
				}
				n_success++;
			}
			if (matches.size())
				qDebug() << "AutoFix: Moved " << n_success << " element(s) " << " matching " << search_path << " to " << move_path;
		}
	}
	
	document.documentElement().setAttribute( "version",QString::number(fix_version) );
	if (fix_version != morpheus_ml_version) {
		auto new_edits = applyAutoFixes(document);
		edits.append(new_edits);
	}
	return edits;
}

bool MorphModel::close()
{
    bool modelChanged = rootNodeContr->getModelDescr().change_count > 1;

    if ( modelChanged ) {
        // The current model was changed
        QMessageBox msgBox(qApp->activeWindow());
        msgBox.setText(QString("Model %1 has been changed.").arg(xml_file.name));
        msgBox.setInformativeText("Do you want to save changes?");
        msgBox.setStandardButtons( QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Save);
        int answer = msgBox.exec();

        switch(answer){
            case QMessageBox::Cancel:
                return false;

            case QMessageBox::Save:
                {
                    if (xml_file.name.isEmpty()) {
                        if (! xml_file.saveAsDialog() )
                            return false;
                    }
                    else {
                        xml_file.save( xml_file.path );
                    }
                    break;
                }
            case QMessageBox::Discard:
                break;
        }
    }
    return true;
}

//------------------------------------------------------------------------------

QModelIndex MorphModel::index(int row, int column, const QModelIndex &parent) const {
    if (parent.isValid()) {
        if (row<0 || row >= indexToItem(parent)->childs.size()) {
            qDebug() << "Unable to create index for child " << row << "," << column << " of " << indexToItem(parent)->name<< "(" <<indexToItem(parent)->childs.size() <<")";
            return QModelIndex();
        }
        return createIndex(row, column, indexToItem(parent)->childs[row]) ;
    }
    else
    {
        if (row<0 || row >= rootNodeContr->childs.size()) {
            qDebug() << "Unable to create index for root child " << row << "," << column << " of " << rootNodeContr->name<< "(" <<rootNodeContr->childs.size() <<")";
            return QModelIndex();
        }
        return createIndex(row, column, rootNodeContr->childs[row]) ;
    }

}

//------------------------------------------------------------------------------

QModelIndex MorphModel::parent(const QModelIndex &child) const {
   if (child.isValid()) {
       return itemToIndex(indexToItem(child)->parent);
   }
   else
       return QModelIndex();
}

//------------------------------------------------------------------------------

Qt::ItemFlags MorphModel::flags( const QModelIndex & index ) const {

	if (index.isValid()) {
		if (indexToItem(index)->isDisabled())
			return Qt::ItemIsDragEnabled | Qt::ItemIsSelectable |  Qt::ItemIsEnabled;
		else if (indexToItem(index)->isInheritedDisabled()) 
			return Qt::ItemIsSelectable |  Qt::ItemIsEnabled;
		else
			return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled |  Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	}
	else
		return Qt::ItemIsDropEnabled | Qt::ItemIsEnabled ;
}

//------------------------------------------------------------------------------

int MorphModel::columnCount(const QModelIndex &/*parent*/) const {
    return 3;
}

//------------------------------------------------------------------------------

int MorphModel::rowCount(const QModelIndex &parent) const {
	nodeController* node = indexToItem(parent);
    if (node)
        return node->getChilds().size();
    else
        return 0;
}

//------------------------------------------------------------------------------


QVariant MorphModel::data(const QModelIndex &index, int role) const {
	if (role == Qt::DisplayRole) {

		switch(index.column()) {

			case 0: return indexToItem(index)->name;

			case 1: {
				QString val;
				nodeController* node = indexToItem(index);
				AbstractAttribute* name = node->attribute("name"); if (name && ! name->isActive()) name = NULL;
				AbstractAttribute* value = node->attribute("value"); if (value && ! value->isActive()) value = NULL;
				AbstractAttribute* symbol = node->attribute("symbol"); if (symbol && ! symbol->isActive()) symbol = NULL;
				AbstractAttribute* symbol_ref = node->attribute("symbol-ref"); if (symbol_ref && ! symbol_ref->isActive()) symbol_ref = NULL;

				if ( node->name == "DiffEqn" ) {
					val = QString("d%1 / dt = ").arg(symbol_ref->get());
				}
				else if ( node->name== "Equation" || node->name== "Rule" ) {
					val = symbol_ref->get() + " = ";
				}
				else if (node->name== "Function") {
					val = symbol->get() + " = ";
				}
				else if (node->name == "Contact") {
					AbstractAttribute* t1=node->attribute("type1");
					AbstractAttribute* t2=node->attribute("type2");
					if (t1 && t2)  {
						val = t1->get() + " - " + t2->get();
					}
				}
				else if (symbol) {
					if ( value ) 
						val = symbol->get() + " = ";
					else
						val = symbol->get();
					
				}
				else if ( symbol_ref && value ) {
					val = symbol_ref->get() + " = ";
				}
				else {
					if (name) {
						if (symbol)
							val = QString("%2 [%1]").arg(name->get(),symbol->get());
						else
							val = name->get();
					}
					else if (symbol) {
							val = symbol->get();
					}
					else {
						return QVariant();
					}
				}
				return val.replace(QRegExp("\\s+")," ");
			}
			case 2: {
				QString val;
				nodeController* node = indexToItem(index);
				nodeController* text_node(node);
				AbstractAttribute* value = node->attribute("value"); if (value && ! value->isActive()) value = NULL;
				AbstractAttribute* symbol_ref=node->attribute("symbol-ref"); if (symbol_ref && ! symbol_ref->isActive()) symbol_ref = NULL;

//                qDebug() << "node->name: " << node->name  << " | value: "<<  (value?value->get():"N.A.")  << " | symbol-ref: "<< (symbol_ref ? symbol_ref->get() : "N.A.") ;

				QString text("");
				if ( node->name == "Equation" || node->name== "Rule" || node->name == "Function" || node->name == "DiffEqn" || node->name == "InitPDEExpression") {
					text_node = node->firstChild("Expression");
					if (text_node) {
						val = text_node->getText();
						val;
						if (val.length() > 100 )
							val = val.left(96) + " ...";
					}
				}
				else if (node->name == "Expression" ){
					val = "";
				}
				else if ( text_node->hasText() ) {
					if (text.length() > 100 )
						val = text_node->getText().left(96) + " ...";
					else
						val = text_node->getText();
					val;
				}
				else if (value) {
					val = value->get();
				}
				else if (symbol_ref) {
					val = symbol_ref->get();
				}
				else {
					return QVariant();
				}
				return val.replace(QRegExp("\\s+")," ");
			}

		}
	}
	else if (role==Qt::ForegroundRole) {
		nodeController* item = indexToItem(index);
		if (item->isDisabled() || item->isInheritedDisabled()) {
			return QBrush(Qt::darkGray);
		}
		else if (index.column()==0 && ( item->getName() == "System" || item->getName() == "CellType" || item->getName() == "PDE" || item->getName() == "Triggers") )
			return QBrush(QColor(0,100,180));
	}
	else if (role==Qt::FontRole) {
		switch (index.column()) {
			case 1: {
				nodeController* node = indexToItem(index);
//                AbstractAttribute* value=node->attribute("value");
				AbstractAttribute* symbol_ref=node->attribute("symbol-ref");
				if ( symbol_ref && symbol_ref->isActive()) {
					QFont font = QApplication::font();
					font.setItalic(true);
					return font;
				}
			}

		}
	}
	else if (role==Qt::DecorationRole && index.column() == 0) {
		nodeController * node = indexToItem(index);
		if ( node->isDisabled() /*&& ! node->isInheritedDisabled()*/) {
			return QThemedIcon("media-pause",QApplication::style()->standardIcon(QStyle::SP_MediaPause));
		}
		if ( isSweeperAttribute( node->textAttribute())  ) {
			return QThemedIcon("media-seek-forward",QApplication::style()->standardIcon(QStyle::SP_MediaSeekForward));
		}
		if ( node->attribute("value") && isSweeperAttribute( node->attribute("value")) ) {
			return QThemedIcon("media-seek-forward",QApplication::style()->standardIcon(QStyle::SP_MediaSeekForward));
		}
	}

	return QVariant();
}

//------------------------------------------------------------------------------

QVariant MorphModel::headerData( int section, Qt::Orientation orientation, int role ) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        switch (section) {
            case 0: return QVariant("Element");
            case 1: return QVariant("Name/Symbol");
            case 2: return QVariant("Value");
            }
    return QVariant();
}

//------------------------------------------------------------------------------

nodeController* MorphModel::indexToItem(const QModelIndex& idx) const {
    if (idx.isValid()) {
        return static_cast< nodeController* >(idx.internalPointer());
    }
    else {
		return NULL;
    }
}

//------------------------------------------------------------------------------

QModelIndex MorphModel::itemToIndex(nodeController* node) const {
	if (node == NULL) {
// 		qDebug() << "Creating a Index for a NULL node";
		return QModelIndex();
	}
    if (node == rootNodeContr)
        return createIndex(0,0,(void*)node);
    else {
        Q_ASSERT(node->parent);
        int pos = node->parent->childIndex(node);
        if (pos == -1) {
            qDebug() << "Unable to locate child node " << node->name << " " << node;
            return QModelIndex();
        }
        return createIndex(pos,0,(void*)node);
    }
}

//------------------------------------------------------------------------------

QModelIndex MorphModel::insertNode(const QModelIndex &parent, QDomNode child, int pos) {
	nodeController* contr = indexToItem(parent);
	QModelIndex result;
	try {
		if ( ! contr )
			throw ModelException(ModelException::InvalidNodeIndex,QString("MorphModel::insertNode: Request to insert into invalid index!"));
		
		if (! child.isComment() ) {
			nodeController::NodeInfo childInfo = contr->childInformation(child.nodeName());
			if (childInfo.is_exchanging) {
				nodeController* groupChild = contr->findGroupChild(childInfo.group);
				if (groupChild) {
					groupChild->setDisabled(true);
					dataChanged(itemToIndex(groupChild),itemToIndex(groupChild));
					
				}
			} 
			else if (childInfo.maxOccure.toInt() == 1 && contr->firstChild(child.nodeName()) != NULL ) {
					nodeController* uniqueChild = contr->firstChild(child.nodeName());
					uniqueChild->setDisabled(true);
					dataChanged(itemToIndex(uniqueChild),itemToIndex(uniqueChild));
			}
		}
		
		
		if (pos<0 || pos>contr->getChilds().size()) {
			pos = contr->getChilds().size();
		}
		
// 		qDebug() << "Inserting Node " << child.nodeName() << " into " << contr ->getName() << " at " << pos;
		
		beginInsertRows(parent, pos, pos);
		auto child_node = contr->insertChild(child,pos);
		endInsertRows();
		result = itemToIndex(child_node);
	} 
	catch (ModelException e) {
		QMessageBox::critical(NULL,"Insert Node Error",e.message);
	}
	catch (...)  {
		QMessageBox::critical(NULL,"Insert Node Error","Unknown error while inserting a Data Node into the Model");
	}
	return result;
}

//------------------------------------------------------------------------------

QModelIndex MorphModel::insertNode(const QModelIndex &parent, QString child, int pos) {
//    qDebug() << "MorphModel::insertNode at " << parent;
	nodeController* contr = indexToItem(parent);
	QModelIndex result;
	try {
	
		if ( ! contr )
			throw ModelException(ModelException::InvalidNodeIndex,QString("MorphModel::insertNode: Request to insert into invalid index!"));
		nodeController::NodeInfo childInfo = contr->childInformation(child);
		if (childInfo.is_exchanging) {
			nodeController* groupChild = contr->findGroupChild(childInfo.group);
			if (groupChild) {
				groupChild->setDisabled(true);
				dataChanged(itemToIndex(groupChild),itemToIndex(groupChild));
			}
		}
		else if (childInfo.maxOccure.toInt() == 1 && contr->firstChild(child) != NULL ) {
			nodeController* uniqueChild = contr->firstChild(child);
			uniqueChild->setDisabled(true);
			dataChanged(itemToIndex(uniqueChild),itemToIndex(uniqueChild));
		}
		
		if (pos<0 || pos>contr->getChilds().size()) {
			pos = contr->getChilds().size();
		}
// 		qDebug() << "Inserting Node " << child << " into " << contr ->getName() << " at " << pos;
		beginInsertRows(parent, pos, pos);
		auto child_node = contr->insertChild(child,pos);
		endInsertRows();
		result = itemToIndex(child_node);
	} 
	catch (ModelException e) {
		QMessageBox::critical(NULL,"Insert Node Error",e.message);
	}
	catch (QString e) {
		QMessageBox::critical(NULL,"Insert Node Error",e);
	}
	catch (...)  {
		QMessageBox::critical(NULL,"Insert Node Error","Unknown error while inserting a Data Node into the Model");
	}
	return result;
}

//------------------------------------------------------------------------------


void MorphModel::removeNode(const QModelIndex &parent, int row) {
//    qDebug() << "MorphModel::removeNode removing " << indexToItem(parent)->childs[row]->name;
    beginRemoveRows(parent, row, row);
	// --> the row of the view might not be identical to the row in the model !
    indexToItem(parent)->removeChild( row );
    endRemoveRows();
}

void MorphModel::setDisabled(const QModelIndex &node_index, bool disabled) {
	nodeController* node = indexToItem(node_index);
	if (node) {
		const nodeController::NodeInfo& childInfo = node->parent->childInformation(node->getName());
		
		if (childInfo.is_exchanging) {
			qDebug() << "Enabled node is exchanging";
			nodeController* group_node = node->parent->findGroupChild(childInfo.group);
			if (group_node) {
				group_node->setDisabled( ! disabled );
				qDebug() << "Disabling node " <<group_node->getName() ;
				emit dataChanged(itemToIndex(group_node), itemToIndex(group_node));
			}
		}
		else if (childInfo.maxOccure.toInt() == 1 && node->parent->firstChild(node->getName()) != NULL ) {
			nodeController* uniqueChild =  node->parent->firstChild(node->getName());
			uniqueChild->setDisabled(! disabled);
			dataChanged(itemToIndex(uniqueChild),itemToIndex(uniqueChild));
		}
		node->setDisabled(disabled);
		emit dataChanged(node_index, node_index);
	}
}

//------------------------------------------------------------------------------

Qt::DropActions MorphModel::supportedDropActions () const {
    return Qt::CopyAction | Qt::MoveAction;
}

// ------------------------------------------------------------------------------

QStringList MorphModel::mimeTypes () const {
    QStringList types;
    types << "text/plain" << IndexListMimeData::indexList;
    return types;
}

// ------------------------------------------------------------------------------

QMimeData* MorphModel::mimeData(const QModelIndexList &indexes) const {
    if ( ! indexes.empty()) {
        QModelIndex dragIndex = indexes.front();

        IndexListMimeData* m_data = new IndexListMimeData();
        m_data->setIndexList(indexes);

        QDomNode xml = indexToItem(dragIndex)-> xmlNode.cloneNode();
        QDomDocument doc("");
        doc.appendChild(xml);
        m_data->setText(doc.toString());

        return m_data;
    }
    return 0;
}

//------------------------------------------------------------------------------

bool MorphModel::dropMimeData( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & new_parent ) {
    if (action == Qt::MoveAction) {
        if (data->formats().contains(IndexListMimeData::indexList)) {
            const IndexListMimeData* tree_data = qobject_cast<const IndexListMimeData*>(data);
            QModelIndex move_index = tree_data->getIndexList().front();
            nodeController* move_contr = indexToItem(move_index);
//            qDebug() << "unpacked node " << contr->name;
            if (row==-1)
                row = rowCount(new_parent);

            if (move_index.parent() == new_parent) {
                // Remember, that moving the element changes indices of the subsequent elements ...
                int final_row;
                if (move_index.row()<row)
                    final_row = row-1;
                else {
                    final_row = row;
                }
                if (final_row == move_index.row())
                    return true;
//                qDebug() << "Moving child within parent " << move_contr->name << " from " << move_index.row() << " to " << final_row << " of " << rowCount(move_index.parent()); 
                // Obviously, notifiers for the view need the old indices ...
                if ( ! beginMoveRows(move_index.parent(),move_index.row(),move_index.row(),new_parent,row) ) {
					qDebug() << "Dropped DnD move due to invalid beginMoveRows().";
					return false;
                }
					
                // Just reorder the nodes of the parent node controller.
                indexToItem(new_parent)->moveChild(move_index.row(),final_row);
                endMoveRows();
                return true;
            }
            else{
                // Reparent the node ...
                if (indexToItem(new_parent)->canInsertChild(move_contr,row) && ! move_contr->parent->isChildRequired(move_contr) ) {

					qDebug() << "Moving child " << move_contr->name << " from " << move_contr->parent->getName()<< "[" << move_index.row() << "] to " << indexToItem(new_parent)->getName()<< "[" << row << "] of " << rowCount(new_parent);
					
					// Determine wether to disable some other node in the new parent
					if ( ! move_contr->isDisabled() ) {
						nodeController::NodeInfo childInfo = indexToItem(new_parent)->childInformation(move_contr->getName());
						if (childInfo.is_exchanging) {
							nodeController* groupChild = indexToItem(new_parent)->findGroupChild(childInfo.group);
							if (groupChild) {
								groupChild->setDisabled(true);
								dataChanged(itemToIndex(groupChild),itemToIndex(groupChild));
							}
						}
						else if (childInfo.maxOccure.toInt() == 1 && indexToItem(new_parent)->firstChild(move_contr->getName()) != NULL ) {
							nodeController* uniqueChild = indexToItem(new_parent)->firstChild(move_contr->getName());
							uniqueChild->setDisabled(true);
							dataChanged(itemToIndex(uniqueChild),itemToIndex(uniqueChild));
						}
					}
					
					if ( ! beginMoveRows(move_index.parent(),move_index.row(),move_index.row(),new_parent,row) ) {
						qDebug() << "Dropped DnD move due to invalid beginMoveRows().";
						return false;
					}
					
					

					if ( ! indexToItem(new_parent)->insertChild(move_contr,row)) {
						qDebug() << "oops, someting went wrong in move";
					}
					endMoveRows();
                    return true;
                }
                else qDebug() << "Dropped DnD move ...";
            }

        }
        else {
			// External data is always copied
            action = Qt::CopyAction;
        }
    }
    else if (action==Qt::CopyAction) {
        if (data->hasText()) {
            QDomDocument doc;
            doc.setContent(data->text());
			if (indexToItem(new_parent)->canInsertChild(doc.documentElement().nodeName(),row)) {
				return insertNode(new_parent,doc.documentElement(),row).isValid();
			}
			else if ( doc.firstChild().isComment() ) {
				return insertNode(new_parent,doc.firstChild(),row).isValid();
			}
			else {
				qDebug() << data->text();
				qDebug() << doc.firstChild().isComment();
				qDebug() << "Not accepting d&d node " <<doc.documentElement().nodeName() << " for parent " <<indexToItem(new_parent)->getName();
			}
        }
    }
//    internalDragIndex = QModelIndex();
    return false;
}

//------------------------------------------------------------------------------

bool MorphModel::addPart(QString name) {
	if (!MorphModelPart::all_parts_index.contains(name))
		return false;
	int idx = MorphModelPart::all_parts_index.value(name);
	return activatePart(idx);
}


bool MorphModel::activatePart(int idx) {
	if (idx<0 || idx>=parts.size())
		return false;
	auto& part = parts[idx];
	if (!part.enabled) {
		part.element_index = insertNode(itemToIndex(rootNodeContr),part.label);
		if (!part.element_index.isValid())
			return false;
		part.element = indexToItem(part.element_index);
		part.enabled = true;
		emit modelPartAdded(idx);
	}
    return true;
}


bool MorphModel::addPart(QDomNode xml) {
	if (!MorphModelPart::all_parts_index.contains(xml.nodeName()))
		return false;
	
	int idx = MorphModelPart::all_parts_index.value(xml.nodeName());
	auto& part = parts[idx];
	if (!part.enabled) {
		part.element_index = insertNode(itemToIndex(rootNodeContr),xml);
		if (!part.element_index.isValid())
			return false;
		part.element = indexToItem(part.element_index);
		emit modelPartAdded(idx);
	}
    return true;
}

void MorphModel::removePart(QString part) {
	if (!MorphModelPart::all_parts_index.contains(part))
		return;
	removePart(MorphModelPart::all_parts_index.value(part));
}

void MorphModel::removePart(int idx) {
	if (idx<0 || idx>=parts.size())
		return;
	if (parts[idx].enabled) {
		parts[idx].enabled = false;
		parts[idx].element = NULL;
		removeNode(itemToIndex(rootNodeContr),parts[idx].element_index.row());
		emit modelPartRemoved(parts[idx].element_index.row());
	}
}

void MorphModel::loadModelParts() {

	int current_order_id = -1;
	bool need_resort = true;
	const QList<nodeController* >& xml_parts = rootNodeContr->getChilds();
	QMap<QString, nodeController*> named_xml_parts;
	for (auto part : xml_parts) named_xml_parts[part->getName()] = part;
	
	auto all_parts = MorphModelPart::all_parts_sorted;
	
	parts.clear();
	for (int i=0; i< all_parts.size(); i++ ) {
		MorphModelPart part;
		part.label = all_parts[i];
		part.enabled = named_xml_parts.contains(part.label);
		if (part.enabled) {
			part.element = named_xml_parts[part.label];
			part.element_index = itemToIndex(part.element);
		}
		parts.push_back(part);
	}

	MorphModelPart param_sweep;
	param_sweep.label = "ParamSweep";
	param_sweep.enabled = true;
	parts.push_back(param_sweep);
}

//------------------------------------------------------------------------------


bool MorphModel::isSweeperAttribute(AbstractAttribute* attr) const
{
	return param_sweep.contains(attr);
}

//------------------------------------------------------------------------------

void MorphModel::removeSweeperAttribute(AbstractAttribute* attr) {
	param_sweep.removeAttribute(attr);
	disconnect(attr, SIGNAL(deleted(AbstractAttribute*)),&param_sweep,SLOT(removeAttribute(AbstractAttribute*)));
}


bool MorphModel::addSweeperAttribute(AbstractAttribute* attr) {

	param_sweep.addAttribute(attr);
	connect(attr, SIGNAL(deleted(AbstractAttribute*)),&param_sweep,SLOT(removeAttribute(AbstractAttribute*)));
    if ( ! isSweeperAttribute(attr) ) {
        QList<AbstractAttribute*> s;
        s.append(attr);
        return true;
    }
    else return false;
}

bool MorphModel::isEmpty() const {
    return xml_file.is_plain_model && rootNodeContr->getModelDescr().change_count==0;
}

