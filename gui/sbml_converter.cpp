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


}

QString s2q(const std::string& s) { return QString::fromStdString(s); }


QSharedPointer<MorphModel> SBMLImporter::importSBML() {
	SBMLImporter* importer = new SBMLImporter();
	if (QDialog::Accepted == importer->exec()) {
		return importer->getMorpheusModel();
	}
	else {
		return QSharedPointer<MorphModel>();
	}
}

SBMLImporter::SBMLImporter(QWidget* parent) : QDialog(parent)
{
	this->setMaximumWidth(500);
	this->setMinimumHeight(250);
	QVBoxLayout* layout = new QVBoxLayout(this);


// 	QLabel* header = new QLabel(this);
// 	header->setText("SBML Import");
// 	header->setAlignment(Qt::AlignHCenter);
	setWindowTitle("SBML Import");

// 	layout->addWidget(header);
// 	layout->addSpacing(20);
	layout->addStretch(1);
	QGroupBox* frame = new QGroupBox("", this);

	frame->setLayout(new QHBoxLayout(this));
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

	QHBoxLayout* path_layout = new QHBoxLayout(this);
	QLabel* path_label = new QLabel("SBML File ",this);
	path_layout->addWidget(path_label);

	path = new QLineEdit(this);
	path_label->setBuddy(path);
	path_layout->addWidget(path);

	QPushButton * file_dlg = new QPushButton(this);
	file_dlg->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
	connect(file_dlg,SIGNAL(clicked(bool)),this,SLOT(fileDialog()));
	path_layout->addWidget(file_dlg);
	layout->addLayout(path_layout);

	layout->addStretch(3);

	QHBoxLayout *bottom = new QHBoxLayout(this);

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
		readSBML(path->text());
	}
	catch (SBMLConverterException e) {
		qDebug() << "Unable to import SBML due to " <<  s2q(e.type2name())<< " "<< s2q(e.what());
		QMessageBox::critical(this,"SBML Import Error", QString("Unable to import %1 due to the following error:\n%2 - %3").arg(path->text(),s2q(e.type2name()), s2q(e.what())), QMessageBox::Ok);
	}
	catch (QString s) {
		qDebug() << "Unable to import SBML due to " << s;
		QMessageBox::critical (this,"SBML Import Error", QString("Unable to import %1 due to the following error:\nSBML_INTERNAL_ERROR - %2").arg(path->text(),s),QMessageBox::Ok);
	}
	if (model)
		accept();
}



