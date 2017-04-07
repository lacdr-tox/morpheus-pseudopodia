#include "vtk_plotter.h"

using namespace SIM;

REGISTER_PLUGIN(VtkPlotter);

void VtkPlotter::loadFromXML(const XMLNode node)	//einlesen der Daten aus der XML
{
	
	map<string, Mode> mode_map;
	mode_map["ascii"] = Mode::ASCII;
	mode_map["binary"] = Mode::BINARY;
	mode.setConversionMap(mode_map);
	mode.setXMLPath("mode");
	mode.setDefault("binary");
	registerPluginParameter(mode);

	// Define PluginParameters for all defined Output tags
	for (uint i=0; i<node.nChildNode("Channel"); i++) {
		
		shared_ptr<Channel> c( new Channel());
		
		c->symbol.setXMLPath("Channel["+to_str(i)+"]/symbol-ref");
		registerPluginParameter(c->symbol);
		
		c->celltype.setXMLPath("Channel["+to_str(i)+"]/celltype");
		registerPluginParameter(c->celltype);

		c->exclude_medium.setXMLPath("Channel["+to_str(i)+"]/exclude-medium");
		c->exclude_medium.setDefault("false");
		registerPluginParameter(c->exclude_medium);
		
		c->min_user.setXMLPath("Channel["+to_str(i)+"]/minimum");
		//c->min.setDefault(numeric_limits<double>::max());
		registerPluginParameter(c->min_user);
		
		c->max_user.setXMLPath("Channel["+to_str(i)+"]/maximum");
		//c->max.setDefault(numeric_limits<double>::min());
		registerPluginParameter(c->max_user);

		c->scale.setXMLPath("Channel["+to_str(i)+"]/scale");
		c->scale.setDefault("false");
		registerPluginParameter(c->scale);

		c->outline.setXMLPath("Channel["+to_str(i)+"]/outline");
		c->outline.setDefault("false");
		registerPluginParameter(c->outline);
		
		c->no_outline.setXMLPath("Channel["+to_str(i)+"]/no-outline");
		c->no_outline.setDefault("false");
		registerPluginParameter(c->no_outline);

		plot.channels.push_back(c);
	}

	AnalysisPlugin::loadFromXML(node);

};

void VtkPlotter::init(const Scope* scope)
{
	for(uint c=0; c<plot.channels.size(); c++){
		plot.channels[c]->celltype.init();
		if( plot.channels[c]->celltype.isDefined() ){
			plot.channels[c]->symbol.setScope(plot.channels[c]->celltype()->getScope());
		}
		
		cout << "VtkPlotter: Channel " << (c+1) << " = \'" << plot.channels[c]->symbol.accessor().getName() << "\'" << endl;
	}
	
	AnalysisPlugin::init( scope );

	latticeDim = SIM::getLattice()->size();
	
	if( !CPM::getCellTypes().empty() )
		cpm_layer = CPM::getLayer();

	plot_number=0;
};

//-------------------------------------------------------------------
void VtkPlotter::analyse(double time)
{
	writeVTK(time);
	plot_number++;	
};


