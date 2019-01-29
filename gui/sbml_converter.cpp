#include "sbml_converter.h"
#include "sbml/extension/SBMLExtensionRegistry.h"
#include "sbml/conversion/SBMLConverterRegistry.h"

/* TODO TODOs
 *
 * Replace the Compartment size Variable, this should not go as a constant.
 * Also compare with the compartment volume Normalisation used by soslib
 *
 * Time variable isolation and replace with the global time variable
 *
 * */

namespace ASTTool {

void renameSymbol(ASTNode* node, const QString& old_name, const QString& new_name ) {
	renameSymbol(node, old_name.toStdString(), new_name.toStdString());
}

void renameSymbol(ASTNode* node, const string& old_name, const string& new_name)
{
	if (node->getType() == AST_NAME && node->getName() == old_name) {
		node->setName(new_name.c_str());
	}
	for (uint i=0; i<node->getNumChildren(); i++) {
		renameSymbol(node->getChild(i),old_name, new_name);
	}
}

void divideSymbol(ASTNode* node, const string& name, const string& by) {
	if (node->getType() == AST_NAME && node->getName() == name) {
		auto sym_node = node->deepCopy();
		auto by_node = new ASTNode(AST_NAME);
		 by_node->setName(by.c_str());
		node->setType(AST_DIVIDE);
		node->addChild(sym_node);
		node->addChild(by_node);
	}
	else if (node->getType() == AST_FUNCTION_DELAY) {
		if (node->getChild(0)->getName() == name) {
		 auto delay = node->deepCopy();
		 node->removeChild(1);
		 node->removeChild(0);
		 node->setType(AST_DIVIDE);
		 node->addChild(delay);
		 auto by_node = new ASTNode(AST_NAME);
		 by_node->setName(by.c_str());
		 node->addChild(by_node);
		}
	}
	else {
		for (uint i=0; i<node->getNumChildren(); i++) {
			divideSymbol(node->getChild(i),name, by);
		}
	}
	
}

void removeUnits(ASTNode* node) {
	if (node->hasUnits())
		node->unsetUnits();

	for (uint i=0; i<node->getNumChildren(); i++) {
		removeUnits(node->getChild(i));
	}
}

void renameTimeSymbol(ASTNode* node, const QString& time_name) {
	if (node->getType() == AST_NAME_TIME) {
		node->setType(AST_NAME);
		node->setName(time_name.toStdString().c_str());
	}
	for (uint i=0; i<node->getNumChildren(); i++) {
		renameTimeSymbol(node->getChild(i),time_name);
	}
	
}

void replaceSymbolByValue(ASTNode* node, const string& name, double value)
{
	if (node->getType() == AST_NAME && node->getName() == name) {
		node->setType(AST_REAL);
		node->setValue(value);
	}
	
	for (uint i=0; i<node->getNumChildren(); i++) {
		replaceSymbolByValue(node->getChild(i), name, value);
	}
}

void replaceFunction(ASTNode* node, FunctionDefinition* function)
{
	if (node->getType() == AST_FUNCTION) {
		if (node->getName() == function->getId()) {
// 			cout << "Found function to replace " << node->getName() << endl;
			if (function->getNumArguments() != node->getNumChildren())
				throw SBMLConverterException(SBMLConverterException::SBML_INVALID, string("Number function arguments does not match its definition."));
			ASTNode* inline_function = function->getMath()->deepCopy();
			for (uint i=0; i<node->getNumChildren(); i++ ) {
// 				cout << "Child " << i << " " << function->getArgument(i)->getName() << "->" << node->getChild(i)->getName() << endl;
				ASTTool::renameSymbol(inline_function, string(function->getArgument(i)->getName()), string(node->getChild(i)->getName()));
			}
			*node = *(inline_function->getChild(inline_function->getNumChildren()-1));
// 			node->swapChildren(inline_function);
// 			cout << SBML_formulaToString(inline_function);
			delete inline_function;
			return;
		}
	}
	for (uint i=0; i<node->getNumChildren(); i++) {
		replaceFunction(node->getChild(i), function);
	}
}

void replaceRateOf(ASTNode* node)
{
#if LIBSBML_VERSION >= LIBSBML_L3V2_VERSION 
	if (node->getType() == AST_FUNCTION_RATE_OF) {
		string symbol = node->getChild(0)->getName();
		node->removeChild(0);
		node->setType(AST_NAME);
		node->setName((symbol+".rate").c_str());
	}
	else {
		for (uint i=0; i<node->getNumChildren(); i++) {
			replaceRateOf(node->getChild(i));
		}
	}
#endif
}


bool containsNaryRelational(ASTNode* m) {
	if (m->getType() == AST_RELATIONAL_EQ || m->getType() == AST_RELATIONAL_LEQ|| m->getType() == AST_RELATIONAL_GEQ|| m->getType() == AST_RELATIONAL_LT || m->getType() == AST_RELATIONAL_GT) {
		if (m->getNumChildren()>2) return true;
	}
	for (uint i=0; i<m->getNumChildren(); i++) {
		if (containsNaryRelational(m->getChild(i)))
			return true;
	}
	return false;
}

void replaceNaryRelational(ASTNode* m) {
	switch (m->getType()) {
		case AST_RELATIONAL_EQ:  m->setType(AST_FUNCTION); m->setName("eq"); break;
		case AST_RELATIONAL_LEQ: m->setType(AST_FUNCTION); m->setName("leq"); break;
		case AST_RELATIONAL_GEQ: m->setType(AST_FUNCTION); m->setName("geq"); break;
		case AST_RELATIONAL_LT:  m->setType(AST_FUNCTION); m->setName("lt"); break;
		case AST_RELATIONAL_GT:  m->setType(AST_FUNCTION); m->setName("gt"); break;
		default: break;
	}
	for (uint i=0; i<m->getNumChildren(); i++) {
		(replaceNaryRelational(m->getChild(i)));
	}
}

void replaceNaryLogical(ASTNode* m) {
		switch (m->getType()) {
		case AST_LOGICAL_AND:  m->setType(AST_FUNCTION); m->setName("and_f"); break;
		case AST_LOGICAL_OR:   m->setType(AST_FUNCTION); m->setName("or_f"); break;
		case AST_LOGICAL_XOR:  m->setType(AST_FUNCTION); m->setName("xor_f"); break;
		case AST_LOGICAL_NOT:  m->setType(AST_FUNCTION); m->setName("not_f"); break;
		default: break;
	}
	for (uint i=0; i<m->getNumChildren(); i++) {
		(replaceNaryLogical(m->getChild(i)));
	}
}

}

QString s2q(const string& s) { return QString::fromStdString(s); }


void  SBMLImporter::replaceDelays(ASTNode* math) {
	if (math->getType() == AST_FUNCTION_DELAY) {
		DelayDef d;
		d.formula = math->getChild(0)->deepCopy();
		d.formula_string = formulaToString(d.formula, false);
		// TODO Assert that the formula is a symbol or deal with formulas in delays properly
		if (math->getChild(1)->isInteger()) {
			d.delay = QString::number(math->getChild(1)->getInteger());
			d.delayed_symbol = d.formula_string + "_" + d.delay;
		}
		else if (math->getChild(1)->isReal()) {
			d.delay = QString::number(math->getChild(1)->getReal());
			d.delayed_symbol = d.formula_string + "_" + d.delay;
		}
		else {
			d.delay = formulaToString( math->getChild(1));
			d.delayed_symbol = d.formula_string.left(4) + "_" + d.delay;
		}
		d.delayed_symbol.remove(QRegExp("[ ()]"));
		d.delayed_symbol.replace(QRegExp("[\\W]"),"_");

		if (!delays.contains(d))
			delays << d;
		
		math->removeChild(1);
		math->removeChild(0);
		math->setType(AST_NAME);
		math->getDefinitionURL()->clear();
		math->setName(d.delayed_symbol.toLatin1());
	}
	
	for (uint i=0; i<math->getNumChildren(); i++) {
		replaceDelays(math->getChild(i));
	}
}

