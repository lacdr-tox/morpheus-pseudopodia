//////
//
// This file is part of the modelling and simulation framework 'Morpheus',
// and is made available under the terms of the BSD 3-clause license (see LICENSE
// file that comes with the distribution or https://opensource.org/licenses/BSD-3-Clause).
//
// Authors:  Joern Starruss and Walter de Back
// Copyright 2009-2016, Technische Universität Dresden, Germany
//
//////

#ifndef SBML_CONVERTER_H
#define SBML_CONVERTER_H

#include <QDomDocument>
#include <QMap>
#include <QStringList>
#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QGroupBox>
#include <QLayout>
#include <QStyle>
#include <QDebug>
#include "morpheusML/morpheus_model.h"
#include "config.h"
#include <stdexcept>
#include <cstdio>
#include <memory>

#include <sbml/SBMLTypes.h>
#include <sbml/common/libsbml-namespace.h>

#ifdef LIBSBML_USE_CPP_NAMESPACE
	using namespace libsbml;
#endif

#define LIBSBML_MIN_VERSION 50000
#define LIBSBML_L3PARSER_VERSION 51200
#define LIBSBML_L3V2_VERSION 51500

//LIBSBML_CPP_NAMESPACE

namespace ASTTool {
	void renameSymbol(ASTNode* node, const QString& old_name, const QString& new_name );
	void renameSymbol(ASTNode* node, const string& old_name, const string& new_name );
	void renameTimeSymbol(ASTNode* node, const QString& time_symbol);
	void replaceSymbolByValue(ASTNode* node, const string& name, double value );
	void replaceFunction(ASTNode* node, FunctionDefinition* function);
}

//LIBSBML_CPP_NAMESPACE


class SBMLConverterException : public std::runtime_error {
public:
	enum ExceptionType  {
		FILE_READ_ERROR,
		SBML_LEVEL_GREATER_2,
		SBML_UNKNOWN_COMPARTMENT,
		SBML_DYNAMIC_COMPARTMENT,
		SBML_ALGEBRAIC_RULE,
		SBML_MULTIPLE_DEFINITION,
		SBML_INVALID,
		SBML_INTERNAL_ERROR
	};

	SBMLConverterException(SBMLConverterException::ExceptionType type, string reason = "") : d_type(type), d_reason(reason), std::runtime_error("SBMLConverterException") {};
	~SBMLConverterException() throw() {};
	ExceptionType type() { return d_type; }
	string type2name() {
		switch (d_type) {
			case FILE_READ_ERROR : return "FILE_READ_ERROR";
			case SBML_LEVEL_GREATER_2: return "SBML_LEVEL_GREATER_2";
			case SBML_UNKNOWN_COMPARTMENT: return "SBML_UNKNOWN_COMPARTMENT";
			case SBML_DYNAMIC_COMPARTMENT: return "SBML_DYNAMIC_COMPARTMENT";
			case SBML_ALGEBRAIC_RULE : return "SBML_ALGEBRAIC_RULE";
			case SBML_INVALID : return "SBML_INVALID";
			case SBML_MULTIPLE_DEFINITION : return "SBML_MULTIPLE_DEFINITION";
			case SBML_INTERNAL_ERROR : return "SBML_INTERNAL_ERROR";
			default:
				return "Unknown SBMLConverterException";
		}
	};
	string what() {
		return d_reason;
	}
// 	const char* what() override {
// 		return d_reason.c_str();
// 	}
private:
	ExceptionType d_type;
	string d_reason;
};


class SBMLImporter: public QDialog {
	Q_OBJECT
public:
	SBMLImporter(QWidget* parent, SharedMorphModel current_model);
	SharedMorphModel getMorpheusModel() { return model;};
	bool haveNewModel() { return model_created; };
// the interface for making this feature puggable
	static const bool supported = true;
 	static SharedMorphModel importSBML();
	static SharedMorphModel importSEDMLTest(QString file);
	static SharedMorphModel importSBMLTest(QString file);
private:
	QLineEdit* path;
	QComboBox* into_celltype;
	QLineEdit* tag;

	SharedMorphModel model;
	bool model_created = false;
	QList<QString> conversion_messages;
	
	enum class Quantity { Conc, Amount };
	
	struct CompartmendDesc {
		QString name;
		double init_value;
		QString init_assignment;
		bool dynamic;
	};
	QMap<QString, CompartmendDesc> compartments;
	nodeController* target_system=NULL;
	nodeController* target_scope=NULL;
	
	struct SpeciesDesc {
		QString name;
		QString formula_symbol;
		QString compartment;
		QString conversion_factor;
		bool is_const;
		bool is_boundary;
		Quantity quantity;
		bool uses_as_amount;
		nodeController* node;
		
	};
	
	struct DelayDef {
		QString formula_string;
		const ASTNode* formula;
		QString delayed_symbol;
		QString delay;
		bool operator ==(const DelayDef& b) { return formula_string == b.formula_string && delay == b.delay; }
	};
	
	struct RateDesc {
		Quantity rate_quantity;
		AbstractAttribute* expression;
	};
	
	QString model_conversion_factor;
	bool useL3formulas;
	QMap<QString, SpeciesDesc> species;
	QMap<QString, FunctionDefinition*> functions;
	QSet<QString> constants;
	QSet<QString> variables;
	QSet<QString> vars_with_assignments;
	QList<DelayDef> delays;
	
	QMap<QString, RateDesc> diffeqn_map;
	QMap<QString,QString> concentration_map, amount_map;
	bool have_events = false;

	/** Read an SBML file and convert it using the @target_code
	 *  The @p target_code is a comma separated string: (new|current),(global|celltype){,celltype name}
	 */
	bool readSBML(QString sbml_file, QString target_code);
	/// Read an SBML test model from the suite and use the settings file.
	bool readSBMLTest(QString file);
	/// Read an SBML test from the suite via a SEDML file.
	bool readSEDML(QString file);
	
    void addSBMLFunctions(Model* sbml_model);
	QString formulaToString(const ASTNode* math, bool make_concentration = true);
    void addSBMLSpecies(Model* sbml_model);
    void addSBMLParameters(Model* sbml_model);
    void addSBMLRules(Model* sbml_model);
	void addSBMLEvents(Model* sbml_model);
	void addSBMLInitialAssignments(Model* sbml_model);
    void translateSBMLReactions(Model* sbml_model);
    void parseMissingFeatures(Model* sbml_model);
	void replaceDelays(ASTNode* math);
	void applyTags(nodeController* node);

private slots:
	void import();
	void fileDialog();
};

#endif // SBML_CONVERTER_H