void SBMLImporter::readSBML(QString sbml_file)
{
	if ( ! QFileInfo(sbml_file).exists() ) {
		throw SBMLConverterException(SBMLConverterException::FILE_READ_ERROR, sbml_file.toStdString());
	}
	SBMLDocument* sbml_doc =0;
	sbml_doc = readSBMLFromFile(sbml_file.toStdString().c_str());
	if (! sbml_doc)
		throw SBMLConverterException(SBMLConverterException::FILE_READ_ERROR);

	//if ( sbml_doc->checkL2v4Compatibility() != 0 && sbml_doc->checkL2v1Compatibility() != 0) {
	//	sbml_doc->getError(0)->print(cout);
	//	throw SBMLConverterException(SBMLConverterException::SBML_LEVEL_GREATER_2, sbml_doc->getError(0)->getMessage());
	//}


	Model* sbml_model = sbml_doc->getModel();

	if (sbml_model->getNumCompartments() != 1 ) {
		std::string message = "Defined compartments: ";
		message.append(sbml_model->getCompartment(0)->getId());
		for (uint i=1; i<sbml_model->getNumCompartments();i++) {
			message.append(", ").append(sbml_model->getCompartment(i)->getId());
		}
		throw SBMLConverterException(SBMLConverterException::SBML_MULTI_COMPARTMENT, message );
	}

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
        <StopTime value=\"1\"/> \
        <TimeSymbol symbol=\"time\"/> \
    </Time> \
    <Global> \
    </Global> \
    <CellTypes> \
        <CellType class=\"biological\" name=\"sbml_ct\"> \
            <System solver=\"runge-kutta\" time-step=\"0.01\"/> \
        </CellType> \
    </CellTypes> \
    <CellPopulations> \
        <Population size=\"1\" type=\"sbml_ct\"/> \
    </CellPopulations> \
    <Analysis> \
        <Logger time-step=\"0.01\"> \
            <Input> \
				<Symbol symbol-ref=\"cell.id\"/> \
            </Input> \
            <Output> \
                <TextOutput/> \
            </Output> \
            <Plots> \
                <Plot time-step=\"-1\"> \
                    <Style style=\"lines\"/> \
                    <Terminal terminal=\"png\"/> \
                    <X-axis> \
                        <Symbol symbol-ref=\"time\"/> \
                    </X-axis> \
                    <Y-axis> \
						<Symbol symbol-ref=\"cell.id\"/> \
					</Y-axis> \
                </Plot> \
            </Plots>  \
        </Logger> \
    </Analysis> \
</MorpheusModel>\"");

	QDomDocument morph_doc;
	morph_doc.setContent(plain_morpheus_model);
	QSharedPointer<MorphModel> morph_model(new MorphModel(morph_doc));

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

	compartment_symbol = s2q(sbml_model->getCompartment(0)->getId());
	compartment_size   = sbml_model->getCompartment(0)->getSize();

	readSBMLFunctions(sbml_model);

	nodeController* celltype = morph_model->rootNodeContr->firstActiveChild("CellTypes")->firstActiveChild("CellType");
	if (!celltype) throw SBMLConverterException(SBMLConverterException::SBML_INTERNAL_ERROR, "Celltype node not found.");

	this->addSBMLParameters(celltype,sbml_model);

	this->addSBMLSpecies(celltype,sbml_model);

	this->addSBMLRules(celltype,sbml_model);

	this->translateSBMLReactions(celltype,sbml_model);

	this->addSBMLEvents(celltype,sbml_model);

	nodeController* cellpopulation = morph_model->rootNodeContr->firstActiveChild("CellPopulations")->firstActiveChild("Population");
	this->addSBMLInitialAssignments(cellpopulation, sbml_model);

	this->parseMissingFeatures(sbml_model);

	nodeController* logger = morph_model->rootNodeContr->firstActiveChild("Analysis")->firstActiveChild("Logger");
	if (!logger) throw SBMLConverterException(SBMLConverterException::SBML_INTERNAL_ERROR, "Logger node not found.");

	// add symbols to GLobal and Logger (both in Logger/Input and in Logger/Plots/Plot/Y-axis)
	nodeController* global      = morph_model->rootNodeContr->firstActiveChild("Global");
	if (!global) throw SBMLConverterException(SBMLConverterException::SBML_INTERNAL_ERROR, "Global node not found.");
	nodeController* logger_log  = logger->firstActiveChild("Input");
	nodeController* logger_plot = logger->firstActiveChild("Plots")->firstActiveChild("Plot")->firstActiveChild("Y-axis");

	bool first=true;
	foreach (const QString& var, variables){
		cout << "Variable : " << var.toStdString() << endl; 
		nodeController* global_const = global->insertChild("Constant");
		global_const->attribute("symbol")->set(var);
		global_const->attribute("value")->set("0.0");
		
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
	cout << flush;
	

/*	logger->firstChild("Format")->attribute("string")->set(QStringList(QList<QString>::fromSet(variables)).join(" "));
	QStringList columns;
	int col=3;
	foreach (const QString& var,variables) {
		columns.append(QString::number(col));
		col++;
	}
	logger->firstChild("Plot")->firstChild("Y-axis")->attribute("columns")->set(columns.join(" "));
*/

	SBMLDocument_free(sbml_doc);

	morph_model->rootNodeContr->clearTrackedChanges();

	morph_model->rootNodeContr->trackNextChange();
	morph_model->rootNodeContr->firstActiveChild("Time")->firstActiveChild("StopTime")->attribute("value")->set("1.0");

	morph_model->rootNodeContr->trackNextChange();
	celltype->firstActiveChild("System")->attribute("time-step")->set("0.01");

	for (int i=0; i<conversion_messages.size(); i++) {
		morph_model->rootNodeContr->trackInformation(conversion_messages[i]);
	}
	model = morph_model;
	
};

