#include "symbol.h"

bool operator<(Granularity a, Granularity b) {
	switch(b) {
		case Granularity::Global:
			return a!=Granularity::Global;
		case Granularity::Cell:
			return a!=Granularity::Global && a!=Granularity::Cell;
		default:
			return false;
	}
}

bool operator<=(Granularity a, Granularity b) {
		switch(b) {
		case Granularity::Global:
			return true;
		case Granularity::Cell:
			return a!=Granularity::Global;
		default:
			return a==b;
	}
}

Granularity operator+(Granularity a, Granularity b) {
	switch (a) {
		case Granularity::Node :
			if (b == Granularity::MembraneNode)
				throw string("Incompatible granularity merge Granularity::MembraneNode + Granularity::Node");
			return a;
		case Granularity::MembraneNode:
			if (b == Granularity::Node)
				throw string("Incompatible granularity merge Granularity::MembraneNode + Granularity::Node");
			return a;
		case Granularity::Cell:
			if (b == Granularity::Node || b == Granularity::MembraneNode)
				return b;
			else
				return a;
		case Granularity::Global:
				return b;
	}
}

Granularity& operator+=(Granularity& g, Granularity b) {
	switch (g) {
		case Granularity::Node :
			if (b == Granularity::MembraneNode)
				throw string("Incompatible granularity merge Granularity::MembraneNode + Granularity::Node");
			break;
		case Granularity::MembraneNode:
			if (b == Granularity::Node)
				throw string("Incompatible granularity merge Granularity::MembraneNode + Granularity::Node");
			break;
		case Granularity::Cell :
			if (b == Granularity::Node || b == Granularity::MembraneNode)
				g = b;
			break;
		case Granularity::Global:
				g = b;
			break;
	}
	return g;
}

ostream& operator<<(ostream& out, Granularity g) {
	switch (g) {
		case Granularity::Node :
			out << "Node";
			break;
		case Granularity::MembraneNode:
			out << "MembraneNode";
			break;
		case Granularity::Cell :
			out << "Cell";
			break;
		case Granularity::Global:
			out << "Global";
			break;
	}
	return out;
}


string SymbolBase::Space_symbol = "SPACE";
string SymbolBase::MembraneSpace_symbol = "MEM_SPACE";
string SymbolBase::Time_symbol = "TIME";
string SymbolBase::CellID_symbol = "cell.id";
string SymbolBase::SuperCellID_symbol = "cell.super_id";
string SymbolBase::SubCellID_symbol = "cell.sub_id";
string SymbolBase::CellType_symbol = "cell.type";
string SymbolBase::CellCenter_symbol = "cell.center";
string SymbolBase::CellPosition_symbol = SymbolBase::CellCenter_symbol;
string SymbolBase::CellOrientation_symbol = "cell.orientation";
string SymbolBase::CellVolume_symbol = "cell.volume";
string SymbolBase::CellSurface_symbol = "cell.surface";
string SymbolBase::CellLength_symbol = "cell.length";
string SymbolBase::Temperature_symbol = "TEMPERATURE";

string sym_RandomUni  = "rand_uni";
string sym_RandomInt  = "rand_int";
string sym_RandomNorm = "rand_norm";
string sym_RandomBool = "rand_bool";
string sym_RandomGamma= "rand_gamma";
string sym_Modulo     = "mod";
