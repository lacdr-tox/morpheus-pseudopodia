#include "init_rectangle.h"

REGISTER_PLUGIN(InitRectangle);

InitRectangle::InitRectangle() {
    numcells.setXMLPath("number-of-cells");
	registerPluginParameter( numcells );
	
	mode.setXMLPath("mode");
	map< string, Mode > convmap;
	convmap["regular"] = InitRectangle::REGULAR;
	convmap["random"] = InitRectangle::RANDOM;
	mode.setConversionMap( convmap );
	registerPluginParameter( mode );

	random_displacement.setXMLPath("random-offset");
	random_displacement.setDefault("0.0");
	registerPluginParameter( random_displacement );
	
	origin_eval.setXMLPath("Dimensions/origin");
	registerPluginParameter( origin_eval );
	
	size_eval.setXMLPath("Dimensions/size");
	registerPluginParameter( size_eval );
};

bool InitRectangle::run(CellType* ct)
{
	SymbolFocus global_focus = SymbolFocus();
	number_of_cells = numcells( global_focus );
	origin = origin_eval( global_focus );
	size   = size_eval( global_focus );
	shared_ptr<const Lattice> lattice = SIM::getLattice();
	
	cout << "InitRectangle: number of cells = " << number_of_cells << ", origin = " << origin << ", size = " << size << endl;

	if( mode() == InitRectangle::REGULAR && 
		random_displacement( global_focus ) > lattice->size().x && 
		random_displacement( global_focus ) > lattice->size().y )
	{
		cerr << "InitRectangle:: Displacement is larger than lattice size, defaulting to uniform random distribution" << endl;
		exit(-1);
	}
	if( mode() == InitRectangle::RANDOM && 
		random_displacement( global_focus ) > 0 ) 
	{
		cout << "InitRectangle: random_displacement > 0 only applicable with 'regular'. Makes no sense in combination with 'random' distribution." << endl;
	}

	celltype = ct;

	if(size.x > lattice->size().x || size.y > lattice->size().y || size.z > lattice->size().z){
		cerr << "InitRectangle:: Dimensions/size (" << size << ") is larger than lattice  (" << lattice->size() << ")" << endl;
		exit(-1);
	}
	
	switch( mode() ){
		case InitRectangle::RANDOM:
			setRandom();
			break;
		case InitRectangle::REGULAR:
			setRegular();
			break;
		default:
			cout << "no type defined" << endl;
			break;
	}
	return true;
}


//============================================================================

bool InitRectangle::createCell(VINT newPos)
{
	if(CPM::getLayer()->get(newPos) == CPM::getEmptyState() && CPM::getLayer()->writable(newPos))
	{
		uint newID = celltype->createCell();
		CPM::setNode(newPos, newID);
		//cout << " - creating cell " <<  ct->getCell(newID).getName() << endl;
		return true;
	}
	else // position is already occupied
	{
		return false;
	}
}

//============================================================================

void InitRectangle::setRandom()			//zufallsbelegung des angegebenen bereichs
{
	uint created_cells=0;
	for(uint i=0; i < number_of_cells; i++){

		int rnd_x = origin.x +  getRandomUint( size.x );
		int rnd_y = origin.y +  getRandomUint( size.y );
		int rnd_z = origin.z +  getRandomUint( size.z );
		
		VINT newPos(rnd_x, rnd_y, rnd_z);
		//= SIM::findEmptyCPMNode(origin,size); // zufallige leere Position im Gitter
		if ( createCell(newPos) ) created_cells++;
	
	}
	cout << "successfully created " << created_cells << " cells." << endl;
	if (created_cells < number_of_cells) {
		cout << "failed to create " << number_of_cells - created_cells << " cells." << endl;
		
	}
	
}

//============================================================================