// Implementation of binary format insipred by this post:
// http://stackoverflow.com/questions/10913666/error-writing-binary-vtk-files
void VtkPlotter::writeVTK(double time){

	ofstream vtkstream;
	stringstream fn;
	fn << "./plot_" << setfill('0') << setw(6) << plot_number << ".vtk";
	vtkstream.open(fn.str().c_str(), ios::out | ios::trunc);
	
	if( !vtkstream.is_open() ){
		cout << "Error opening file " << fn.str() << endl;
		return;
	}

	vtkstream << "# vtk DataFile Version 3.1" << endl; 
	vtkstream << "Morpheus" << endl;
	if( mode() == Mode::ASCII ){
		vtkstream << "ASCII" << "\n";
		vtkstream.close();
		vtkstream.clear();
		vtkstream.open(fn.str().c_str(), ios::out | ios::app );
	}
	else if( mode() == Mode::BINARY ){
		vtkstream << "BINARY" << "\n";
		vtkstream.close();
		vtkstream.clear();
		vtkstream.open(fn.str().c_str(), ios::out | ios::app | ios::binary);
	}

	vtkstream << "DATASET STRUCTURED_POINTS" << "\n";
	vtkstream << "DIMENSIONS " << latticeDim.x << " " << latticeDim.y << " " << latticeDim.z << "\n";
	vtkstream << "ORIGIN 0 0 0" << "\n";
	vtkstream << "SPACING 1 1 1" << "\n";
	vtkstream << "POINT_DATA " << latticeDim.x * latticeDim.y * latticeDim.z << "\n";
	
	int EmptyCellTypeID = CPM::getEmptyCelltypeID();
	for (uint c=0; c<plot.channels.size(); c++){
		
		//cout << "VtkPlotter: Channel " << (c+1) << " = \'" << plot.channels[c]->symbol.accessor().getName() << "\'" << endl;

		vtkstream << "SCALARS " << plot.channels[c]->symbol.accessor().getName() << " FLOAT 1" << "\n";
		vtkstream << "LOOKUP_TABLE default" << "\n";

		// Field
		if(plot.channels[c]->symbol.accessor().getLinkType() == SymbolData::PDELink){
			VINT pos(0,0,0);
			for (pos.z=0; pos.z<latticeDim.z; pos.z++){
				for (pos.y=0; pos.y<latticeDim.y; pos.y++){
					for (pos.x=0; pos.x<latticeDim.x; pos.x++){
						if( mode() == Mode::ASCII )
							vtkstream << float(plot.channels[c]->symbol.get(pos)) << " ";
						else {// binary
							float value = plot.channels[c]->symbol.get(pos);
							value = swap_endian( value ); // swap_endian<double>(value); 
							vtkstream.write((char*)& value, sizeof(float) );
							
						}
					}
				}
			}
		}
		else{ // cell properties
			VINT pos(0,0,0);
			for (pos.z=0; pos.z<latticeDim.z; pos.z++){
				for (pos.y=0; pos.y<latticeDim.y; pos.y++){
					for (pos.x=0; pos.x<latticeDim.x; pos.x++){
						
						float value = 0.0;
						if (cpm_layer) { // cell property or membrane
							uint celltype_at_pos = CPM::getCellIndex( cpm_layer->get(pos).cell_id ).celltype;
							
							// if cell type is specified, only plot property of that cell type, and assume 0 for other cell types
							if( plot.channels[c]->celltype.isDefined() ){
							
								if( plot.channels[c]->exclude_medium.get() 
									&& celltype_at_pos == EmptyCellTypeID ){
									value = 0;
								}
								// if pos is part of cell of chosen cell type, plot the value
								else if( celltype_at_pos == plot.channels[c]->celltype.get()->getID() ){

									if( plot.channels[c]->outline.get() ){
										if ( CPM::isSurface( pos ) ){
											value = plot.channels[c]->symbol.get( pos );
										}
									}
									else{ // if not outline, plot property over whole cell
										value = plot.channels[c]->symbol.get( pos );

										// if no-outline, set value to zero on cell boundary
										if( plot.channels[c]->no_outline.get() && CPM::isSurface( pos ) ){
											value = 0;
										}
									}
								}
								else{
									value = 0;
								}
							}
							// otherwise, plot property of all cell types
							else{
								if( ( plot.channels[c]->exclude_medium() && celltype_at_pos == EmptyCellTypeID ) ){
									value = 0;
								}
								else{
									if( plot.channels[c]->outline.get() ){
										if ( CPM::isSurface( pos ) ){
											value = plot.channels[c]->symbol.get( pos );
										}
									}
									else{
										value = plot.channels[c]->symbol.get( pos );
										
										// if no-outline, set value to zero on cell boundary
										if( plot.channels[c]->no_outline.get() && CPM::isSurface( pos ) ){
											value = 0;
										}
									}
								}
							}
							if( mode() == Mode::ASCII )
								vtkstream << value << " ";
							else if( mode() == Mode::BINARY ) {
								value = swap_endian(value); 
								vtkstream.write((char*)& value, sizeof(float) );
							}
						}
					} // end x
				}  // end y
			} // end z
		}  // end cell property
		vtkstream << "\n";
	} // end channel
	vtkstream.close();
	cout << time << ": VtkPlotter wrote '" << fn.str() << "'" << endl;

}


