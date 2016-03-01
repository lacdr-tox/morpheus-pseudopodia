#include "symbol.h"
#include "simulation.h"

const string TypeInfo<double>::name() { return "Double"; };
const string TypeInfo<float>::name() { return "Float"; };
const string TypeInfo<bool>::name() { return "Boolean"; };
template <> const string TypeInfo<VDOUBLE>::name() { return "Vector"; };
template <> const string TypeInfo<vector<double> >::name() { return "Array";};
template <> const string TypeInfo<double_queue >::name() { return "Queue"; };


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
			if (b != Granularity::Undef)
				return b;
		case Granularity::Undef :
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
			if (b != Granularity::Undef)
				g = b;
			break;
		case Granularity::Undef :
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
		case Granularity::Undef :
			out << "Undef";
			break;
	}
	return out;
}


string SymbolData::Space_symbol = "SPACE";
string SymbolData::MembraneSpace_symbol = "MEM_SPACE";
string SymbolData::Time_symbol = "TIME";
string SymbolData::CellID_symbol = "cell.id";
string SymbolData::SuperCellID_symbol = "cell.super_id";
string SymbolData::SubCellID_symbol = "cell.sub_id";
string SymbolData::CellType_symbol = "cell.type";
string SymbolData::CellCenter_symbol = "cell.center";
string SymbolData::CellPosition_symbol = SymbolData::CellCenter_symbol;
string SymbolData::CellOrientation_symbol = "cell.orientation";
string SymbolData::CellVolume_symbol = "cell.volume";
string SymbolData::CellSurface_symbol = "cell.surface";
string SymbolData::CellLength_symbol = "cell.length";
string SymbolData::Temperature_symbol = "TEMPERATURE";

string sym_RandomUni  = "rand_uni";
string sym_RandomNorm = "rand_norm";
string sym_RandomBool = "rand_bool";
string sym_RandomGamma= "rand_gamma";
string sym_Modulo     = "mod";


string SymbolData::getLinkTypeName(LinkType linktype) {
	switch (linktype) {
		case GlobalLink:
			return "GlobalLink";
		case SymbolData::CellPropertyLink:
			return "CellProperty";
		case SymbolData::PDELink:
			return "PDE";
		case SymbolData::CellMembraneLink:
			return "CellMembrane";
		case SingleCellPropertyLink:
			return "Single Celltype CellProperty";
		case SingleCellMembraneLink:
			return "Singel CellType CellMembrane";
		case SymbolData::FunctionLink:
			return "Function";
		case SymbolData::Space:
			return "Space";
		case SymbolData::MembraneSpace:
			return "MembraneSpace";
		case SymbolData::VecXLink :
			return "VecXLink";
		case SymbolData::VecYLink :
			return "VecYLink";
		case SymbolData::VecZLink :
			return "VecZLink";
		case SymbolData::VecAbsLink:
			return "VecAbsLink";
		case SymbolData::VecPhiLink:
			return "VecPhiLink";
		case SymbolData::VecThetaLink:
			return "VecThetaLink";
		case SymbolData::Time:
			return "Time";
		case SymbolData::CellIDLink:
			return "Cell.ID";
		case SymbolData::SubCellIDLink:
			return "Cell.Sub_ID";
		case SymbolData::SuperCellIDLink:
			return "Cell.Super_ID";
		case SymbolData::CellTypeLink:
			return "Cell.Type";
        case SymbolData::CellCenterLink:
            return "Cell.Center";
        case SymbolData::CellOrientationLink:
            return "Cell.Orientation";    
        case SymbolData::CellVolumeLink:
            return "Cell.Volume";
        case SymbolData::CellSurfaceLink:
            return "Cell.Surface";
        case SymbolData::CellLengthLink:
            return "Cell.Length";
		case SymbolData::PopulationSizeLink:
			return "PopulationSizeLink";
		case SymbolData::CompositeSymbolLink:
			return "Composite Symbol Link";
		case SymbolData::UnLinked:
			return "! Unlinked symbol !";
	}
	return "! Unknown symbol type !";
}

string SymbolData::getLinkTypeName() const {
    return getLinkTypeName(link);
}
