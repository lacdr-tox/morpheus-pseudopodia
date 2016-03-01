#include "membrane_reporter.h"


REGISTER_PLUGIN ( MembraneReporter );

MembraneReporter::MembraneReporter(): TimeStepListener(ScheduleType::REPORTER) {}


void MembraneReporter::computeTimeStep() { // TODO this function directly outputs the data
	// However, we wanted to buffer them and write later in executeStep()
	TimeStepListener::computeTimeStep();
	vector<CPM::CELL_ID> cells = celltype -> getCellIDs();

#pragma omp parallel for
	for ( int c=0; c < cells.size(); c++ ) {
		CPM::CELL_ID cell_id = cells[c];
		VDOUBLE cell_center;

		// get list of neighbor nodes, sorted by angle
		Cell::Nodes neighbor_nodes; // holds nodes of neighbors of membrane nodes. Used for <concentration>
		const Cell::Nodes& membrane = CPM::getCell ( cell_id ).getSurface();
		for ( Cell::Nodes::const_iterator m = membrane.begin(); m != membrane.end(); ++m ) {
			// check the von neumann neighborhood
			// gain the halo of nodes surrounding the cell
			for ( int i_nei = 0; i_nei < neighbor_sites.size(); ++i_nei ) {
				VINT neighbor_position = ( *m ) + neighbor_sites[i_nei];
				const CPM::STATE& nb_spin = cpm_layer->get ( neighbor_position );

				if ( cell_id != nb_spin.cell_id ) { // if neighbor is different from me
					neighbor_nodes.insert ( neighbor_position ); // add neighbor node to list of unique neighboring points (used for layers below)
				}
			}
		}
		if (neighbor_nodes.empty() ) {
			cout << "MembraneReporter refuses to report on cell " << cell_id << " because it has no surface" << endl;
			cout << "Cell "<< cell_id << " Membrane size was " << membrane.size() << endl;
			cout << "Cell "<< cell_id << " Cell size was " << CPM::getCell ( cell_id ).nNodes() << endl;
			celltype->removeCell( cell_id );
			continue;
		}
		assert( !neighbor_nodes.empty() );

		for ( uint r = 0; r < reporters.size(); r++ ) {
			MembraneMapper::MODE mode;
			if (reporters[r].input == Reporter::IN_CELLTYPE) {
				mode = MembraneMapper::MAP_BOOLEAN;
			}
			else if (reporters[r].input == Reporter::IN_CELLTYPE_DISTANCE) {
				mode = MembraneMapper::MAP_DISTANCE_TRANSFORM;
			}
			else if (reporters[r].datatype == Reporter::DATA_DISCRETE) {
				mode = MembraneMapper::MAP_DISCRETE;
			}
			else
				mode = MembraneMapper::MAP_CONTINUOUS;
			
			if(reporters[r].input == Reporter::IN_CELLSHAPE){
				cell_center =  CPM::getCell( cell_id ).getCenter();
			}

			MembraneMapper mapper(mode,(mode == MembraneMapper::MAP_DISTANCE_TRANSFORM && reporters[r].deformed_sphere));
			mapper.attachToCell(cell_id);
			for ( Cell::Nodes::const_iterator i=neighbor_nodes.begin(); i != neighbor_nodes.end(); ++i ) {
				SymbolFocus f(*i);
				if ( reporters[r].input == Reporter::IN_CELLTYPE || reporters[r].input == Reporter::IN_CELLTYPE_DISTANCE ) {
					mapper.map(*i, f.cell_index().celltype == reporters[r].input_celltype_id);
				} 
				else if ( reporters[r].input == Reporter::IN_CELLSHAPE) {
					double distance = (*i - cell_center).abs();
					mapper.map(*i, distance);
				} 
				else { // Reporter::IN_CONCENTRATION
					double val = reporters[r].input_symbol.get(f);
					if (std::isnan(val))
						mapper.map(*i,0);
					else 
						mapper.map(*i,val);
				}
			}

			mapper.fillGaps();

			if ( reporters[r].output == Reporter::OUT_MEMBRANEPROPERTY ) {
				if ( reporters[r].debug_distance )
					mapper.copyDistance(reporters[r].output_membrane.getMembrane(cell_id));
				else if ( reporters[r].debug_sources )
					mapper.copyAccum(reporters[r].output_membrane.getMembrane(cell_id));
				else if (reporters[r].input == Reporter::IN_CELLTYPE_DISTANCE)
					mapper.copyDistance(reporters[r].output_membrane.getMembrane(cell_id));
				else
					mapper.copyData(reporters[r].output_membrane.getMembrane(cell_id));
			} 
			else if ( reporters[r].output == Reporter::OUT_CELLPROPERTY ) {
				switch (reporters[r].mapping) {
					case  Reporter::MAP_SUM:
						reporters[r].output_cellproperty.set ( cell_id, mapper.getData().sum() );
						break;
					case Reporter::MAP_AVERAGE:
						reporters[r].output_cellproperty.set ( cell_id,  mapper.getData().mean() );
						break;
					case Reporter::MAP_DIFFERENCE:
						reporters[r].output_cellproperty.set ( cell_id, mapper.getData().max_val() - mapper.getData().min_val() );
						break;
					case Reporter::MAP_VARIANCE:
						reporters[r].output_cellproperty.set ( cell_id,  mapper.getData().variance() );
						break;
				}
			}
		}
	}
}