QSharedPointer<MorphModel> SBMLImporter::importSBML() {
	SBMLImporter* importer = new SBMLImporter(nullptr, config::getModel());
	
	if (QDialog::Accepted == importer->exec()) {
		if (importer->haveNewModel())
			return importer->getMorpheusModel();
		else
			return QSharedPointer<MorphModel>();
	}
	else
		return QSharedPointer<MorphModel>();
}

QSharedPointer<MorphModel> SBMLImporter::importSEDMLTest(QString file) {
	SBMLImporter* importer = new SBMLImporter(nullptr, QSharedPointer<MorphModel>::create() );
	
	if (importer->readSEDML(file))
		return importer->getMorpheusModel();
	else
		return QSharedPointer<MorphModel>();
}

QSharedPointer<MorphModel> SBMLImporter::importSBMLTest(QString file) {
	SBMLImporter* importer = new SBMLImporter(nullptr, QSharedPointer<MorphModel>::create() );
	
	if (importer->readSBMLTest(file)) 
		return importer->getMorpheusModel();
	else
		return QSharedPointer<MorphModel>();
}

SBMLImporter::SBMLImporter(QWidget* parent, QSharedPointer< MorphModel > current_model) : QDialog(parent)
{
	this->setMaximumWidth(500);
	this->setMinimumHeight(250);
	QVBoxLayout* layout = new QVBoxLayout(this);
// 	this->setLayout(layout);
// 	auto layout = this->layout();
	


// 	QLabel* header = new QLabel(this);
// 	header->setText("SBML Import");
// 	header->setAlignment(Qt::AlignHCenter);
	setWindowTitle("SBML Import");

// 	layout->addWidget(header);
// 	layout->addSpacing(20);
	layout->addStretch(1);
	QGroupBox* frame = new QGroupBox("", this);

	frame->setLayout(new QHBoxLayout());
	frame->setFlat(true);

	QLabel* excl = new QLabel(this);
	excl->setPixmap(style()->standardIcon(QStyle::SP_MessageBoxWarning).pixmap(50,50));
	frame->layout()->addWidget(excl);

	QLabel* disclaimer = new QLabel(this);
	disclaimer->setWordWrap(true);
	disclaimer->setText(
		"Select an SBML model file to import.\n\n"
		"Note: Not all SBML concepts are supported. Units are discarded. Simulation details are set to default values.");

	frame->layout()->addWidget(disclaimer);
	layout->addWidget(frame);

	layout->addStretch(1);

	QHBoxLayout* path_layout = new QHBoxLayout();
	QLabel* path_label = new QLabel("SBML File ",this);
	path_layout->addWidget(path_label);

	path = new QLineEdit(this);
	path_label->setBuddy(path);
	path_layout->addWidget(path);
	
	QHBoxLayout* celltype_layout = new QHBoxLayout();
	
	into_celltype  = new QComboBox(this);
	into_celltype->addItem("Global (new model)", "new,global");
	into_celltype->addItem("CellType (new model)", "new,celltype,sbml_cell");
	
	if (current_model) {
		into_celltype->insertSeparator(2);
		into_celltype->addItem("Global", "current,global");
		QString new_celltype = "sbml_cell";
		int i=0;
		for (const auto& part :current_model->parts ) {
			if (part.label=="CellTypes" && part.enabled) {
				part.element->getChilds();
				for (auto ct : part.element->activeChilds("CellType")) {
					auto ct_name = ct->attribute("name")->get();
					into_celltype->addItem(
						QString("CellType \"%1\"").arg(ct_name),
						QString("current,celltype,%1").arg(ct_name)
					);
					// Check name to not overlap new celltype naming convention
					if (ct_name == new_celltype && i==0)
						i=1;
					else {
						QRegExp ct_id("sbml_cell_(\\d+)");
						if (ct_id.exactMatch(ct_name)) {
							if (i<ct_id.cap(0).toInt())
								i = ct_id.cap(0).toInt();
						}
					}
				}
			}
		}
		if (i!=0)
			into_celltype->addItem("new CellType", QString("current,celltype,sbml_cell_%1").arg(i));
		else 
			into_celltype->addItem("new CellType", QString("current,celltype,sbml_cell"));
		model = current_model;
	}
	
	QLabel* celltype_label = new QLabel("Import into ",this);	
	celltype_layout->addWidget(celltype_label);
	celltype_layout->addWidget(into_celltype);
	celltype_label->setBuddy(into_celltype);
	
	celltype_layout->addStretch(1);
	

	QPushButton * file_dlg = new QPushButton(this);
	file_dlg->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
	connect(file_dlg,SIGNAL(clicked(bool)),this,SLOT(fileDialog()));
	path_layout->addWidget(file_dlg);
	layout->addLayout(path_layout);
	layout->addLayout(celltype_layout);
	
	layout->addStretch(3);

	QHBoxLayout *bottom = new QHBoxLayout();

	bottom->addStretch(1);

	QPushButton *ok, *cancel;
	ok = new QPushButton( "Import", this );
	connect( ok, SIGNAL(clicked()), SLOT(import()) );
	bottom->layout()->addWidget(ok);

	cancel = new QPushButton( "Cancel", this );
	connect( cancel, SIGNAL(clicked()), SLOT(reject()) );
	bottom->addWidget(cancel);
	layout->addLayout(bottom);
}


void SBMLImporter::fileDialog()
{
	QString directory = ".";
	if ( QSettings().contains("FileDialog/path") ) {
		directory = QSettings().value("FileDialog/path").toString();
	}
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open SBML model"), directory, tr("SBML Files (*.xml)"));
	if (! fileName.isEmpty() ) {
		path->setText(fileName);
		QString path = QFileInfo(fileName).dir().path();
		QSettings().setValue("FileDialog/path", path);
	}
}


void SBMLImporter::import()
{
	try {
		if (readSBML(path->text(), into_celltype->itemData(into_celltype->currentIndex()).toString()))
			accept();
	}
	catch (SBMLConverterException e) {
		qDebug() << "Unable to import SBML due to " <<  s2q(e.type2name())<< " "<< s2q(e.what());
		QMessageBox::critical(this,"SBML Import Error", QString("Unable to import %1 due to the following error:\n%2 - %3").arg(path->text(),s2q(e.type2name()), s2q(e.what())), QMessageBox::Ok);
	}
	catch (QString s) {
		qDebug() << "Unable to import SBML due to " << s;
		QMessageBox::critical (this,"SBML Import Error", QString("Unable to import %1 due to the following error:\nSBML_INTERNAL_ERROR - %2").arg(path->text(),s),QMessageBox::Ok);
	}
	catch (...) {
		qDebug() << "Unable to import SBML due to unknown error";
		QMessageBox::critical (this,"SBML Import Error", QString("Unable to import %1 due to an unknown error:").arg(path->text()),QMessageBox::Ok);
	}
}