void SBMLImporter::addSBMLSpecies(nodeController* celltype, Model* sbml_model)
{
	double compartment_size = sbml_model->getCompartment(0)->getSize();
	QStringList all_species;
	for (uint spec=0; spec<sbml_model->getNumSpecies(); spec++) {
		Species* species = sbml_model->getSpecies(spec);
		bool is_const = (species->isSetConstant() && species->getConstant());

		nodeController* property_node = celltype->insertChild((is_const ? "Constant" : "Property"));

		this->variables.insert(s2q(species->getId()));

		property_node->attribute("symbol")->set(species->getId());
		if (species->isSetName()){
			property_node->attribute("name")->setActive(true);
			property_node->attribute("name")->set(species->getName());
		}

		double init_value;
		if (species->isSetInitialConcentration()) {
			init_value = species->getInitialConcentration();
		}
		else {
			init_value = species->getInitialAmount();
			if (compartment_size !=0) init_value /= compartment_size;
		}
		property_node->attribute("value")->set(init_value);
	}
};

void SBMLImporter::addSBMLParameters(nodeController* celltype, Model* sbml_model)
{
	// Adding the compartment size as a constant -- some sane models use that as value
// 	nodeController* compartment_size_constant = celltype->insertChild("Constant");
// 	compartment_size_constant->attribute("symbol")-> set(sbml_model->getCompartment(0)->getId());
// 	compartment_size_constant->attribute("name")  -> set("compartment size");
// 	compartment_size_constant->attribute("name")  -> setActive(true);
// 	compartment_size_constant->attribute("value") -> set(sbml_model->getCompartment(0)->getSize());

	for (uint param=0; param<sbml_model->getNumParameters(); param++){
		Parameter* parameter = sbml_model->getParameter(param);

		if (parameter->isSetConstant() &&  ! parameter->getConstant()) { // this is a variable parameter
			nodeController* property_node = celltype->insertChild("Property");

			property_node->attribute("symbol")->set(parameter->getId());
			variables.insert(s2q(parameter->getId()));

			if (parameter->isSetName()) {
				property_node->attribute("name")->set(parameter->getName());
				property_node->attribute("name")->setActive(true);
			}

			double init_value;
			if (parameter->isSetValue())
				init_value = parameter->getValue();
			else
				init_value = 0.0;
			property_node->attribute("value")->set(init_value);
		}
		else {
			nodeController* const_node = celltype->insertChild("Constant");
			const_node->attribute("symbol")->set(parameter->getId());
			constants.insert(s2q(parameter->getId()));

			if (parameter->isSetName()) {
				const_node->attribute("name")->set(parameter->getName());
				const_node->attribute("name")->setActive(true);
			}
			const_node->attribute("value")->set(parameter->getValue());

		}
	}
}

void SBMLImporter::readSBMLFunctions(Model* sbml_model)
{
	for (uint fun=0; fun<sbml_model->getNumFunctionDefinitions();fun++) {
		FunctionDefinition* function = sbml_model->getFunctionDefinition(fun);
		functions.insert(s2q(function->getId()),function);
	}
};

void SBMLImporter::sanitizeAST(ASTNode* math)
{
	ASTTool::replaceSymbolByValue(math, compartment_symbol.toStdString(), compartment_size);

	QMap<QString, FunctionDefinition* >::const_iterator it;
	for (it=functions.begin() ; it!=functions.end(); it++) {
		ASTTool::replaceFunction(math,it.value());
	}
}


