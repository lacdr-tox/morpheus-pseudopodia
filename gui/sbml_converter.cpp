#include "sbml_converter.h"

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
			cout << "Found function to replace " << node->getName() << endl;
			if (function->getNumArguments() != node->getNumChildren())
				throw SBMLConverterException(SBMLConverterException::SBML_INVALID, std::string("Number function arguments does not match its definition."));
			ASTNode* inline_function = function->getMath()->deepCopy();
			for (uint i=0; i<node->getNumChildren(); i++ ) {
				cout << "Child " << i << " " << function->getArgument(i)->getName() << "->" << node->getChild(i)->getName() << endl;
				ASTTool::renameSymbol(inline_function, std::string(function->getArgument(i)->getName()), std::string(node->getChild(i)->getName()));
			}
			*node = *(inline_function->getChild(inline_function->getNumChildren()-1));
// 			node->swapChildren(inline_function);
			cout << SBML_formulaToString(inline_function);
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


}

QString s2q(const std::string& s) { return QString::fromStdString(s); }


void  SBMLImporter::replaceDelays(ASTNode* math) {
	if (math->getType() == AST_FUNCTION_DELAY) {
		DelayDef d;
		d.formula = math->getChild(0);
		d.symbol = formulaToString(d.formula);
		if (math->getChild(1)->isInteger()) {
			d.delay = QString::number(math->getChild(1)->getInteger());
			d.delayed_symbol = d.symbol + "_" + d.delay;
		}
		else if (math->getChild(1)->isNumber()) {
			d.delay = QString::number(math->getChild(1)->getReal());
			d.delayed_symbol = d.symbol + "_" + d.delay;
		}
		else {
			d.delay = formulaToString( math->getChild(1));
			d.delayed_symbol = d.symbol + "_" + d.symbol.left(4);
		}
		d.delayed_symbol.remove('(');
		d.delayed_symbol.remove(')');
		d.delayed_symbol.replace(',','_');
		d.delayed_symbol.replace('.','_');

		if (!delays.contains(d))
			delays << d;
		
		math->removeChild(1);
		math->removeChild(0);
		math->setType(AST_NAME);
		math->setName(d.delayed_symbol.toLatin1());
	}
	
	for (uint i=0; i<math->getNumChildren(); i++) {
		replaceDelays(math->getChild(i));
	}
}