void MembraneReporter::init () {
	TimeStepListener::init();
	celltype = SIM::getScope()->getCellType();
	buffer.resize ( MembraneProperty::resolution );

	cpm_layer 		= CPM::getLayer();
	lattice			= SIM::getLattice();
	neighbor_sites	= lattice->getNeighborhood( 1 );

	for ( uint i=0; i < reporters.size(); i++ ) {

		if ( reporters[i].input == Reporter::IN_CELLTYPE || reporters[i].input == Reporter::IN_CELLTYPE_DISTANCE ) {
			reporters[i].input_celltype_id = CPM::findCellType ( reporters[i].input_celltype_str )->getID();
		}
		else if ( reporters[i].input == Reporter::IN_PDE ) {
			reporters[i].input_symbol = findGlobalSymbol<double>( reporters[i].input_symbol_str );
		}
		else if ( reporters[i].input == Reporter::IN_MEMBRANE) {
			// Create a Symbol that defaults to zero if there is no container in the back ... (i.e. no membrane defined in a medium celltype)
			reporters[i].input_symbol = findGlobalSymbol<double> ( reporters[i].input_symbol_str );
		}
		
		if ( reporters[i].output == Reporter::OUT_MEMBRANEPROPERTY ) {
			reporters[i].output_membrane = celltype->findMembrane ( reporters[i].output_symbol_str, true );
		} else if ( reporters[i].output == Reporter::OUT_CELLPROPERTY ) {
			// HACK: If we output information of a Membrane to a CellProperty, we set output_membrane to the input string
			if ( reporters[i].input != Reporter::IN_CELLTYPE && reporters[i].input != Reporter::IN_CELLTYPE_DISTANCE) {
				reporters[i].output_membrane = celltype->findMembrane ( reporters[i].input_symbol_str, true );
			}
			reporters[i].output_cellproperty = findRWSymbol<double> ( reporters[i].output_symbol_str);
			if ( reporters[i].output_mapping_str == string ( "sum" ) )
				reporters[i].mapping = Reporter::MAP_SUM;
			if ( reporters[i].output_mapping_str == string ( "average" ) )
				reporters[i].mapping = Reporter::MAP_AVERAGE;
			if ( reporters[i].output_mapping_str == string ( "difference" ) )
				reporters[i].mapping = Reporter::MAP_DIFFERENCE;
			if ( reporters[i].output_mapping_str == string ( "variance" ) )
				reporters[i].mapping = Reporter::MAP_VARIANCE;
		}
	}
}

void MembraneReporter::executeTimeStep() {
    TimeStepListener::executeTimeStep();
}

set< string > MembraneReporter::getDependSymbols() {
    set< string > s;
    for ( uint i=0; i< reporters.size(); i++ ) {
		if( reporters[i].input == Reporter::IN_CELLTYPE ||
			reporters[i].input == Reporter::IN_CELLTYPE_DISTANCE || 
			reporters[i].input == Reporter::IN_CELLSHAPE ){
		}
		else{ 
			s.insert ( reporters.at( i ).input_symbol_str );
		}
    }
    return s;
}

set< string > MembraneReporter::getOutputSymbols() {
    set< string > s;
    for ( uint i=0; i< reporters.size(); i++ ) {
		if ( reporters[i].output == Reporter::OUT_MEMBRANEPROPERTY ){
			s.insert ( reporters.at( i ).output_membrane.getSymbol() );
		}
		else{
			s.insert ( reporters.at( i ).output_symbol_str );
		}
	}
    return s;
}


