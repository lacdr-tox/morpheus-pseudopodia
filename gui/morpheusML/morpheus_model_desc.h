#ifndef MORPHEUS_MODEL_DESC_H
#define MORPHEUS_MODEL_DESC_H

#include <exception>
#include <QDomNode>
#include "xsd.h"

class AbstractAttribute;

// template <class T>
// bool operator <( const QSharedPointer<T> a, const QSharedPointer<T> b) {
//     return a.data() < b.data();
// }

typedef QMap< AbstractAttribute*, AbstractAttribute* > AttributeMap;

struct MorphModelEdit {
	enum ModelEditType {AttribAdd, AttribRemove, AttribRename, AttribChange,  NodeAdd, NodeRemove, NodeRename, NodeMove, TextChange};
	QString info;
	QString name, value;
	ModelEditType edit_type;
	QDomNode xml_parent;
};

struct ModelDescriptor {
	AttributeMap symbolNames;
	XSD xsd;
	bool track_next_change;
	bool stealth;
	QList<MorphModelEdit> auto_fixes;
	QList<MorphModelEdit> edit_operations;
	QList<AbstractAttribute*> terminal_names;
	QList<AbstractAttribute*> sys_file_refs;
	AbstractAttribute* time_symbol = nullptr;
    QString title;
    QString details;
    int edits;
	int change_count;
	QMap<QString,QString> getSymbolNames(QString type_name) const;
	QMap<QString, int> used_tags;
	QList<QString> getTags() const { return used_tags.keys(); };
	QMap<QString,int> pluginNames;
};

class ModelException : public std::exception {
public:
	enum Type { UnknownXSDType, UndefinedNode, InvalidNodeIndex, InvalidVersion, FileIOError };
	virtual const char* what() const throw();
	QString message;
	Type type;
	ModelException(Type t, QString n);
	~ModelException() throw() {};
};

#endif