bool SBMLImporter::readSBMLTest(QString sbml_file)
{	
	try {
	if ( ! readSBML(sbml_file,QString("current,global")) )
		return false;
	}
	catch (SBMLConverterException& e ) {
		cerr << "Conversion error " << e.type2name() << endl << e.what() << endl;
		return false;
	}
	
	QFileInfo sbml_file_info(sbml_file);
	QString test_name = sbml_file_info.fileName().left(5);
	QString settings_file_name = sbml_file_info.path() + "/" +  test_name + "-settings.txt";
	QFile settings_file( settings_file_name );
	
	if (!settings_file.exists()) {
		cout<< "Unable to find SBML Test settings file " << settings_file_name.toStdString() << endl;
		return false;
	}
	
	settings_file.open(QIODevice::ReadOnly);
	QMap<QString,QString> settings;
	while (1) {
		QString line = settings_file.readLine();
		auto s = line.split(":");
		if (s.size() == 2)
			settings.insert(s[0],s[1].trimmed());
		if (settings_file.atEnd()) break;
	}
	
	auto time = model->rootNodeContr->firstActiveChild("Time");
	time->firstActiveChild("StartTime")->attribute("value")->set(settings["start"]);
	time->firstActiveChild("StopTime")->attribute("value")->set(settings["duration"]);
	time->firstActiveChild("StopTime")->attribute("symbol")->set("stop");
	
	auto system = model->rootNodeContr->firstActiveChild("Global")->firstActiveChild("System");
	system->attribute("solver-eps")->setActive(true);
	system->attribute("solver-eps")->set(settings["relative"]);
	
	auto analysis = model->rootNodeContr->insertChild("Analysis");
		
	auto logger = analysis->insertChild("Logger");
	
	logger->attribute("time-step")->set(QString("stop/")+settings["steps"]);
	logger->attribute("time-step")->setActive(true);
	
	auto sep = logger->firstActiveChild("Output")->firstActiveChild("TextOutput")->attribute("separator");
	sep->set("comma");
	sep->setActive(true);
	
	auto variables = settings["variables"].split(",",QString::SplitBehavior::SkipEmptyParts);
	for (auto& v : variables) { v=v.trimmed(); }

	auto amounts = settings["amount"].split(",",QString::SplitBehavior::SkipEmptyParts);
	for (auto& a : amounts) {
		a = a.trimmed();
		auto i = variables.indexOf(a);
		if (amount_map.contains(a))
			variables[i] = amount_map[a];
	}
	
	auto concentrations = settings["concentration"].split(",",QString::SplitBehavior::SkipEmptyParts);
	for (auto& c : concentrations) {
		c = c.trimmed();
		auto i = variables.indexOf(c);
		if (concentration_map.contains(c))
			variables[i] = concentration_map[c];
	}
	
	auto row_headers = logger->firstActiveChild("Output")->firstActiveChild("TextOutput")->attribute("header-guarding");
	row_headers->set("false");
	row_headers->setActive(true);
	
	auto file = logger->firstActiveChild("Output")->firstActiveChild("TextOutput")->attribute("file-name");
	file->set(test_name + "-morpheus");
	file->setActive(true);

	for (auto var : variables) {
		logger->firstActiveChild("Input")->insertChild("Symbol")->attribute("symbol-ref")->set(var.trimmed());
	}

	
// 	auto plot_logger = analysis->insertChild("Logger");
// 	plot_logger->attribute("time-step")->set(QString("stop/")+settings["steps"]);
// 	plot_logger->attribute("time-step")->setActive(true);
// 	
// 	auto logger_plot = plot_logger->insertChild("Plots")->insertChild("Plot");
// 	logger_plot->firstActiveChild("X-axis")->firstActiveChild("Symbol")->attribute("symbol-ref")->set("time");
	
// 	for (auto var : variables) {
// 		logger_plot->firstActiveChild("Y-axis")->insertChild("Symbol")->attribute("symbol-ref")->set(var.trimmed());
// 	}
	return true;
}

bool SBMLImporter::readSEDML(QString file)
{
	QDomDocument sed_doc(file);
	QString  sbml_file = sed_doc.firstChildElement("listOfModels").firstChildElement("model").attribute("source");
	
	readSBML(sbml_file,QString("current,global"));
	
	
	QMap<QString,QString> outputs;
	
	auto dg = sed_doc.firstChildElement("listOfDataGenerators").firstChildElement("dataGenerator");
	while (!dg.isNull()) {
		// For simplicity assume that there is a simple variable reference, who's id is identical to the variable name
		outputs[dg.attribute("id")] = dg.firstChildElement("listOfVariables").firstChildElement("variable").attribute("id");
		dg = dg.nextSiblingElement("dataGenerator");
	}
	
	auto logger = model->rootNodeContr->firstActiveChild("Analysis")->insertChild("Logger");
	auto sep = logger->firstActiveChild("Output")->firstActiveChild("TextOutput")->attribute("separator");
	sep->setActive(true);
	sep->set("comma");
	
	auto logger_plot = logger->insertChild("Plots")->insertChild("Plot");
	
	// We just register the Plot variables, since those are logged anyways
	auto dataSet = sed_doc.firstChildElement("listOfOutputs").firstChildElement("report").firstChildElement("dataSet");
	if (!dataSet.isNull()) {
		logger_plot->firstActiveChild("Y-axis")->firstActiveChild("Symbol")->attribute("symbol-ref")->set(outputs[dataSet.attribute("dataReference")]);
		dataSet = dataSet.nextSiblingElement("dataSet");
		while (!dataSet.isNull()) {
			// add to the Logger section ....
			logger_plot->firstActiveChild("Y-axis")->insertChild("Symbol")->attribute("symbol-ref")->set(outputs[dataSet.attribute("dataReference")]);
			dataSet = dataSet.nextSiblingElement("dataSet");
		}
	}
	
	return true;
}