void MembraneReporter::loadFromXML ( const XMLNode xNode ) {

	TimeStepListener::loadFromXML ( xNode );

	for ( int i=0; i<xNode.nChildNode ( "Reporter" ); i++ ) {
		XMLNode xReporter= xNode.getChildNode ( "Reporter",i );
		Reporter reporter;

		// Input
		XMLNode xInput  = xReporter.getChildNode ( "Input" );
		if ( xInput.nChildNode ( "Contact" ) ) {
			reporter.datatype = Reporter::DATA_BOOLEAN;
			reporter.input = Reporter::IN_CELLTYPE;
			if ( !getXMLAttribute ( xInput, "Contact/celltype", reporter.input_celltype_str ) ) {
				cerr << "MembraneReporter::loadFromXML: Input Contact/celltype not defined!" << endl;
				exit ( -1 );
			}
		}
		else if (xInput.nChildNode ( "ContactDistance" )) {
			reporter.datatype = Reporter::DATA_CONTINUOUS;
			reporter.input = Reporter::IN_CELLTYPE_DISTANCE;
			reporter.deformed_sphere = false;
			if ( !getXMLAttribute ( xInput, "ContactDistance/celltype", reporter.input_celltype_str ) ) {
				cerr << "MembraneReporter::loadFromXML: Input ContactDistance/celltype not defined!" << endl;
				exit ( -1 );
			}
			getXMLAttribute(xInput, "ContactDistance/cell_shape", reporter.deformed_sphere);
		}
		else if ( xInput.nChildNode ( "PDE" ) ) {
			reporter.datatype = Reporter::DATA_CONTINUOUS;
			reporter.input = Reporter::IN_PDE;
			if ( !getXMLAttribute ( xInput, "PDE/symbol-ref", reporter.input_symbol_str ) ) {
				cerr << "MembraneReporter::loadFromXML: Input PDE/symbol not defined!" << endl;
				exit ( -1 );
			}
		}
		else if ( xInput.nChildNode ( "MembraneProperty" ) ) {
			reporter.input = Reporter::IN_MEMBRANE;
			if ( !getXMLAttribute ( xInput, "MembraneProperty/symbol-ref", reporter.input_symbol_str ) ) {
				cerr << "MembraneReporter::loadFromXML: Input MembraneProperty/symbol not defined!" << endl;
				exit ( -1 );
			}
			reporter.datatype = Reporter::DATA_CONTINUOUS;
			string data_str;
			getXMLAttribute ( xInput, "MembraneProperty/data", data_str );
			if(data_str == "boolean")
				reporter.datatype = Reporter::DATA_BOOLEAN;
			else if(data_str == "discrete")
				reporter.datatype = Reporter::DATA_DISCRETE;
			else //if(data_str == "boolean")
				reporter.datatype = Reporter::DATA_CONTINUOUS;	
		}
		else if ( xInput.nChildNode ( "CellShape" ) ) {
			reporter.datatype = Reporter::DATA_CONTINUOUS;
			reporter.input = Reporter::IN_CELLSHAPE;
		}
		else {
			cerr << "MembraneReporter::loadFromXML: Unknown Input ! (num children: " << xInput.nChildNode() << ")" << endl;
			exit ( -1 );
		}

		// Output
		XMLNode xOutput = xReporter.getChildNode ( "Output" );
		if ( xOutput.nChildNode ( "MembraneProperty" ) ) {
			reporter.output = Reporter::OUT_MEMBRANEPROPERTY;
			getXMLAttribute ( xOutput, "MembraneProperty/symbol-ref", reporter.output_symbol_str );
 			reporter.debug_distance=false;
			getXMLAttribute ( xOutput, "distance", reporter.debug_distance );
			reporter.debug_sources=false;
			getXMLAttribute ( xOutput, "sources", reporter.debug_sources );
		} else if ( xOutput.nChildNode ( "CellProperty" ) ) {
			reporter.output = Reporter::OUT_CELLPROPERTY;
			if ( !getXMLAttribute ( xOutput, "CellProperty/symbol-ref", reporter.output_symbol_str ) ) {
				cerr << "MembraneReporter::loadFromXML: Output CellProperty/symbol not defined!" << endl;
				exit ( -1 );
			}

			if ( !getXMLAttribute ( xOutput, "CellProperty/mapping", reporter.output_mapping_str ) ) {
				cerr << "MembraneReporter::loadFromXML: Output CellProperty/mapping not defined (compulsory when mapping to membranePDE to cellProperty)!" << endl;
				exit ( -1 );
			}
			lower_case ( reporter.output_mapping_str );
		} else {
			cerr << "MembraneReporter::loadFromXML: Unknown Output !" << endl;
			exit ( -1 );
		}

		reporters.push_back ( reporter );
	}

}

/*
double MembraneReporter::angle2d(VDOUBLE v1, VDOUBLE v2) const{
	return atan2(v1.x*v2.y-v2.x*v1.y, v1*v2) + M_PI; // [-PI, PI]
}*/
