#include "population_reporter.h"

REGISTER_PLUGIN(PopulationReporter);

PopulationReporter::PopulationReporter(): TimeStepListener(ScheduleType::REPORTER) {} 

void PopulationReporter::loadFromXML(const XMLNode xNode)
{
	TimeStepListener::loadFromXML ( xNode );
	stored_xml = xNode;
	if(xNode.nChildNode("Input"))
		getXMLAttribute(xNode,"Input/symbol-ref",input_string);
	if(xNode.nChildNode("InputVector"))
		getXMLAttribute(xNode,"InputVector/symbol-ref",input_string_vec);

	getXMLAttribute(xNode,"Output/symbol-ref",output_string);
	
	mapping_string = "sum";
	getXMLAttribute(xNode,"mapping",mapping_string);
	lower_case(mapping_string);
	
}

void PopulationReporter::init()
{
	TimeStepListener::init();

	celltype = SIM::getScope()->getCellType();
	lattice = SIM::getLattice();
	
	if( !input_string.empty() ){
		input_symbol = SIM::findSymbol<double>(input_string,ct);
		if( input_symbol.getLinkType() != SymbolData::CellPropertyLink && 
			input_symbol.getLinkType() != SymbolData::FunctionLink &&
			input_symbol.getLinkType() != SymbolData::CellOrientationLink ){
			cerr << "PopulationReporter: Input must be a Property, Function or CellOrientation symbol (now: " << input_symbol.getLinkType() << ")" << endl;
			exit(-1);
		}
	}
	else{
		 if (mapping_string == "positional_regularity") {
			 input_string_vec = "cell.center";
		 }
		input_symbol_vec= SIM::findSymbol<VDOUBLE>(input_string_vec,ct);
		if( input_symbol_vec.getLinkType() != SymbolData::CellPropertyLink && 
			input_symbol_vec.getLinkType() != SymbolData::FunctionLink &&
			input_symbol_vec.getLinkType() != SymbolData::CellOrientationLink &&
			input_symbol_vec.getLinkType() != SymbolData::CellCenterLink){
			cerr << "PopulationReporter: Input must be a Property, Function or CellOrientation symbol (now: " << input_symbol_vec.getLinkType() << ")" << endl;
			exit(-1);
		}
		cout << input_symbol_vec.getBaseName() << endl;

	}
	
	
	output_symbol = SIM::findRWSymbol<double>(output_string);
	
	if (mapping_string == "average") {
		mapping = MAP_AVERAGE;
	} else if (mapping_string == "sum") {
		mapping = MAP_SUM;
	} else if (mapping_string == "variance") {
		mapping = MAP_VARIANCE;
	} else if (mapping_string == "minimum") {
		mapping = MAP_MIN;
	} else if (mapping_string == "maximum") {
		mapping = MAP_MAX;
	} else if (mapping_string == "ave_direction") {
		mapping = MAP_AVERAGE_DIRECTION;
	} else if (mapping_string == "var_direction") {
		mapping = MAP_VARIANCE_DIRECTION;
	} else if (mapping_string == "positional_regularity") {
		mapping = MAP_POSITIONAL_REGULARITY;
	} else {
		cerr << "PopulationReporter: Unknown mapping: " << mapping_string << endl;
		exit(-1);
	}

	if( (input_string.empty() && input_string_vec.empty()) || output_string.empty() ){
		cerr << "PopulationReporter: input and/or output is not specified" << endl;
		exit(-1);
	}

}