bool SBMLImporter::readSBML(QString sbml_file, QString target_code)
{
	compartments.clear();
	species.clear();
	constants.clear();
	variables.clear();
	vars_with_assignments.clear();
	delays.clear();
	diffeqn_map.clear();
	
	constants.insert("avogadro");
	constants.insert("pi");
	constants.insert("exponentiale");
	
	if ( ! QFileInfo(sbml_file).exists() ) {
		throw SBMLConverterException(SBMLConverterException::FILE_READ_ERROR, sbml_file.toStdString());
	}
	SBMLDocument* sbml_doc =0;
	sbml_doc = readSBMLFromFile(sbml_file.toStdString().c_str());
	if (! sbml_doc)
		throw SBMLConverterException(SBMLConverterException::FILE_READ_ERROR, "Cannot read file.");
	
	if (SBMLExtensionRegistry::isPackageEnabled("comp"))
	{
		ConversionProperties props;
		props.addOption("flatten comp");
		SBMLConverter* converter =
		SBMLConverterRegistry::getInstance().getConverterFor(props);

		converter->setDocument(sbml_doc);
		int result = converter->convert();
		if (result != LIBSBML_OPERATION_SUCCESS)
		{
			cerr << "Conversion failed\n";
			sbml_doc->printErrors();
		}
	}
	else {
		cerr << "The version of libsbml being used does not have the comp"
			<< " package extension enabled" << endl;
	}

	Model* sbml_model = sbml_doc->getModel();
	if (! sbml_model)
		throw SBMLConverterException(SBMLConverterException::FILE_READ_ERROR, "File does not contain an SBML model.");
	// Add annotation for new models
	useL3formulas = sbml_model->getLevel() ==3 && sbml_model->getVersion() ==2;
	// Load model, test compatibility

	// Setup target model
	
	QSharedPointer<MorphModel> morph_model;
	auto target = target_code.split(",");
	
	if (target[0] == "new") {
		QString plain_morpheus_model(
"<?xml version='1.0' encoding='UTF-8'?> \
<MorpheusModel version=\"4\"> \
    <Description> \
        <Details>details</Details> \
        <Title>title</Title> \
    </Description> \
    <Space> \
        <Lattice class=\"linear\"> \
            <Neighborhood> \
                <Order>1</Order> \
            </Neighborhood> \
            <Size symbol=\"size\" value=\"1, 0, 0\"/> \
        </Lattice> \
        <SpaceSymbol symbol=\"space\"/> \
    </Space> \
    <Time> \
        <StartTime value=\"0\"/> \
        <StopTime value=\"1\" symbol=\"stop\"/> \
        <TimeSymbol symbol=\"time\"/> \
    </Time> \
    <Analysis> \
        <Logger  time-step=\"stop/500\"> \
            <Input> \
            </Input> \
            <Output> \
                <TextOutput/> \
            </Output> \
            <Plots> \
                <Plot> \
                    <Style style=\"lines\"/> \
                    <Terminal terminal=\"png\"/> \
                    <X-axis> \
                        <Symbol symbol-ref=\"time\"/> \
                    </X-axis> \
                    <Y-axis> \
                    </Y-axis> \
                </Plot> \
            </Plots>  \
        </Logger> \
    </Analysis> \
</MorpheusModel>\"");

		QDomDocument morph_doc;
		morph_doc.setContent(plain_morpheus_model);
		morph_model = QSharedPointer<MorphModel>(new MorphModel(morph_doc));
	}
	else {
		morph_model = model;
	}

	// Setup target scope
	
	if (target[1]=="global" )  {
		morph_model->addPart("Global");
		target_scope = morph_model->rootNodeContr->firstActiveChild("Global");
		if (!target_scope) throw SBMLConverterException(SBMLConverterException::SBML_INTERNAL_ERROR, "Target scope could not be created.");
	}
	else /* (target[2]=="celltype") */ {
		auto cts_part_idx = MorphModelPart::all_parts_index["CellTypes"];
		if (!morph_model->parts[cts_part_idx].enabled) {
			morph_model->activatePart(cts_part_idx);
			// Reuse the default celltype created
			target_scope = morph_model->parts[cts_part_idx].element->firstActiveChild("CellType");
			if (target_scope) {
				target_scope->attribute("name")->set(target[2]);
				target_scope->attribute("class")->set("biological");
			}
		}
		else { // Try to find an existing celltyoe
			
			target_scope = morph_model->rootNodeContr->find(QStringList() << "CellTypes" << QString("CellType[name=%1]").arg(target[2]));
		}
		if (!target_scope) { // create CellType from scratch
			target_scope = morph_model->rootNodeContr->firstActiveChild("CellTypes")->insertChild("CellType");
			if (!target_scope) throw SBMLConverterException(SBMLConverterException::SBML_INTERNAL_ERROR, "Target scope could not be created.");
			
			target_scope->attribute("name")->set(target[2]);
			target_scope->attribute("class")->set("biological");
		}
		
		if (target[0] == "new") {
			morph_model->addPart("CellPopulations");
			auto population = morph_model->rootNodeContr->firstActiveChild("CellPopulations")->firstActiveChild("Population");
			population->attribute("size")->set("1");
			population->attribute("type")->set(target[2]);
		}
	}
	
	// Setup target system 
	
	target_system = target_scope->insertChild("System");
	target_system->attribute("solver")->set("runge-kutta-adaptive");
	auto stop_symbol_attr = morph_model->rootNodeContr->firstActiveChild("Time")->firstActiveChild("StopTime")->attribute("symbol");
	QString stop_symbol;
	if (stop_symbol_attr->isActive()) {
		stop_symbol = stop_symbol_attr->get();
	}
	if (stop_symbol.isEmpty()) {
		stop_symbol_attr->setActive(true);
		stop_symbol_attr->set("stop");
		stop_symbol = "stop";
	}
		
	target_system->attribute("time-step")->set(stop_symbol+"/1000");
	
	if (target[0] == "new") {
		nodeController* description = morph_model->rootNodeContr->firstActiveChild("Description");
		if (!description) throw SBMLConverterException(SBMLConverterException::SBML_INTERNAL_ERROR, "Description node not found." );

		string title;
		if (sbml_doc->isSetName())
			title = sbml_model->getName();
		else
			title = sbml_model->getId();
		description->firstActiveChild("Title")->setText(s2q(title));

		string details = sbml_doc->getNotesString();
		description->firstActiveChild("Details")->setText(s2q(details));
	}
	
	// Convert to target scope
	
	if (sbml_model->isSetConversionFactor())
		model_conversion_factor = s2q(sbml_model->getConversionFactor());

	this->addSBMLFunctions(sbml_model);
	
	this->addSBMLParameters(sbml_model);
	
	if (sbml_model->getNumCompartments()) {
		for (uint i=0; i<sbml_model->getNumCompartments(); i++ ) {
			CompartmendDesc comp;
			comp.name = s2q(sbml_model->getCompartment(i)->getId());

			comp.init_value = sbml_model->getCompartment(i)->getSize();
			if (isnan(comp.init_value)) comp.init_value=1;

			auto init = sbml_model->getInitialAssignment(comp.name.toStdString());
			auto rule=sbml_model->getAssignmentRuleByVariable(comp.name.toStdString());
			if (rule && rule->getMath())
				comp.init_assignment = formulaToString(rule->getMath());
			else if (init && init->getMath())
				comp.init_assignment = formulaToString(init->getMath());

			comp.dynamic = (sbml_model->getCompartment(i)->isSetConstant() && ! sbml_model->getCompartment(i)->getConstant());
			nodeController* compartment_node = target_scope->insertChild((comp.dynamic ? "Variable" : "Constant"));
			compartment_node -> attribute("symbol") -> set(comp.name);
			if (!comp.init_assignment.isEmpty())
				compartment_node -> attribute("value") -> set(comp.init_assignment);
			else
				compartment_node -> attribute("value") -> set(comp.init_value);
			compartments[comp.name] = comp;
			amount_map[comp.name] = comp.name;
		}
	}
	else {
		CompartmendDesc comp;
		comp.name = "";
		comp.init_value = 1;
		comp.dynamic = false;
		comp.init_assignment = "";
		compartments[comp.name] = comp;
	}

	this->addSBMLSpecies(sbml_model);

	this->translateSBMLReactions(sbml_model);

	this->addSBMLRules(sbml_model);

	this->addSBMLEvents(sbml_model);

	// All rate equations are done, now  sanitize unit quantities and compensate for compartment volume changes
	for (auto& comp : compartments) {
		//Manipulate all specie's odes rules in the compartment
		QString rate_name = comp.name + ".rate";
		bool rate_used=false;
		comp.dynamic = diffeqn_map.contains(comp.name);
		
		for (auto species_ode = diffeqn_map.begin(); species_ode !=diffeqn_map.end(); species_ode++) {
			if (!species.contains(species_ode.key())) continue;
			const SpeciesDesc& spec = species[species_ode.key()];
			if (spec.compartment != comp.name) continue;
			
			if (spec.quantity == Quantity::Amount && species_ode.value().rate_quantity == Quantity::Conc) {
				auto scaled_expr = species_ode.value().expression->get();
				scaled_expr = "(" + scaled_expr + ") * "+ comp.name ;
				if (comp.dynamic) {
					// Reduce the rate by the concentration change already contributed by the volume change
					scaled_expr += " + " + species_ode.key() + "*"+ rate_name + "/" + comp.name;
					rate_used = true;
				}
				species_ode.value().expression->set(scaled_expr);
			}

			if (spec.quantity == Quantity::Conc && species_ode.value().rate_quantity == Quantity::Amount) {
				auto scaled_expr = species_ode.value().expression->get();
				scaled_expr = QString("(") + scaled_expr + ") / " + comp.name ;
				if (comp.dynamic) {
					// Account the concentration change contributed by the volume change
					scaled_expr += " - " + species_ode.key() + "*" + rate_name + "/" + comp.name;
					rate_used = true;
				}
				species_ode.value().expression->set(scaled_expr);
			}
		}
		
		
		if (rate_used) {
			// Expose dv/dt
			auto dv = target_system->insertChild("Intermediate");
			dv->attribute("symbol")->set(rate_name);
			dv->attribute("value")->set(diffeqn_map[comp.name].expression->get());
		}
	}

	// Add all delays collected while parsing
	for (const auto& delay : delays) {
// 		bool does_not_exist=delays.contains(delay);
// 		if (does_not_exist) {
			
			bool is_celltype = target[1]!="global";
			QString var_tag_name = is_celltype ? "Property" : "Variable";
			auto delay_property = target_scope->insertChild(is_celltype ? "DelayProperty" : "DelayVariable");
			delay_property->attribute("symbol")->set(delay.delayed_symbol);
			delay_property->attribute("delay")->set(delay.delay);
			
			// TODO This does only work for symbol delays, not with delayed formulas.
			// We must replace *all* symbols in a delayed formula with the respective initAssignments.
			auto init_expression = delay.formula->deepCopy();
			for ( auto var : target_scope->activeChilds(var_tag_name) ) {
				ASTTool::renameSymbol(init_expression, var->attribute("symbol")->get(), QString("(") + var->attribute("value")->get() +")");
			}
			delay_property->attribute("value")->set(formulaToString(init_expression));
			
			auto delay_rule = target_scope->insertChild("Equation");
			delay_rule->attribute("symbol-ref")->set(delay.delayed_symbol);
			delay_rule->firstActiveChild("Expression")->setText(delay.formula_string);
			
// 			delays.append(delay);
// 		}
	}

	this->parseMissingFeatures(sbml_model);
	if (have_events) {
		target_system->attribute("time-step")->set("stop/30000"); //19900
		target_system->attribute("solver")->set("runge-kutta");
	}
	
	
	// Create Analysis section for new models
	
	if (target[0] == "new") {
		nodeController* logger = morph_model->rootNodeContr->firstActiveChild("Analysis")->firstActiveChild("Logger");
		if (!logger) throw SBMLConverterException(SBMLConverterException::SBML_INTERNAL_ERROR, "Logger node not found.");
		if (into_celltype) {
			logger->insertChild("Restriction")->attribute("exclude-medium")->setActive(true);
			logger->firstActiveChild("Restriction")->attribute("exclude-medium")->set("true");
		}

		// add symbols to GLobal and Logger (both in Logger/Input and in Logger/Plots/Plot/Y-axis)
	// 	nodeController* global      = morph_model->rootNodeContr->firstActiveChild("Global");
	// 	if (!global) throw SBMLConverterException(SBMLConverterException::SBML_INTERNAL_ERROR, "Global node not found.");
		nodeController* logger_log  = logger->firstActiveChild("Input");
		nodeController* logger_plot = logger->firstActiveChild("Plots")->firstActiveChild("Plot")->firstActiveChild("Y-axis");

		foreach (const QString& var, variables){
			nodeController* symbol_log = logger_log->insertChild("Symbol");			
			symbol_log->attribute("symbol-ref")->set(var);
			nodeController* symbol_plot = logger_plot->insertChild("Symbol");
			symbol_plot->attribute("symbol-ref")->set(var);
		}
	}
		

	SBMLDocument_free(sbml_doc);

	morph_model->rootNodeContr->clearTrackedChanges();

	// Create notifications for assumed default values
	if (target[0] == "new") {
		morph_model->rootNodeContr->trackNextChange();
		morph_model->rootNodeContr->firstActiveChild("Time")->firstActiveChild("StopTime")->attribute("value")->set("100.0");
	}

	for (int i=0; i<conversion_messages.size(); i++) {
		morph_model->rootNodeContr->trackInformation(conversion_messages[i]);
	}
	
	model = morph_model;
	model_created = (target[0] == "new");
	return true;
	
};

