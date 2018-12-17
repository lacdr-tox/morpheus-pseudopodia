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
#include "morpheus_model.h"
#include "config.h"
#include <stdexcept>
#include <cstdio>

#include <sbml/SBMLTypes.h>
#include <sbml/common/libsbml-namespace.h>

#ifdef LIBSBML_USE_CPP_NAMESPACE
	using namespace libsbml;
#endif

#define LIBSBML_MIN_VERSION 50000
#define LIBSBML_L3PARSER_VERSION 50900
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

	SBMLConverterException(SBMLConverterException::ExceptionType type, std::string reason = "") : d_type(type), d_reason(reason), std::runtime_error("SBMLConverterException") {};
	~SBMLConverterException() throw() {};
	ExceptionType type() { return d_type; }
	std::string type2name() {
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
	std::string what() {
		return d_reason;
	}
private:
	ExceptionType d_type;
	std::string d_reason;
};


class SBMLImporter: public QDialog {
	Q_OBJECT
public:
	SBMLImporter(QWidget* parent, QSharedPointer< MorphModel > current_model);
	QSharedPointer<MorphModel> getMorpheusModel() { return model;};
// the interface for making this feature puggable
	static const bool supported = true;
 	static QSharedPointer<MorphModel> importSBML();
private:
	QLineEdit* path;
	QComboBox* into_celltype;

	QSharedPointer<MorphModel> model;
	QList<QString> conversion_messages;
	
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
		QString compartment;
		bool is_const;
		bool is_amount;
	};
	
	struct DelayDef {
		QString symbol;
		const ASTNode* formula;
		QString delayed_symbol;
		QString delay;
		bool operator ==(const DelayDef& b) { return symbol == b.symbol && delay == b.delay; }
	};
	
	QMap<QString, SpeciesDesc> species;
	QMap<QString, FunctionDefinition*> functions;
	QSet<QString> constants;
	QSet<QString> variables;
	QSet<QString> vars_with_assignments;
	QList<DelayDef> delays;
	QMap<QString, AbstractAttribute*> diffeqn_map;

	bool readSBML(QString sbml_file, QString target_code);
    void addSBMLFunctions(Model* sbml_model);
	QString formulaToString(const ASTNode* math);
    void addSBMLSpecies(Model* sbml_model);
    void addSBMLParameters(Model* sbml_model);
    void addSBMLRules(Model* sbml_model);
	void addSBMLEvents(Model* sbml_model);
	void addSBMLInitialAssignments(Model* sbml_model);
    void translateSBMLReactions(Model* sbml_model);
    void parseMissingFeatures(Model* sbml_model);
	void replaceDelays(ASTNode* math);

private slots:
	void import();
	void fileDialog();
};

#endif // SBML_CONVERTER_H