void SBMLImporter::addSBMLRules(nodeController* celltype, Model* sbml_model)
{
	nodeController*  system = celltype->firstActiveChild("System");
	for (uint rul=0; rul<sbml_model->getNumRules();rul++) {
		const Rule* rule = sbml_model->getRule(rul);
		nodeController* rule_node;
		switch (rule->getTypeCode()) {
			case SBML_ALGEBRAIC_RULE :
				throw SBMLConverterException(SBMLConverterException::SBML_ALGEBRAIC_RULE);
				break;
			case SBML_ASSIGNMENT_RULE :
				rule_node = celltype->insertChild("Equation");
				break;
			case SBML_RATE_RULE :
				rule_node = system->insertChild("DiffEqn");
				break;
			default :
				throw SBMLConverterException(SBMLConverterException::SBML_INVALID);
				break;
		}
		rule_node->attribute("symbol-ref")->set(rule->getVariable());
		if (rule->isSetName()) {
			rule_node->attribute("name")->set(rule->getName());
			rule_node->attribute("name")->setActive(true);
		}

		ASTNode* math = rule->getMath()->deepCopy();
		sanitizeAST(math);
		rule_node->firstActiveChild("Expression")->setText(SBML_formulaToString(math));
		delete math;

		if (rule->getTypeCode() == SBML_ASSIGNMENT_RULE) {
			vars_with_assignments.insert(s2q(rule->getVariable()));
		}
		else if (rule->getTypeCode() == SBML_RATE_RULE) {
			diffeqn_map.insert(s2q(rule->getVariable()),rule_node->firstActiveChild("Expression")->textAttribute());
		}
	}
};

void SBMLImporter::addSBMLEvents(nodeController* celltype, Model* sbml_model)
{
	for (uint i=0; i< sbml_model->getNumEvents(); i++) {
		Event* e = sbml_model->getEvent(i);
		if (e->isSetDelay()) {
			this->conversion_messages.append(
				QString("Dropped unsupported SBML Event \"%1\" assigning %2 = %3 due to unsupported Delay").arg(
					s2q( (e->isSetId()?e->getId() : e->getName()) ),
					s2q( e->getEventAssignment(0)->getVariable()),
					SBML_formulaToString (e->getEventAssignment(0)->getMath())
				)
			);
			continue;
		}
		nodeController* event = celltype->insertChild("Event");
		if (e->isSetName()) {
			event->attribute("name")->setActive(true);
			event->attribute("name")->set(s2q(e->getName()));
		}
		if (e->isSetTrigger()) {
			ASTNode* math = e->getTrigger()->getMath()->deepCopy();
			sanitizeAST(math);
			nodeController* condition = event->firstActiveChild("Condition");
			if (!condition)
				condition = event->insertChild("Condition");
			condition->setText(SBML_formulaToString(math));
			delete math;
		}
		for (uint j=0; j<e->getNumEventAssignments(); j++) {
			const EventAssignment* ass = e->getEventAssignment(j);
			nodeController* equation =  event->insertChild("Rule");
			equation->attribute("symbol-ref")->set(ass->getVariable());

			ASTNode* math = ass->getMath()->deepCopy();
			sanitizeAST(math);
			equation->firstActiveChild("Expression")->setText(SBML_formulaToString(math));
			delete math;
		}
	}
}