void SBMLImporter::addSBMLSpecies(Model* sbml_model)
{
// 	double compartment_size = sbml_model->getCompartment(0)->getSize();
	bool is_celltype = target_scope->getName() == "CellType";
	QStringList all_species;
	for (uint spec=0; spec<sbml_model->getNumSpecies(); spec++) {
		Species* species = sbml_model->getSpecies(spec);
		SpeciesDesc desc;
		desc.compartment = s2q(species->getCompartment());
		desc.is_const = (species->isSetConstant() && species->getConstant());
		desc.is_boundary = (species->isSetBoundaryCondition() && species->getBoundaryCondition());
		desc.name = s2q(species->getId());
		desc.quantity = Quantity::Amount;
		desc.uses_as_amount = (species->isSetHasOnlySubstanceUnits() && species->getHasOnlySubstanceUnits()); 
		if (desc.uses_as_amount) desc.quantity = Quantity::Amount;
		if (desc.quantity == Quantity::Amount) {
			if (desc.uses_as_amount)
				desc.formula_symbol = desc.name;
			else 
				desc.formula_symbol = QString("(%1/%2)").arg(desc.name).arg(desc.compartment);
		}
		else {
			if (desc.uses_as_amount)
				desc.formula_symbol = QString("(%1*%2)").arg(desc.name).arg(desc.compartment);
			else 
				desc.formula_symbol = desc.name;
		}
		desc.conversion_factor = model_conversion_factor;
		if (species->isSetConversionFactor())
			desc.conversion_factor = s2q(species->getConversionFactor());

		desc.node = target_scope->insertChild((desc.is_const ? "Constant" : (is_celltype ? "Property" : "Variable")));
		this->variables.insert(desc.name);
		this->species[desc.name] = desc;

		desc.node->attribute("symbol")->set(species->getId());
		if (species->isSetName()){
			desc.node->attribute("name")->setActive(true);
			desc.node->attribute("name")->set(species->getName());
		}
		
		QString init_value;
		if (species->isSetInitialConcentration()) {
			init_value = QString::number(species->getInitialConcentration());
			if (desc.quantity == Quantity::Amount)
				init_value = init_value + "*" + desc.compartment;
		}
		else if (species->isSetInitialAmount() ){
			init_value = QString::number(species->getInitialAmount());
			if ( desc.quantity == Quantity::Conc)
				init_value = init_value + "/" + desc.compartment;
		}
		else {
			init_value = "0";
		}
		
		desc.node->attribute("value")->set(init_value);
		
		concentration_map[desc.name] = QString("c") + desc.name;
		auto cFun = target_scope->insertChild("Function");
		cFun->attribute("symbol")->set(concentration_map[desc.name]);
		cFun->attribute("name")->setActive(true);
		cFun->attribute("name")->set(desc.name);
		if (desc.quantity == Quantity::Amount)
			cFun->firstActiveChild("Expression")->setText(desc.name + "/" + desc.compartment);
		else
			cFun->firstActiveChild("Expression")->setText(desc.name);
		
		amount_map[desc.name] = /*QString("a") +*/ desc.name;
// 		auto aFun = target_scope->insertChild("Function");
// 		aFun->attribute("symbol")->set(amount_map[desc.name]);
// 		if (desc.quantity == Quantity::Amount)
// 			aFun->firstActiveChild("Expression")->setText(desc.name);
// 		else
// 			aFun->firstActiveChild("Expression")->setText(desc.name + "*" + desc.compartment);
		
	}
	
	for (auto& specie : species)
	{
		QString init_value;
		auto rule=sbml_model->getAssignmentRuleByVariable(specie.name.toStdString());
		auto init=sbml_model->getInitialAssignmentBySymbol(specie.name.toStdString());
		if (rule && rule->getMath()){
			init_value = formulaToString(rule->getMath());
			if (specie.quantity == Quantity::Amount)
				init_value = QString("(%1*%2)").arg(init_value).arg(specie.compartment);
			specie.node->attribute("value")->set(init_value);
		}
		else if (init && init->getMath()) {
			init_value = formulaToString(init->getMath());
			if (specie.quantity == Quantity::Amount)
				init_value = QString("(%1*%2)").arg(init_value).arg(specie.compartment);
			specie.node->attribute("value")->set(init_value);
		}
	}
};