/*
void VtkPlotter::writeCPMLayer(double time)
{
	fstream fs;
	stringstream fn;
	fn << "./cpm_" << setfill('0') << setw(6) << plot_number << ".vtk";
	fs.open(fn.str().c_str(), fstream::out );
	if( !fs.is_open() ){
		cout << "Error opening file " << fn << endl;
		return;
	}

	fs << "# vtk DataFile Version 3.1" << endl; 
	fs << "CPM cells" << endl;
	fs << "ASCII" << endl;
	fs << "DATASET STRUCTURED_POINTS" << endl;
	fs << "DIMENSIONS " << latticeDim.x << " " << latticeDim.y << " " << latticeDim.z << endl;
	fs << "ORIGIN 0 0 0" << endl;
	fs << "SPACING 1 1 1" << endl;
	fs << "POINT_DATA " << latticeDim.x * latticeDim.y * latticeDim.z << endl;
	
	for (int i = 0; i < 2+properties.size(); i++)
	{
		if( i == 0)
			fs << "SCALARS celltype FLOAT 1" << endl;
		else if (i == 1)
			fs << "SCALARS cell_id FLOAT 1" << endl;
		else
			fs << "SCALARS " << properties[i-2].symbolref << " FLOAT 1" << endl;
		fs << "LOOKUP_TABLE default" << endl;

		uint medium_ct_index = CPM::getCellIndex( CPM::getEmptyState().cell_id ).celltype;
		VINT pos;
		for (pos.z=0; pos.z<latticeDim.z; pos.z++)
		{
			for (pos.y=0; pos.y<latticeDim.y; pos.y++)
			{
				for (pos.x=0; pos.x<latticeDim.x; pos.x++)
				{
					if( i == 0 ){ // cell type
						int celltype = CPM::getCellIndex( cpm_layer->get(pos).cell_id ).celltype;
						fs << (celltype == medium_ct_index ? -1 : celltype ) << "\n";
					}
					else if( i == 1 ){ // cell ID
						fs << cpm_layer->get(pos).cell_id << "\n";
					}
					else{ // properties
						if( properties[i-2].sa.getLinkType() == SymbolData::CellMembraneLink){ // if cell membrane, plot only at cell boundary
							if( CPM::isSurface( pos ) )
								fs << properties[i-2].sa.get(pos) << "\n";
							else
								fs << "-1\n";
						}
						else // normal property
							fs << properties[i-2].sa.get(pos) << "\n";
					}
				}
			}
		}
		fs << "\n";
	}

	fs.close();
};


void VtkPlotter::writePDELayer(double time)
{
		fstream fs;
		stringstream fn;
                fn << "pde_"  << setfill('0') << setw(6) << plot_number << ".vtk";
		fs.open(fn.str().c_str(), fstream::out );
		if( !fs.is_open() ){
			cout << "Error opening file " << fn.str() << endl;
			return;
		}
		
		fs << "# vtk DataFile Version 3.1" << endl; 
		fs << "PDE field" << endl;
		fs << "ASCII" << endl;
		fs << "DATASET STRUCTURED_POINTS" << endl;
		fs << "DIMENSIONS " << latticeDim.x << " " << latticeDim.y << " " << latticeDim.z << endl;
		fs << "ORIGIN 0 0 0" << endl;
		fs << "SPACING 1 1 1" << endl;
		fs << "POINT_DATA " << latticeDim.x * latticeDim.y * latticeDim.z << endl;
		
		for (int i = 0; i < pdes.size(); ++i)
		{
			fs << "SCALARS " << pdes[i].symbolref << " FLOAT 1" << endl;
			fs << "LOOKUP_TABLE default" << endl;
			
			VINT pos;
			for (pos.z=0; pos.z<latticeDim.z; pos.z++)
			{
				for (pos.y=0; pos.y<latticeDim.y; pos.y++)
				{
					for (pos.x=0; pos.x<latticeDim.x; pos.x++)
					{
						fs << pdes[i].pde->get(pos) << "\n";
					}
				}
			}
			fs << "\n";
		}

		fs.close();
		cout << time << ": VtkPlotter wrote '" << fn.str() << "'" << endl;
};
*/
//-------------------------------------------------------------------