void InitRectangle::setRegular()
{
	vector<double> vec_ids, vec_lines;	//koordinaten der knoten
	int i_lines, i_hlines, i_dlines, zeile, spalte, reihe;	//anzahl der linien, nummer der zeile, reihe und spalte

	shared_ptr<const Lattice> l = SIM::getLattice();
	uint created_cells=0;

	if (l->getDimensions() == 1) {
		double cell_distance = double (size.x) / (number_of_cells);
		for(int i = 0; i < number_of_cells; i++)
		{
			int line_position  =  ( double(i + 0.5) * cell_distance );
			VDOUBLE newPos;						//neue knoten an position anlegen
			newPos.x = origin.x + line_position;
			newPos.y = 0;
			newPos.z = 0;

			if(random_displacement( SymbolFocus() ) != 0.0){
                newPos.x += getRandom01() * random_displacement( SymbolFocus() ) - random_displacement( SymbolFocus() )/2 ;
			}

			created_cells += createCell(l->from_orth(newPos));
		}
	}
	if (l->getDimensions() == 2)	// wenn "2D", dann...
	{
		i_lines = int(sqrt((size.y * number_of_cells) / size.x) + 0.5);	//berechnung der linienanzahl
		i_lines = max(1, i_lines);
		double total_length  = i_lines * size.x;
		double cell_distance = total_length / (number_of_cells);
		double line_distance = (double)(size.y)/ (i_lines);
		line_distance = l->to_orth(VDOUBLE(0,line_distance,0)).y;
		
		for(int i = 0; i < number_of_cells; i++)
		{
			int line_position  =  ( double(i) * cell_distance );
			VDOUBLE newPos;						//neue knoten an position anlegen
			newPos.x = origin.x + line_position % size.x + 0.25;
			newPos.y = origin.y + (double (line_position /  size.x) ) * line_distance;
			newPos.z = 0;
			
			if(random_displacement( SymbolFocus() )  != 0.0){
                newPos.x += getRandom01() * random_displacement( SymbolFocus() ) - random_displacement( SymbolFocus() ) / 2 ;
                newPos.y += getRandom01() * random_displacement( SymbolFocus() ) - random_displacement( SymbolFocus() ) / 2 ;
                newPos.z += 0;
			}
			
			created_cells += createCell(l->from_orth(newPos));
		}

	}

	if (l->getDimensions() == 3)		//wenn "3D", dann...
	{
		if (size.x < 1) size.x=1;
		if (size.y < 1) size.y=1;
		if (size.z < 1) size.z=1;
		double cell_distance = pow((double(size.y * size.z * size.x) / number_of_cells), (1.0 / 3.0));		//abstand der linien

		i_dlines = max ( 1, int((size.z / cell_distance) + 0.5));
		
		cell_distance = sqrt(double(size.y * size.x) / i_dlines / number_of_cells);
		i_hlines = max ( 1, int((size.y / cell_distance) + 0.5));
		
		i_lines = i_hlines * i_dlines;	//anzahl der linien
		cell_distance = double(i_lines * size.x) / number_of_cells;	//erneuter abstand der linien (geändert durch rundung der linienanzahl)

		for(int i = 0; i < number_of_cells; i++)	//knotenpositionen festlegen
		{vec_ids.push_back(i * cell_distance);}
		
		VINT offset = VINT( int(0.5*cell_distance),int(0.5 * cell_distance),0);
		for(int i = 0; i < number_of_cells; i++)
		{
			zeile = int(vec_ids[i] / size.x);
			spalte = int((int)vec_ids[i] % int(size.x));
			reihe = int(zeile / i_hlines);
			zeile = int((int)zeile % i_hlines);

			VINT newPos;					//neue knoten an positionen anlegen
			newPos.y = int(origin.y + (zeile * cell_distance));
			newPos.x = int(origin.x + spalte);
			newPos.z = int(origin.z + (reihe * cell_distance));

            if(random_displacement( SymbolFocus() ) != 0.0){
                newPos.x += getRandom01() * random_displacement( SymbolFocus() ) - random_displacement( SymbolFocus() )/2 ;
                newPos.y += getRandom01() * random_displacement( SymbolFocus() ) - random_displacement( SymbolFocus() )/2 ;
                newPos.z += 0;
            }

			created_cells += createCell(newPos + offset);
		}
	}
	
	cout << "successfully created " << created_cells << " cells." << endl;
	if (created_cells < number_of_cells) {
		cout << "failed to create " << number_of_cells - created_cells << " cells." << endl;
	}
	
}

//============================================================================