void SBMLImporter::addSBMLParameters(Model* sbml_model)
{
	bool is_celltype = target_scope->getName() == "CellType";
	for (uint param=0; param<sbml_model->getNumParameters(); param++){
		Parameter* parameter = sbml_model->getParameter(param);

		nodeController* param_node = nullptr;
		if (parameter->isSetConstant() &&  ! parameter->getConstant()) { // this is a variable parameter
			param_node = target_scope->insertChild((is_celltype ? "Property" :  "Variable"));
			variables.insert(s2q(parameter->getId()));

		}
		else {
			param_node = target_scope->insertChild("Constant");
			constants.insert(s2q(parameter->getId()));
		}
		
		param_node->attribute("symbol")->set(parameter->getId());
		if (parameter->isSetName()) {
			param_node->attribute("name")->set(parameter->getName());
			param_node->attribute("name")->setActive(true);
		}
		
		QString init_val;
		auto init=sbml_model->getInitialAssignment(parameter->getId());
		auto rule=sbml_model->getAssignmentRuleByVariable(parameter->getId());
		if (rule && rule->getMath())
			init_val = formulaToString(rule->getMath());
		else if (init && init->getMath())
			init_val = formulaToString(init->getMath());
		else if (parameter->isSetValue())
			init_val = QString::number(parameter->getValue());
		else
			init_val = "0.0";
		param_node->attribute("value")->set(init_val);
	}
}

void SBMLImporter::addSBMLFunctions(Model* sbml_model)
{
	for (uint fun=0; fun<sbml_model->getNumFunctionDefinitions();fun++) {
		FunctionDefinition* function = sbml_model->getFunctionDefinition(fun);
		if (!function->getBody())
			continue;
		auto mo_function = target_scope->insertChild("Function");
		mo_function->attribute("symbol")->set(function->getId());
		mo_function->attribute("name")->set(function->getName());
		mo_function->attribute("name")->setActive(true);
		for (uint i=0; i<function->getNumArguments(); i++ ) {
			auto param = mo_function->insertChild("Parameter",i);
			param->attribute("symbol")->set(function->getArgument(i)->getName());
		}
		mo_function->firstActiveChild("Expression")->setText(formulaToString(function->getBody()));
	}
};

QString SBMLImporter::formulaToString(const ASTNode* math, bool make_concentration)
{
	auto m = unique_ptr<ASTNode>(math->deepCopy());
// 	XMLOutputStream xos(cout);
// 	m->write(xos); 
	ASTTool::removeUnits(m.get());
	
	ASTTool::renameTimeSymbol(m.get(), target_scope->getModelDescr().time_symbol->get());
	
	if (make_concentration) {
		for (const auto& specie : species) {
			if (specie.quantity == Quantity::Amount && ! specie.uses_as_amount)
				ASTTool::divideSymbol(m.get(), specie.name.toStdString(), specie.compartment.toStdString());
		}
	}
	
	replaceDelays(m.get());
	
	ASTTool::replaceRateOf(m.get());
	ASTTool::replaceNaryRelational(m.get());
	ASTTool::replaceNaryLogical(m.get());
	unique_ptr<char[]> c;
#if LIBSBML_VERSION >= LIBSBML_L3PARSER_VERSION
	c = unique_ptr<char[]>(SBML_formulaToL3String(m.get()));
	
#else
	c = unique_ptr<char[]>(SBML_formulaToString(m.get()));
#endif
// 	m->write(xos); 
// 	cout << "\n" << c.get() << endl;;
	
	return QString(c.get());
}


void SBMLImporter::addSBMLRules(Model* sbml_model)
{
	
	for (uint rul=0; rul<sbml_model->getNumRules();rul++) {
		const Rule* rule = sbml_model->getRule(rul);
		
		if (!rule->getMath())
			continue;
		
		auto symbol = s2q(rule->getVariable());
		if (species.count(symbol) && species[symbol].is_const)
			continue;
// 		if (compartments.count(symbol)) {
// 			compartments[symbol].dynamic = true;
// 		}
		nodeController* rule_node;
// 		if (compartments.count(s2q(rule->getVariable()))) {
// 			throw SBMLConverterException(SBMLConverterException::SBML_DYNAMIC_COMPARTMENT, string("Compartment size dynamics currently not supported.") );
// 		}
		
		auto expression = formulaToString(rule->getMath());
		
		
		switch (rule->getTypeCode()) {
			case SBML_ALGEBRAIC_RULE :
				throw SBMLConverterException(SBMLConverterException::SBML_ALGEBRAIC_RULE);
				break;
			case SBML_ASSIGNMENT_RULE :
			{
				
				rule_node = target_scope->insertChild("Equation");
				rule_node->attribute("symbol-ref")->set(symbol);
				
				if ( species.contains(symbol) && species[symbol].quantity == Quantity::Amount && ! species[symbol].uses_as_amount) {
					rule_node->firstActiveChild("Expression")->setText(QString("%1 * (%2)").arg(species[symbol].compartment).arg(expression));
				}
				else
					rule_node->firstActiveChild("Expression")->setText(expression);
				
				if (rule->isSetName()) {
					rule_node->attribute("name")->set(rule->getName());
					rule_node->attribute("name")->setActive(true);
				}
				break;
				
			}
			case SBML_RATE_RULE : {
				rule_node = target_system->insertChild("DiffEqn");
				rule_node->attribute("symbol-ref")->set(symbol);
				
				Quantity quantity = Quantity::Amount;
				if ( species.contains(symbol)) {
					if (! species[symbol].uses_as_amount)
						quantity = Quantity::Conc;
// 					rule_node->firstActiveChild("Expression")->setText(QString("%1 * (%2)").arg(species[symbol].compartment).arg(expression));
				}
				
				rule_node->firstActiveChild("Expression")->setText(expression);
				
				if (rule->isSetName()) {
					rule_node->attribute("name")->set(rule->getName());
					rule_node->attribute("name")->setActive(true);
				}
				diffeqn_map.insert(symbol, { quantity, rule_node->firstActiveChild("Expression")->textAttribute() });
				break;
			}
			default :
				throw SBMLConverterException(SBMLConverterException::SBML_INVALID);
				break;
		}

	}
};

