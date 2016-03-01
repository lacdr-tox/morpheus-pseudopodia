#include "vtkPlotter.h"

using namespace SIM;

//-------------------------------------------------------------------

REGISTER_PLUGIN(vtkPlotter);							//registrieren des neuen plugins

vtkPlotter::vtkPlotter()
{
	vec_ct = CPM::getCellTypes();
};

vtkPlotter::~vtkPlotter(){};

//-------------------------------------------------------------------


void vtkPlotter::loadFromXML(const XMLNode node)	//einlesen der Daten aus der XML
{
	Analysis_Listener::loadFromXML(node);
	
	for(uint i=0; i < node.nChildNode("CellProperty"); i++){
		XMLNode pNode = node.getChildNode("CellProperty",i);
		VTK_CellProperty cp;
		getXMLAttribute(pNode, "symbol-ref", cp.symbolref);
		properties.push_back(cp);
	}
	
	for (int i=0; i<node.nChildNode("PDE"); i++)
	{
		XMLNode xDataLayer = node.getChildNode("PDE",i);
		VTK_PDE p;
		getXMLAttribute(xDataLayer, "symbol-ref", p.symbolref);
		pdes.push_back( p );
	}
};


void vtkPlotter::init(double time)
{
	Analysis_Listener::init( time );

	latticeDim = SIM::getLattice()->size();
	
	for(uint i=0; i < properties.size(); i++){
		SymbolAccessor<double> p = SIM::findSymbol<double>(properties[i].symbolref);
                if( !p.valid() || (p.getLinkType() != SymbolData::CellPropertyLink && p.getLinkType() != SymbolData::CellMembraneLink)){
			cerr << "vtkPlotter: Error: property '" << properties[i].symbolref << "' is invalid or does not link to cell Property." << endl;
			assert( p.valid() );
                      assert( p.getLinkType() == SymbolData::CellPropertyLink || p.getLinkType() == SymbolData::CellMembraneLink);
			exit(-1);
		}
		properties[i].sa = p;
		properties[i].name = p.getFullName();
		
	}
	
	for(uint i=0; i < pdes.size(); i++){
		shared_ptr<PDE_Layer> p = SIM::findPDELayer(pdes[i].symbolref);
		//pdes[i].sa = SIM::findSymbol<double>(properties[i].symbolref);
		pdes[i].name = p->getSymbol();
		pdes[i].pde = p;
	}

        plot_number=0;
};


set< string > vtkPlotter::getDependSymbols()
{
	set<string> symbols;
	for(uint i=0; i < properties.size(); i++){
		symbols.insert(properties[i].symbolref);
	}
	return  symbols;
}

//-------------------------------------------------------------------

void vtkPlotter::notify(double time)
{
/*	vtkPlotter::vtk_header();
	vtkPlotter::vtk_points();
	vtkPlotter::vtk_cells();
	vtkPlotter::vtk_cell_types();
	vtkPlotter::vtk_point_data();
	vtkPlotter::vtk_cell_data();
	vtkPlotter::create_vtk(mcs);*/
	
	//cout << time << ": vtkPlotter::mcs_notify() "<< endl;

	Analysis_Listener::notify(time);
	plot_number++;
	
	if( properties.size() > 0)
		vtkPlotter::writeCPMLayer( time );
	
	if( pdes.size() > 0 )
		vtkPlotter::writePDELayer( time );
	
};




void vtkPlotter::writeCPMLayer(double time)
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
								if( CPM::isBoundary( pos ) )
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


void vtkPlotter::writePDELayer(double time)
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

//-------------------------------------------------------------------

void vtkPlotter::finish(double time){};


//-------------------------------------------------------------------

// void vtkPlotter::create_vtk(uint mcs)
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
// set<VINT,less_VINT> vtkPlotter::getBoundaryNodes( const Cell& cell ){
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
// void vtkPlotter::vtk_header()
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
// void vtkPlotter::vtk_points()
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
// void vtkPlotter::vtk_cells()
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
// void vtkPlotter::vtk_cell_types()
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
// void vtkPlotter::vtk_point_data()
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
// void vtkPlotter::vtk_cell_data()
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