void PopulationReporter::computeTimeStep(){
	TimeStepListener::computeTimeStep();
	
	vector<CPM::CELL_ID> cells = celltype->getCellIDs();
	valarray<double> data_d(cells.size());
	vector<VDOUBLE> data_v(cells.size(), VDOUBLE(0.,0.,0.));
	
	for (int i=0; i < cells.size(); i++ ) {
		SymbolFocus focus( cells[ i ] );
		
		if( mapping == MAP_AVERAGE_DIRECTION || mapping == MAP_VARIANCE_DIRECTION  ){
			data_v.push_back( input_symbol_vec.get( focus ) );
		}
		else if( mapping == MAP_POSITIONAL_REGULARITY  ){
			//data_v.push_back( focus.cell().getCenter() );
		}
		else{
			data_d[i] = input_symbol.get( focus );
		}
	}

	SymbolFocus focus;
	switch (mapping) {
		case ( MAP_AVERAGE ) :
			output_symbol.set( focus, data_d.sum()/data_d.size() );
			break;
		case ( MAP_SUM ) :
			output_symbol.set( focus, data_d.sum() );
			break;
		case ( MAP_VARIANCE ) :
			// centering
			data_d -= data_d.sum()/data_d.size();
			output_symbol.set( focus, (data_d*data_d).sum() / data_d.size() );
			break;
		case ( MAP_MIN ) :
			output_symbol.set( focus, data_d.min() );
			break;
		case ( MAP_MAX ) :
			output_symbol.set( focus, data_d.max() );
			break;
		case ( MAP_AVERAGE_DIRECTION ) :
		case ( MAP_VARIANCE_DIRECTION ) :{
			VDOUBLE mean_vec(0.,0.,0.);
			for(uint i=0; i<data_v.size(); i++){
				mean_vec.x += data_v[i].x;
				mean_vec.y += data_v[i].y;
				mean_vec.z += data_v[i].z;
			}
			mean_vec.x /= data_v.size();
			mean_vec.y /= data_v.size();
			mean_vec.z /= data_v.size();

			if( mapping == MAP_AVERAGE_DIRECTION){
				output_symbol.set( focus, mean_vec.abs() );
			}
			else if( mapping == MAP_VARIANCE_DIRECTION){
				VDOUBLE variance_vec(0.,0.,0.);
				for(uint i=0; i<data_v.size(); i++){
					variance_vec.x = sqr(data_v[i].x - mean_vec.x);
					variance_vec.y = sqr(data_v[i].y - mean_vec.y);
					variance_vec.z = sqr(data_v[i].z - mean_vec.z);
				}
				variance_vec.x /= data_v.size();
				variance_vec.y /= data_v.size();
				variance_vec.z /= data_v.size();
				output_symbol.set( focus, variance_vec.abs()/mean_vec.abs() );
			}
			break;
		}
		case ( MAP_POSITIONAL_REGULARITY ) :{
			/* Regularity index (RI): 
			 * - Nearest neighbor (NN) method:
			 *   RI = mean distance to NN / standard deviation
			 *   
			 * For other methods, see:
			 * The patterning of retinal horizontal cells: normalizing the regularity index enhances the detection of genomic linkage
			 * October 2014
			 * http://journal.frontiersin.org/Journal/10.3389/fnana.2014.00113/abstract
			 * 
			 */
			
			valarray<double> NN(0.0, cells.size());
			
			for (int i=0; i < cells.size(); i++ ) {
				SymbolFocus cellfocus( cells[ i ] );
				
				VDOUBLE center = cellfocus.cell().getCenter();
				double mindist = 9999.9;
				
				// get nearest neighbor
				const map <CPM::CELL_ID, double >& interfaces = cellfocus.cell().getInterfaceLengths();
				for (map<CPM::CELL_ID,double>::const_iterator nb = interfaces.begin(); nb != interfaces.end(); nb++) {
					CPM::CELL_ID cell_id = nb->first;
					if ( cell_id == CPM::getEmptyState().cell_id )
						continue;
					SymbolFocus nbfocus(cell_id);
					double dist = lattice->orth_distance( center, nbfocus.cell().getCenter() ).abs();
					if( dist < mindist )
						mindist=dist;
				}
				NN[i]=mindist;
			}
			
			
			double meanNN = NN.sum() / cells.size();
			double sumDiffSqr = 0.;
			for (int i = 0; i < cells.size(); i++)
			{
// 				cout << "NN dist = " << NN[i] << "\n";
				sumDiffSqr = sumDiffSqr + (NN[i] - meanNN)*(NN[i] - meanNN);
			}
			double stdDev = sqrt(sumDiffSqr / cells.size());
			
			double regularity_index = meanNN / stdDev;
// 			cout << "RI " << regularity_index << " = meanNN " << meanNN << " / sd " << stdDev <<  endl;
			
			output_symbol.set(focus, regularity_index);
			break;
		}
		default:
			cerr << "PopulationReporter: Unknown mapping" << mapping_string << endl;
			exit(-1);
			break;
	}
}

void PopulationReporter::executeTimeStep() {
    TimeStepListener::executeTimeStep();
}



set< string > PopulationReporter::getDependSymbols() {
    set< string > s;
	if( !input_string.empty() )
		s.insert( input_string );
	if( !input_string_vec.empty() )
		s.insert( input_string_vec );
	
    return s;
}

set< string > PopulationReporter::getOutputSymbols() {
    set< string > s;
	s.insert ( output_string );
    return s;
}