void SBMLImporter::addSBMLEvents(Model* sbml_model)
{
	have_events = sbml_model->getNumEvents()>0;
	
	for (uint i=0; i< sbml_model->getNumEvents(); i++) {
		Event* e = sbml_model->getEvent(i);
		if (! (e->isSetTrigger() && e->getTrigger()->getMath()))
			continue;
			
		nodeController* event = target_scope->insertChild("Event");
		if (e->isSetName()) {
			event->attribute("name")->setActive(true);
			event->attribute("name")->set(s2q(e->getName()));
		}
		if (e->isSetTrigger() && e->getTrigger()->getMath()) {
			nodeController* condition = event->firstActiveChild("Condition");
			if (!condition)
				condition = event->insertChild("Condition");
			condition->setText(formulaToString(e->getTrigger()->getMath()));
			
			if (e->getTrigger()->isSetInitialValue()) {
				condition->attribute("history")->setActive(true);
				condition->attribute("history")->set(e->getTrigger()->getInitialValue() ? "true" : "false");
			}
		}
		if (e->isSetDelay()) {
			event->attribute("delay")->setActive(true);
			if (!e->getDelay()->getMath())
				event->attribute("delay")->set("0");
			else
				event->attribute("delay")->set(formulaToString(e->getDelay()->getMath()));
			if ( e->isSetUseValuesFromTriggerTime() && e->getUseValuesFromTriggerTime() ) {
				event->attribute("compute-time")->setActive(true);
				event->attribute("compute-time")->set("on-trigger");
			}
			else {
				event->attribute("compute-time")->setActive(true);
				event->attribute("compute-time")->set("on-execution");
			}
			
			event->attribute("persistent")->setActive(true);
			event->attribute("persistent")->set((e->getTrigger()->getPersistent() ? "true" : "false"));
		}
		if (e->isSetPriority()) {
			this->conversion_messages.append(
				QString("Dropped unsupported priority element on SBML Event \"%1\"").arg( s2q( (e->isSetId()?e->getId() : e->getName()) ) )
			);
		}
		for (uint j=0; j<e->getNumEventAssignments(); j++) {
			const EventAssignment* ass = e->getEventAssignment(j);
			if (!ass->getMath()) continue;
			nodeController* equation =  event->insertChild("Rule");
			equation->attribute("symbol-ref")->set(ass->getVariable());
			
			auto formula = formulaToString(ass->getMath());
			
			if ( species.contains(s2q(ass->getVariable())) && species[s2q(ass->getVariable())].quantity == Quantity::Amount && !species[s2q(ass->getVariable())].uses_as_amount) {
				formula = QString("%1 * (%2)").arg(species[s2q(ass->getVariable())].compartment).arg(formula);
			}
			equation->firstActiveChild("Expression")->setText(formula);
		}
	}
}