void SBMLImporter::translateSBMLReactions(nodeController* celltype, Model* sbml_model)
{
	// Here we have to take care, that all symbols are meant to be reaction local,
	// i.e. only valid in the scope the current reaction.
	// we thus rename symbols that are already defined, such that they all can be valid on global scope

	QSet<QString> all_symbols_defined = variables + constants + functions.keys().toSet();
	bool have_renamed = false;

	nodeController* system = celltype->firstActiveChild("System");
	// this was inspired by the soslib odeConstruct.c:Species_odeFromReactions();
	for ( uint r=0; r<sbml_model->getNumReactions(); r++ ) {
		Reaction* reaction = sbml_model->getReaction(r);
		KineticLaw* kinetics = reaction->getKineticLaw();
		QMap<QString,QString> renamed_symbols;
		if (!kinetics)
			throw SBMLConverterException(SBMLConverterException::SBML_INVALID);

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

			nodeController* param_node = system->insertChild("Constant");
			param_node->attribute("symbol")->set(param_symbol);
			if (param->isSetName()) {
				param_node->attribute("name")->set(param->getName());
				param_node->attribute("name")->setActive(true);
			}
			param_node->attribute("value")->set(param->getValue());
		}
		
		// After parsing the params, we know which symbols to rename in the kinetics
		QMap<QString,QString>::iterator rename;
		ASTNode* kinetics_math = kinetics->getMath()->deepCopy();
		for (rename = renamed_symbols.begin(); rename!=renamed_symbols.end(); rename++) {
			ASTTool::renameSymbol(kinetics_math, rename.key(), rename.value());
		}
		sanitizeAST(kinetics_math);

		// Solve the left hand side -- reactants of the equation
		for (uint j=0; j<reaction->getNumReactants(); j++) {
			SpeciesReference* reactant = reaction->getReactant(j);
			QString reactant_name = s2q(reactant->getSpecies());
			if (vars_with_assignments.contains(reactant_name))
				continue;

			if (! diffeqn_map.contains(reactant_name)) {
				nodeController* deq_node = system->insertChild("DiffEqn");
				deq_node->attribute("symbol-ref")->set(reactant_name);
				deq_node->attribute("name")->set("gained from reactions");
				deq_node->attribute("name")->setActive(true);

				diffeqn_map.insert(reactant_name,deq_node->firstActiveChild("Expression")->textAttribute());
			}

			QString term;

			if (reactant->isSetStoichiometryMath()) {
				throw SBMLConverterException(SBMLConverterException::SBML_INVALID);
			}
			if (reactant->getStoichiometry() != 1.0) {
				term += QString::number(reactant->getStoichiometry()) + "*";
			}

			term += QString(SBML_formulaToString(kinetics_math));

			diffeqn_map[reactant_name]->set(diffeqn_map[reactant_name]->get() + " - " + term);
		}

		// Solve the right hand side -- products of the reaction
		for (uint j=0; j<reaction->getNumProducts(); j++) {
			SpeciesReference* product = reaction->getProduct(j);
			QString product_name = s2q(product->getSpecies());
			if (vars_with_assignments.contains(product_name))
				continue;

			if (! diffeqn_map.contains(product_name)) {
				nodeController* deq_node = system->insertChild("DiffEqn");
				deq_node->attribute("symbol-ref")->set(product_name);
				deq_node->attribute("name")->set("gained from reactions");
				deq_node->attribute("name")->setActive(true);

				diffeqn_map.insert(product_name,deq_node->firstActiveChild("Expression")->textAttribute());
			}

			QString term;
			if (product->isSetStoichiometryMath()) {
				throw SBMLConverterException(SBMLConverterException::SBML_INVALID);
			}
			if (product->getStoichiometry() != 1.0) {
				term += QString::number(product->getStoichiometry()) + "*";
			}

			term += QString(SBML_formulaToString(kinetics_math));
			QString expr = diffeqn_map[product_name]->get();

			diffeqn_map[product_name]->set(expr + (expr.isEmpty() ? " " : " + ") + term);
		}
		delete kinetics_math;
		// TODO Divides by the size of the compartment.
		// Here soslib divides by the size of the compartment.
		// Can we safely discard this due to the fact that odes are rate equations based on concentrations?
	}
	if (have_renamed) {
		conversion_messages.append("Some parameters of the reaction kinetics had to be renamed. Renaming naming scheme is {parameter}_{#reaction}");
	}
}

void SBMLImporter::addSBMLInitialAssignments(nodeController* cellPopulation, Model* sbml_model)
{
	for (uint i=0; i< sbml_model->getNumInitialAssignments(); i++) {
		InitialAssignment* e = sbml_model->getInitialAssignment(i);

		if ( !variables.contains(s2q(e->getSymbol())) ) {
			this->conversion_messages.append(
				QString("Dropped unsupported SBML InitialAssignment to constant \"%1\" assigning %2 = %3").arg(
					s2q( (e->isSetId()?e->getId() : e->getName()) ),
					s2q( e->getSymbol()),
					SBML_formulaToString (e->getMath())
				)
			);
			continue;
		}
		nodeController* init_property = cellPopulation->insertChild("InitProperty");
		init_property->attribute("symbol-ref")->set(e->getSymbol());
		ASTNode* math = e->getMath()->deepCopy();
		sanitizeAST(math);
		init_property->firstActiveChild("Expression")->setText(SBML_formulaToString (math));
		delete math;
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