void VtkPlotter::finish(double time){};


//-------------------------------------------------------------------

// void VtkPlotter::create_vtk(uint mcs)
// {
// 	fstream fs;
// 	stringstream sstr_vtk;
// 	sstr_vtk << "./cpm_" << mcs << ".vtk";
// 	fs.open(sstr_vtk.str().c_str(), fstream::out);
// 
// 	if(fs.is_open())
// 	{
// 		cout << "Saving " << sstr_vtk.str() << endl;
// 		fs.write((vtkText.str()).c_str(), (vtkText.str()).length());
// 		fs.close();
// 	}
// 	else
// 	{
// 		cout << "Oeffnen der Datei " << sstr_vtk.str() << " fehlgeschlagen!" << endl;
// 	}
// 
// 	vtkText.str("");
// };
// 
// //-------------------------------------------------------------------
// 
// //-------------------------------------------------------------------
// 
// set<VINT,less_VINT> VtkPlotter::getBoundaryNodes( const Cell& cell ){
//   
//     set<VINT,less_VINT> cell_boundary_nodes;
//     const set<VINT,less_VINT>& cell_nodes = cell.getNodes();
//     set<VINT,less_VINT>::const_iterator it;
//     for (it = cell_nodes.begin(); it != cell_nodes.end(); it++ )
//     {
// 	    VINT pos = *it;
// 	    cpm_layer->getLattice()->resolve(pos);
// 	    if( SIM::isCPMBoundary( pos) )
// 	      cell_boundary_nodes.insert( pos );
//     }
//     return cell_boundary_nodes;
// }
// 
// 
// void VtkPlotter::vtk_header()
// {
// 	vtkText << "# vtk DataFile Version 3.1 " << endl;
// 	vtkText << "cpmsim grid" << endl;
// 	vtkText << "ASCII" << endl;
// 	vtkText << "DATASET UNSTRUCTURED_GRID" << endl;
// 	vtkText << endl;
// };
// 
// //-------------------------------------------------------------------
// 
// void VtkPlotter::vtk_points()
// {
// 	VINT pos;
// 	i_anzNodes = 0;
// 	
// 	for(int i = 0; i < vec_ct.size(); i++)
// 	{
// 		if(vec_ct[i]->getName() != "Medium")
// 		{
// 			vector<CELL_ID> cell_ids = vec_ct[i]->getCellIDs();
// 			for(int j = 0; j < cell_ids.size(); j++)
// 			{
// 				i_anzNodes += getBoundaryNodes( CPM::getCell(cell_ids[j]) ).size();
// 			}
// 		}
// 	}
// 
// 	vtkText << "POINTS " << i_anzNodes << " FLOAT" << endl;
// 
// 	for(int i = 0; i < vec_ct.size(); i++)
// 	{
// 		if(vec_ct[i]->getName() != "Medium")
// 		{
// 			vector<CELL_ID> cell_ids = vec_ct[i]->getCellIDs();
// 			for(int j = 0; j < cell_ids.size(); j++)
// 			{
// 				const Cell::Nodes& list_cells = CPM::getCell(cell_ids[j]).getNodes();
// 				Cell::Nodes::const_iterator it;
// 				for (it = list_cells.begin(); it != list_cells.end(); it++ )
// 				{
// 					pos = *it;
// 					cpm_layer->getLattice()->resolve(pos);
// 					const & node = CPM::getNode(pos);
// 
// 					vtkText << pos.x << " " << pos.y << " " << pos.z << endl;
// 				}
// 			}
// 		}
// 	}
// 
// 	vtkText << endl;
// };
// 
// //-------------------------------------------------------------------
// 
// void VtkPlotter::vtk_cells()
// {
// 	i_anzCells = 0;
// 	uint id = 0;
// 
// 	for(int i = 0; i < vec_ct.size(); i++)
// 	{
// 		if(vec_ct[i]->getName() != "Medium")
// 		{
// 			i_anzCells += vec_ct[i]->getCellIDs().size();
// 		}
// 	}
// 
// 	vtkText << "CELLS " << i_anzCells << " " << i_anzCells + i_anzNodes << endl;
// 	
// 	for(int i = 0; i < vec_ct.size(); i++)
// 	{
// 		if(vec_ct[i]->getName() != "Medium")
// 		{
// 			vector<CELL_ID> cell_ids = vec_ct[i]->getCellIDs();
// 			for(int j = 0; j < cell_ids.size(); j++)
// 			{
// 				const Cell::Nodes& list_cells = CPM::getCell(cell_ids[j]).getNodes();
// 				Cell::Nodes::const_iterator it;
// 
// 				vtkText << list_cells.size();
// 
// 				for (it = list_cells.begin(); it != list_cells.end(); it++ )
// 				{
// 					vtkText << " " << id;
// 					id++;
// 				}
// 
// 				vtkText << endl;
// 			}
// 		}
// 	}
// 
// 	vtkText << endl;
// };
// 
// //-------------------------------------------------------------------
// 
// void VtkPlotter::vtk_cell_types()
// {
// 	vtkText << "CELL_TYPES " << i_anzCells << endl;
// 
// 	for(int i = 0; i < vec_ct.size(); i++)
// 	{
// 		if(vec_ct[i]->getName() != "Medium")
// 		{
// 			//uint num_celltypes = vec_ct.size();
// 			for(int j = 0; j < vec_ct[i]->getCellIDs().size(); j++)
// 			{
// 				vtkText << "2 ";
// 			}
// 		}
// 	}
// 
// 	vtkText << endl << endl;
// };
// 
// //-------------------------------------------------------------------
// 
// void VtkPlotter::vtk_point_data()
// {
// 	vtkText << "POINT_DATA " << i_anzNodes << endl;
// 	if(use_property)
// 		vtkText << "SCALARS " << property.getFullName() << " FLOAT" << endl;
// 	else
// 		vtkText << "SCALARS celltype FLOAT" << endl;
// 	vtkText << "LOOKUP_TABLE default" << endl;
// 
// 	for(int i = 0; i < vec_ct.size(); i++)
// 	{
// 		if(vec_ct[i]->getName() != "Medium")
// 		{
// 			vector <> cells = vec_ct[i]->getCellIDs();
// 			for(int j = 0; j < cells.size(); j++)
// 			{
// 				const Cell::Nodes& nodes =  CPM::getCell(cells[j]).getNodes();
// 				Cell::Nodes::const_iterator it;
// 				for (it = nodes.begin(); it != nodes.end(); it++ )
// 				{
// 					if(use_property)
// 						vtkText << property.get( cells[j] ) << endl;
// 					else // by default, print cell id
// 						vtkText << i << endl;
// 				}
// 			}
// 		}
// 	}
// 
// 	vtkText << endl;
// };
// 
// //-------------------------------------------------------------------
// 
// void VtkPlotter::vtk_cell_data()
// {
// 	
// 	vtkText << "CELL_DATA " << i_anzCells << endl;
// 	vtkText << "SCALARS cellID FLOAT" << endl;
// 	vtkText << "LOOKUP_TABLE default" << endl;
// 
// 	uint i_cellID = 0;
// 
// 	for(int i = 0; i < vec_ct.size(); i++)
// 	{
// 		if(vec_ct[i]->getName() != "Medium")
// 		{
// 			vector <> cells = vec_ct[i]->getCellIDs();
// 			for(int j = 0; j < cells.size(); j++)
// 			{
// 				//i_cellID ++;
// 				  vtkText << cells[j] << endl;
// 			}
// 		}
// 	}
// };
// 
// //-------------------------------------------------------------------