void SBMLImporter::translateSBMLReactions(Model* sbml_model)
{
	// Here we have to take care, that all symbols are meant to be reaction local,
	// i.e. only valid in the scope the current reaction.
	// we thus rename symbols that are already defined, such that they all can be valid on global scope

	QSet<QString> all_symbols_defined = variables + constants + functions.keys().toSet();
	for ( uint r=0; r<sbml_model->getNumReactions(); r++ ) {
		Reaction* reaction = sbml_model->getReaction(r);

		if (reaction->isSetId()) {
			all_symbols_defined.insert(s2q(reaction->getId()));
		}
		for (uint j=0; j<reaction->getNumReactants(); j++) {
			auto reactant = reaction->getReactant(j);
			if ( reactant->isSetId()) {
				all_symbols_defined.insert(s2q(reactant->getId()));
			}
		}
		for (uint j=0; j<reaction->getNumProducts(); j++) {
			auto reactant = reaction->getProduct(j);
			if ( reactant->isSetId()) {
				all_symbols_defined.insert(s2q(reactant->getId()));
			}
		}
	}
	bool have_renamed = false;
	bool is_celltype = target_scope->getName() == "CellType";
	
	// this was inspired by the soslib odeConstruct.c:Species_odeFromReactions();
	for ( uint r=0; r<sbml_model->getNumReactions(); r++ ) {
		Reaction* reaction = sbml_model->getReaction(r);
		KineticLaw* kinetics = reaction->getKineticLaw();
		QMap<QString,QString> renamed_symbols;
		if (!kinetics)
			throw SBMLConverterException(SBMLConverterException::SBML_INVALID, "Missing reaction kinetics");
		if (!kinetics->getMath())
			continue;

		ListOfParameters* local_params = kinetics->getListOfParameters();
		for (uint p=0; p<local_params->size(); p++) {
			Parameter* param = local_params->get(p);
			
			QString param_symbol = s2q(param->getId());
			
			if (all_symbols_defined.contains(param_symbol) ) {
				QString new_symbol= QString("%1_%2").arg(param_symbol).arg(r);
				if (all_symbols_defined.contains(new_symbol)) {
					char suff = 'a';
					QString new_symbol_mod = new_symbol + suff++;
					while (all_symbols_defined.contains(new_symbol_mod)) {
						new_symbol_mod = new_symbol + suff++;
						if (suff == 'Z')
							throw SBMLConverterException(SBMLConverterException::SBML_INTERNAL_ERROR);
					}
					new_symbol = new_symbol_mod;
				}
				renamed_symbols.insert(param_symbol, new_symbol);
				param_symbol = new_symbol;
				have_renamed = true;
			}
			
			constants.insert(param_symbol);
			all_symbols_defined.insert(param_symbol);

			nodeController* param_node = target_scope->insertChild("Constant");
			param_node->attribute("symbol")->set(param_symbol);
			if (param->isSetName()) {
				param_node->attribute("name")->set(param->getName());
				param_node->attribute("name")->setActive(true);
			}
			param_node->attribute("value")->set(param->getValue());
		}
		
		// After parsing the params, we know which symbols to rename in the kinetics
		QMap<QString,QString>::iterator rename;
		auto kinetics_math = unique_ptr<ASTNode>(kinetics->getMath()->deepCopy());
		for (rename = renamed_symbols.begin(); rename!=renamed_symbols.end(); rename++) {
			ASTTool::renameSymbol(kinetics_math.get(), rename.key(), rename.value());
		}
		auto kinetics_formula = formulaToString(kinetics_math.get());
		if (! reaction->getId().empty()) {
			auto rate_function = target_scope->insertChild("Function");
			rate_function->attribute("symbol")->set(reaction->getId());
			rate_function->firstActiveChild("Expression")->setText(kinetics_formula);
		}
		
		// Solve the left hand side -- reactants of the equation
		string reactants_compartment;
		for (uint j=0; j<reaction->getNumReactants(); j++) {
			SpeciesReference* reactant = reaction->getReactant(j);
			QString reactant_name = s2q(reactant->getSpecies());
			if (!species.count(reactant_name))
				throw SBMLConverterException(SBMLConverterException::SBML_INVALID,string("Unknown Species ")+ reactant_name.toStdString());
				
			QString term;

			if ( reactant->isSetId()) {
				// Use a variable for the Stochiometry instead of an expression.
				nodeController* stoch_node = nullptr;
				stoch_node = target_scope->insertChild(is_celltype ? "Property" : "Variable");
				stoch_node->attribute("symbol")->set(reactant->getId());
				variables.insert(s2q(reactant->getId()));
				
				
				QString init_val;
				auto init = sbml_model->getInitialAssignment(reactant->getId());
				auto rule=sbml_model->getAssignmentRuleByVariable(reactant->getId());
				if (rule && rule->getMath()) {
					init_val = formulaToString(rule->getMath());
				}
				else if (init && init->getMath()) {
					init_val = formulaToString(init->getMath());
				}
				else if (reactant->isSetStoichiometryMath()) {
					init_val = formulaToString(reactant->getStoichiometryMath()->getMath());
					auto eq = target_scope->insertChild("Equation");
					eq->attribute("symbol-ref")->set(reactant->getId());
					eq->firstActiveChild("Expression")->setText(init_val);
					vars_with_assignments.insert(s2q(reactant->getId()));
				}
				else {
					init_val = QString::number(reactant->getStoichiometry());
				}
				stoch_node->attribute("value")->set(init_val);
				
				term += s2q(reactant->getId());
				
			}
			else {
				if (reactant->isSetStoichiometryMath()) {
					term += "(" + formulaToString( reactant->getStoichiometryMath()->getMath() ) + ")" ;
				}
				else {
					term += "(" + QString::number(reactant->getStoichiometry()) + ")" ;
				}
			}
			
			if (!species[reactant_name].conversion_factor.isEmpty()) {
				term += " * " + species[reactant_name].conversion_factor;
			}
			term += " * (" + kinetics_formula + ")";
			
						// Skip dynamics of const species 
			if (species[reactant_name].is_const || species[reactant_name].is_boundary) {
				continue;
			}
			// Skip dynamics of species with direct assignments
			if (vars_with_assignments.contains(reactant_name))
				continue;
			
			if (! diffeqn_map.contains(reactant_name)) {
				nodeController* deq_node = target_system->insertChild("DiffEqn");
				deq_node->attribute("symbol-ref")->set(reactant_name);
				deq_node->attribute("name")->set("gained from reactions");
				deq_node->attribute("name")->setActive(true);

				diffeqn_map.insert(reactant_name, { Quantity::Amount, deq_node->firstActiveChild("Expression")->textAttribute() } );
				diffeqn_map[reactant_name].expression->set(QString("- ") + term);
			}
			else {
				diffeqn_map[reactant_name].expression->append(" - " + term);
			}

		}

		// Solve the right hand side -- products of the reaction
		for (uint j=0; j<reaction->getNumProducts(); j++) {
			SpeciesReference* product = reaction->getProduct(j);
			QString product_name = s2q(product->getSpecies());
			if (!species.count(product_name))
				throw SBMLConverterException(SBMLConverterException::SBML_INVALID,string("Unknown Species ")+ product_name.toStdString());
			

			QString term;
			if ( product->isSetId()) {
				// Use a variable for the Stochiometry instead of an expression.
				nodeController* stoch_node = nullptr;
				stoch_node = target_scope->insertChild(is_celltype ? "Property" : "Variable");
				stoch_node->attribute("symbol")->set(product->getId());
				variables.insert(s2q(product->getId()));
				
				QString init_val;
				auto init = sbml_model->getInitialAssignment(product->getId());
				auto rule=sbml_model->getAssignmentRuleByVariable(product->getId());
				if (rule && rule->getMath()) {
					init_val = formulaToString(rule->getMath());
				}
				else if (init && init->getMath()) {
					init_val = formulaToString(init->getMath());
				}
				else if (product->isSetStoichiometryMath()) {
					init_val = formulaToString(product->getStoichiometryMath()->getMath());
					auto eq = target_scope->insertChild("Equation");
					eq->attribute("symbol-ref")->set(product->getId());
					eq->firstActiveChild("Expression")->setText(init_val);
					vars_with_assignments.insert(s2q(product->getId()));
				}
				else {
					init_val = QString::number(product->getStoichiometry());
				}
				stoch_node->attribute("value")->set(init_val);
				
				term += s2q(product->getId());
			}
			else {
				if (product->isSetStoichiometryMath()) {
					term += "(" + formulaToString( product->getStoichiometryMath()->getMath() ) + ")" ;
	// 				throw SBMLConverterException(SBMLConverterException::SBML_INVALID, "Stochiometry set via MathML is not supported");
				}
				else {
					term += "(" + QString::number(product->getStoichiometry()) + ")" ;
				}
			}
			if (!species[product_name].conversion_factor.isEmpty()) {
				term += " * " + species[product_name].conversion_factor;
			}
			
			term += " * (" + kinetics_formula + ")";
			
			// Skip dynamics of const species 
			if (species[product_name].is_const || species[product_name].is_boundary)
				continue;
			// Skip dynamics of species with direct assignments
			if (vars_with_assignments.contains(product_name))
				continue; 
			
			if (! diffeqn_map.contains(product_name)) {
				nodeController* deq_node = target_system->insertChild("DiffEqn");
				deq_node->attribute("symbol-ref")->set(product_name);
				deq_node->attribute("name")->set("gained from reactions");
				deq_node->attribute("name")->setActive(true);

				diffeqn_map.insert(product_name, {Quantity::Amount, deq_node->firstActiveChild("Expression")->textAttribute()} );
				diffeqn_map[product_name].expression->set(term);
			}
			else {
				diffeqn_map[product_name].expression->append(" + " + term);
			}

		}

	}
	if (have_renamed) {
		conversion_messages.append("Some parameters of the reaction kinetics had to be renamed. Renaming naming scheme is {parameter}_{#reaction}");
	}
}

void SBMLImporter::addSBMLInitialAssignments(Model* sbml_model)
{
	for (uint i=0; i< sbml_model->getNumInitialAssignments(); i++) {
		InitialAssignment* e = sbml_model->getInitialAssignment(i);
		auto childs = target_scope->getChilds();
		bool found_matching =false;
		for (auto child :childs) {
			if (child->getName() == "Constant" || child->getName() == "Variable" || child->getName() == "Property" ) {
				if (child->attribute("symbol") && child->attribute("symbol")->get() == s2q(e->getSymbol()) ) {
					child->attribute("value")->set(formulaToString(e->getMath()));
					found_matching= true;
					break;
				}
			}
		}
	}
}


void SBMLImporter::parseMissingFeatures(Model* sbml_model)
{
	for (uint i=0; i< sbml_model->getNumConstraints(); i++) {
		Constraint* e = sbml_model->getConstraint(i);
		this->conversion_messages.append(
			QString("Dropped unsupported SBML Constraint %1 = %2").arg(
				s2q( (e->isSetId()?e->getId() : e->getName()) ),
				SBML_formulaToString (e->getMath())
			)
		);
	}
}