QSharedPointer<MorphModel> SBMLImporter::importSBML() {
	SBMLImporter* importer = new SBMLImporter(nullptr, config::getModel());
	
	if (QDialog::Accepted == importer->exec()) {
		return importer->getMorpheusModel();
	}
	else {
		return QSharedPointer<MorphModel>();
	}
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
	
	if ( ! QFileInfo(sbml_file).exists() ) {
		throw SBMLConverterException(SBMLConverterException::FILE_READ_ERROR, sbml_file.toStdString());
	}
	SBMLDocument* sbml_doc =0;
	sbml_doc = readSBMLFromFile(sbml_file.toStdString().c_str());
	if (! sbml_doc)
		throw SBMLConverterException(SBMLConverterException::FILE_READ_ERROR, "Cannot read file.");

	Model* sbml_model = sbml_doc->getModel();
	if (! sbml_model)
		throw SBMLConverterException(SBMLConverterException::FILE_READ_ERROR, "File does not contain an SBML model.");
	// Add annotation for new models
	
	// Load model, test compatibility
	
	//if ( sbml_doc->checkL2v4Compatibility() != 0 && sbml_doc->checkL2v1Compatibility() != 0) {
	//	sbml_doc->getError(0)->print(cout);
	//	throw SBMLConverterException(SBMLConverterException::SBML_LEVEL_GREATER_2, sbml_doc->getError(0)->getMessage());
	//}

	// Setup target model
	
	QSharedPointer<MorphModel> morph_model;
	auto target = target_code.split(",");
	
	if (target[0] == "new") {
		QString plain_morpheus_model(
"<?xml version='1.0' encoding='UTF-8'?> \
<MorpheusModel version=\"3\"> \
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
		
	target_system->attribute("time-step")->set(stop_symbol);
	
	if (target[0] == "new") {
		nodeController* description = morph_model->rootNodeContr->firstActiveChild("Description");
		if (!description) throw SBMLConverterException(SBMLConverterException::SBML_INTERNAL_ERROR, "Description node not found." );

		string title;
		if (sbml_doc->isSetName())
			title = sbml_model->getName();
		else
			title = sbml_model->getId();
		description->firstActiveChild("Title")->setText(s2q(title));

		std::string details = sbml_doc->getNotesString();
		description->firstActiveChild("Details")->setText(s2q(details));
	}
	
	// Convert to target scope

	this->addSBMLFunctions(sbml_model);
	
	this->addSBMLParameters(sbml_model);
	
	if (sbml_model->getNumCompartments()) {
		for (uint i=0; i<sbml_model->getNumCompartments(); i++ ) {
			CompartmendDesc comp;
			comp.name = s2q(sbml_model->getCompartment(i)->getId());
			comp.init_value = sbml_model->getCompartment(i)->getSize();
			auto init = sbml_model->getInitialAssignment(comp.name.toStdString());
			if (init) {
				comp.init_assignment = formulaToString(init->getMath());
			}
			comp.dynamic = (sbml_model->getCompartment(i)->isSetConstant() && ! sbml_model->getCompartment(i)->getConstant());
			nodeController* compartment_node = target_scope->insertChild((comp.dynamic ? "Variable" : "Constant"));
			compartment_node -> attribute("symbol") -> set(comp.name);
			if (!comp.init_assignment.isEmpty())
				compartment_node -> attribute("value") -> set(comp.init_assignment);
			else
				compartment_node -> attribute("value") -> set(comp.init_value);
			compartments[comp.name] = comp;
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

	this->addSBMLRules(sbml_model);

	this->translateSBMLReactions(sbml_model);

	this->addSBMLEvents(sbml_model);

	// All rate equations are done, now compensate for compartments with changing vbolumes
	for (const auto& comp : compartments) {
		if (comp.dynamic && diffeqn_map.contains(comp.name)) {
			// Expose dv/dt
			auto dv = target_system->insertChild("Intermediate");
			QString dv_name = comp.name + ".rate";
			dv->attribute("symbol")->set(dv_name);
			dv->attribute("value")->set(diffeqn_map[comp.name]->get());
			
			//Manipulate all specie's odes rules in the compartment
			
			for (auto species_ode = diffeqn_map.begin(); species_ode !=diffeqn_map.end(); species_ode++) {
				if  (species[species_ode.key()].compartment == comp.name && (!species[species_ode.key()].is_amount) ) {
					species_ode.value()->append( QString(" - %1*%2/%3").arg(species_ode.key()).arg(dv_name).arg(comp.name)  );
				}
			}
		}
	}

	// Add all delays collected while parsing
	for (const auto& delay : delays) {
// 		bool does_not_exist=delays.contains(delay);
// 		if (does_not_exist) {
			
			bool is_celltype = target[1]!="global";
			auto delay_property = target_scope->insertChild(is_celltype ? "DelayProperty" : "DelayVariable");
			delay_property->attribute("symbol")->set(delay.delayed_symbol);
			delay_property->attribute("delay")->set(delay.delay);
			
			// TODO This does only work for symbol delays, not with delayed formulas.
			// We must replace *all* symbols in a delayed formula with the respective initAssignments. 
			auto init = sbml_model->getInitialAssignment(delay.symbol.toStdString().c_str());
			if (init) {
				delay_property->attribute("value")->set(formulaToString(init->getMath()));
			}
			
			auto delay_rule = target_scope->insertChild("Equation");
			delay_rule->attribute("symbol-ref")->set(delay.delayed_symbol);
			delay_rule->firstActiveChild("Expression")->setText(delay.symbol);
			
// 			delays.append(delay);
// 		}
	}

	this->parseMissingFeatures(sbml_model);
	
	
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

		bool first=true;
		foreach (const QString& var, variables){
			if( first ){
				logger_log->firstActiveChild("Symbol")->attribute("symbol-ref")->set(var);
				logger_plot->firstActiveChild("Symbol")->attribute("symbol-ref")->set(var);
				first = false;
			}
			else{
				nodeController* symbol_log = logger_log->insertChild("Symbol");			
				symbol_log->attribute("symbol-ref")->set(var);
				nodeController* symbol_plot = logger_plot->insertChild("Symbol");
				symbol_plot->attribute("symbol-ref")->set(var);
			}
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
	
	if (target[0]=="new") {
		model = morph_model;
	}
	else {
		model.clear();
	}
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
		desc.name = s2q(species->getId());
		desc.is_amount = false;
		if (species->isSetSubstanceUnits()) {
			if (species->getSubstanceUnits() == string("substance") ) desc.is_amount = true;
			if (species->getSubstanceUnits() == string("mole") ) desc.is_amount = true;
		}
		

		nodeController* property_node = target_scope->insertChild((desc.is_const ? "Constant" : (is_celltype ? "Property" : "Variable")));
		this->variables.insert(desc.name);
		this->species[desc.name] = desc;

		property_node->attribute("symbol")->set(species->getId());
		if (species->isSetName()){
			property_node->attribute("name")->setActive(true);
			property_node->attribute("name")->set(species->getName());
		}
		
		QString init_value;
		
		auto init=sbml_model->getInitialAssignment(species->getId());
		if (init) {
			init_value = formulaToString(init->getMath());
		}
		else if (species->isSetInitialConcentration()) {
			init_value = QString::number(species->getInitialConcentration());
			if (desc.is_amount)
				init_value = init_value + "*" + desc.compartment;
		}
		else if (species->isSetInitialAmount() ){
			init_value = QString::number(species->getInitialAmount());
			if ( ! desc.is_amount)
				init_value = init_value + "/" + desc.compartment;
		}
		else {
			init_value = "0";
		}
		
		property_node->attribute("value")->set(init_value);
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

		auto init=sbml_model->getInitialAssignment(parameter->getId());
		if (init) {
			param_node->attribute("value")->set( formulaToString(init->getMath()) );
		}
		else if (parameter->isSetValue())
			param_node->attribute("value")->set( parameter->getValue() );
		else
			param_node->attribute("value")->set( "0.0" );
	}
}

void SBMLImporter::addSBMLFunctions(Model* sbml_model)
{
	for (uint fun=0; fun<sbml_model->getNumFunctionDefinitions();fun++) {
		auto mo_function = target_scope->insertChild("Function");
		FunctionDefinition* function = sbml_model->getFunctionDefinition(fun);
		mo_function->attribute("symbol")->set(function->getId());
		mo_function->attribute("name")->set(function->getName());
		mo_function->attribute("name")->setActive(true);
		for (uint i=0; i<function->getNumArguments(); i++ ) {
			auto param = mo_function->insertChild("Parameter",i);
			param->attribute("symbol")->set(function->getArgument(i)->getName());
		}
		mo_function->firstActiveChild("Expression")->setText(formulaToString(function->getMath()));
	}
};

QString SBMLImporter::formulaToString(const ASTNode* math)
{
	auto m = std::unique_ptr<ASTNode>(math->deepCopy());
	ASTTool::renameTimeSymbol(m.get(), target_scope->getModelDescr().time_symbol->get());
	
	replaceDelays(m.get());
	
	ASTTool::replaceRateOf(m.get());
#if LIBSBML_VERSION >= LIBSBML_L3PARSER_VERSION
	auto c = std::unique_ptr<char[]>(SBML_formulaToL3String(m.get()));
#else
	auto c = std::unique_ptr<char[]>(SBML_formulaToString(m.get()));
#endif
	return QString(c.get());
}


void SBMLImporter::addSBMLRules(Model* sbml_model)
{
	for (uint rul=0; rul<sbml_model->getNumRules();rul++) {
		const Rule* rule = sbml_model->getRule(rul);
		
		if (species.count(s2q(rule->getVariable()))) {
			if (species[s2q(rule->getVariable())].is_const) {
				continue;
			}
		}
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
				rule_node->attribute("symbol-ref")->set(rule->getVariable());
				rule_node->firstActiveChild("Expression")->setText(expression);
				if (rule->isSetName()) {
					rule_node->attribute("name")->set(rule->getName());
					rule_node->attribute("name")->setActive(true);
				}
				break;
			}
			case SBML_RATE_RULE :
				rule_node = target_system->insertChild("DiffEqn");
				rule_node->attribute("symbol-ref")->set(rule->getVariable());
				rule_node->firstActiveChild("Expression")->setText(expression);
				if (rule->isSetName()) {
					rule_node->attribute("name")->set(rule->getName());
					rule_node->attribute("name")->setActive(true);
				}
				
				diffeqn_map.insert(s2q(rule->getVariable()),rule_node->firstActiveChild("Expression")->textAttribute());
				break;
			default :
				throw SBMLConverterException(SBMLConverterException::SBML_INVALID);
				break;
		}

	}
};

void SBMLImporter::addSBMLEvents(Model* sbml_model)
{
	for (uint i=0; i< sbml_model->getNumEvents(); i++) {
		Event* e = sbml_model->getEvent(i);
		nodeController* event = target_scope->insertChild("Event");
		if (e->isSetName()) {
			event->attribute("name")->setActive(true);
			event->attribute("name")->set(s2q(e->getName()));
		}
		if (e->isSetTrigger()) {
			nodeController* condition = event->firstActiveChild("Condition");
			if (!condition)
				condition = event->insertChild("Condition");
			condition->setText(formulaToString(e->getTrigger()->getMath()));
		}
		if (e->isSetDelay()) {
			event->attribute("delay")->setActive(true);
			event->attribute("delay")->set(formulaToString(e->getDelay()->getMath()));
			if ( e->isSetUseValuesFromTriggerTime() && e->getUseValuesFromTriggerTime() ) {
				event->attribute("compute-time")->setActive(true);
				event->attribute("compute-time")->set("on-trigger");
			}
			else {
				event->attribute("compute-time")->setActive(true);
				event->attribute("compute-time")->set("on-execution");
			}
		}
		if (e->isSetPriority()) {
			this->conversion_messages.append(
				QString("Dropped unsupported priority element on SBML Event \"%1\"").arg( s2q( (e->isSetId()?e->getId() : e->getName()) ) )
			);
		}
		for (uint j=0; j<e->getNumEventAssignments(); j++) {
			const EventAssignment* ass = e->getEventAssignment(j);
			nodeController* equation =  event->insertChild("Rule");
			equation->attribute("symbol-ref")->set(ass->getVariable());
			equation->firstActiveChild("Expression")->setText(formulaToString(ass->getMath()));
		}
	}
}


void SBMLImporter::translateSBMLReactions(Model* sbml_model)
{
	// Here we have to take care, that all symbols are meant to be reaction local,
	// i.e. only valid in the scope the current reaction.
	// we thus rename symbols that are already defined, such that they all can be valid on global scope

	QSet<QString> all_symbols_defined = variables + constants + functions.keys().toSet();
	bool have_renamed = false;

	// this was inspired by the soslib odeConstruct.c:Species_odeFromReactions();
	for ( uint r=0; r<sbml_model->getNumReactions(); r++ ) {
		Reaction* reaction = sbml_model->getReaction(r);
		KineticLaw* kinetics = reaction->getKineticLaw();
		QMap<QString,QString> renamed_symbols;
		if (!kinetics)
			throw SBMLConverterException(SBMLConverterException::SBML_INVALID, "Missing reaction kinetics");

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

			nodeController* param_node = target_system->insertChild("Constant");
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
		
		// Solve the left hand side -- reactants of the equation
		string reactants_compartment;
		for (uint j=0; j<reaction->getNumReactants(); j++) {
			SpeciesReference* reactant = reaction->getReactant(j);
			QString reactant_name = s2q(reactant->getSpecies());
			if (!species.count(reactant_name))
				throw SBMLConverterException(SBMLConverterException::SBML_INVALID,string("Unknown Species ")+ reactant_name.toStdString());
				
			// Skip dynamics of const species 
			if (species[reactant_name].is_const) {
				continue;
			}
			// Skip dynamics of species with direct assignments
			if (vars_with_assignments.contains(reactant_name))
				continue;

			QString term;

			if (reactant->isSetStoichiometryMath()) {
				throw SBMLConverterException(SBMLConverterException::SBML_INVALID, "Stochiometry set via MathML is not supported");
			}
			if (reactant->getStoichiometry() != 1.0) {
				term += QString::number(reactant->getStoichiometry()) + "*";
			}
			
			term += "(" + kinetics_formula + ")";
			if (!species[reactant_name].is_amount) {
				// divide by compartment size, since kinetics describe the amount flux, not concentration flux
				term += "/" + species[reactant_name].compartment;
			}
			
			if (! diffeqn_map.contains(reactant_name)) {
				nodeController* deq_node = target_system->insertChild("DiffEqn");
				deq_node->attribute("symbol-ref")->set(reactant_name);
				deq_node->attribute("name")->set("gained from reactions");
				deq_node->attribute("name")->setActive(true);

				diffeqn_map.insert(reactant_name,deq_node->firstActiveChild("Expression")->textAttribute());
				diffeqn_map[reactant_name]->set(QString("- ") + term);
			}
			else {
				diffeqn_map[reactant_name]->append(" - " + term);
			}

		}

		// Solve the right hand side -- products of the reaction
		for (uint j=0; j<reaction->getNumProducts(); j++) {
			SpeciesReference* product = reaction->getProduct(j);
			QString product_name = s2q(product->getSpecies());
			if (!species.count(product_name))
				throw SBMLConverterException(SBMLConverterException::SBML_INVALID,string("Unknown Species ")+ product_name.toStdString());
			
			// Skip dynamics of const species 
			if (species[product_name].is_const)
				continue;
			// Skip dynamics of species with direct assignments
			if (vars_with_assignments.contains(product_name))
				continue; 

			QString term;
			if (product->isSetStoichiometryMath()) {
				throw SBMLConverterException(SBMLConverterException::SBML_INVALID, "Stochiometry set via MathML is not supported");
			}
			if (product->getStoichiometry() != 1.0) {
				term += QString::number(product->getStoichiometry()) + "*";
			}
			term += "(" + kinetics_formula + ")";
			if (!species[product_name].is_amount) {
				// divide by compartment size, since kinetics describe the amount flux, not concentration flux
				term += "/" + species[product_name].compartment;
			}
			
			if (! diffeqn_map.contains(product_name)) {
				nodeController* deq_node = target_system->insertChild("DiffEqn");
				deq_node->attribute("symbol-ref")->set(product_name);
				deq_node->attribute("name")->set("gained from reactions");
				deq_node->attribute("name")->setActive(true);

				diffeqn_map.insert(product_name,deq_node->firstActiveChild("Expression")->textAttribute());
				diffeqn_map[product_name]->set(term);
			}
			else {
				diffeqn_map[product_name]->append(" + " + term);
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
