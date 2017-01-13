#include "tiff_reader.h"

REGISTER_PLUGIN(TIFFReader);

TIFFReader::TIFFReader(){
	filename.setXMLPath("filename");
	registerPluginParameter( filename );

	offset.setXMLPath("offset");
	offset.setDefault("0 0 0");
	registerPluginParameter( offset );
	
	keepIDs.setXMLPath("keepIDs");
	keepIDs.setDefault("false");
	registerPluginParameter( keepIDs );
	
	scaleToMax.setXMLPath("scaleToMax");
	scaleToMax.setDefault("1.0");
	registerPluginParameter( scaleToMax );
	
	skipped_nodes=0;
	created_nodes=0;
	created_cells=0;
}

bool TIFFReader::run(PDE_Layer* pde) 
{
	struct timeval t1, t2; double elapsedTime;
	gettimeofday( &t1, NULL);

	mode = TIFFReader::PDE;
	pde_layer = pde;
	bool success = loadTIFF();
	if (!success){
		cerr << "TIFFReader: Could not read '" << filename() << "'." << endl;
		exit(-1);
	}

	gettimeofday( &t2, NULL);
	elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
	elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms

	cout << "TIFFReader: loading TIFF for '" << pde->getName() << "' took " <<  elapsedTime  << " ms.\n";
	return success;
}

bool TIFFReader::run(CellType* ct)
{
	
	struct timeval t1, t2; double elapsedTime;
	gettimeofday( &t1, NULL);

	mode = TIFFReader::CELLS;
	celltype = ct;
	bool success = loadTIFF();
	if (!success){
		cerr << "TIFFReader: Could not read '" << filename() << "'." << endl;
		exit(-1);
	}
	gettimeofday( &t2, NULL);
	elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
	elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms

	cout << "TIFFReader: loading TIFF for '"<< ct->getName() << "' took " <<  elapsedTime  << " ms.\n"; 
	
	return success;
}

bool TIFFReader::loadTIFF(){
	
	empty_state = CPM::getEmptyState().cell_id;

	VINT offset_ = VINT( offset().x, (SIM::getLattice()->getDimensions() > 1 ? offset().y : 0), (SIM::getLattice()->getDimensions() >2 ? offset().z : 0));
	if( offset_.abs() > 0 ){
		cout << "TIFFReader::loadTIFF: Offset = " << offset_ << endl;
	}

	TIFFSetWarningHandler(0);	
	TIFF* tif = TIFFOpen(filename().c_str(), "r");
	
	if (tif) {
		uint32 w, h;
		unsigned short int numbits, numsamples;
		size_t npixels;
		uint32* raster8;
		uint16* raster16;
		float* raster32;
		
		TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
		TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
		TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &numbits);
		TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &numsamples);

		cout << "TIFF Reader, Image size = " << w << " , " << h << "\n";
		
		// check whether it fits within lattice
		if ( w > SIM::getLattice()->size().x ) {
			cerr << "TIFFReader: Image width (x) too large for the lattice! ("<< w <<" > "<<SIM::getLattice()->size().x<<")";
			exit(-1);
		}
		if ( h > SIM::getLattice()->size().y ) {
			cerr << "TIFFReader: Image height (y) too large for the lattice! ("<< h <<" > "<<SIM::getLattice()->size().y<<")";
			exit(-1);
		}
		
		// allocate memory for buffer
		npixels = w * h;
		switch (numbits){
			
			case 8:
			{
				cout << "TIFFReader: TIFF image is 8 bit.\n";
				raster8 = (uint32*) _TIFFmalloc(npixels * sizeof (uint32));
				if( mode == TIFFReader::PDE )
					cout << "TIFFReader: PDE values will be normalized to 1.0 (divided by 255)." << endl;
				break;
			}
			case 16:
			{
				cout << "TIFF image is 16 bit. Using scanline method\n"; 
				raster16 = (uint16*) _TIFFmalloc( TIFFScanlineSize(tif) );
				if( mode == TIFFReader::PDE )
					cout << "TIFFReader: PDE values will be normalized to 1.0 (divided by 65535)." << endl;
				break;
			}
			case 32:
			{
				cout << "TIFF image is 32 bit. Using scanline method\n"; 
 				if( mode == TIFFReader::CELLS ){
 					cerr << "TIFFReader: Cannot initialize cells using 32-bit (float) format TIFF. Use 8 or 16 bit format with unsigned integers instead." << endl;
 					//exit(-1);
 				}
				if( mode == TIFFReader::PDE )
					cout << "TIFFReader: PDE values read from 32 bit file." << endl;
				raster32 = (float*) _TIFFmalloc( TIFFScanlineSize(tif) );
				break;
			}
			default:
			{
				cerr << "TIFF image is "<< numbits <<" bit, which is not supported! Only 8 (0-255), 16 (0-65535), and 32 (floating-point) bit format are supported." << endl; 
				exit(-1);
			}
		}
		
		if (raster8 == NULL && raster16 == NULL && raster32 == NULL) {
 			return false;
 		}
 		
 		VINT pos(0,0,0);
		do {
			
			if ( (pos.z+1) > SIM::getLattice()->size().z ) {
				cerr << "TIFFReader: Image depth (z) too large for the lattice! ("<<(pos.z+1)<<" > "<<SIM::getLattice()->size().z<<")" << endl;
				exit(-1);
			}
			uint32 w1, h1;
			TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w1);
			TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h1);
			if (w1!=w || h1!=h){
				cerr << "TIFFReader: Require identical sizes for images in a stack!" << endl;
				exit(-1);
			}

			switch( numbits ){
				
				case 8:
				{
					
					if (TIFFReadRGBAImage(tif, w, h, raster8, 0)) {
						pos.y=h-1;
						
						for (uint i=0; i<npixels; i++, pos.x++) {
							
							if ( pos.x >= w ){
								pos.x=0; 
								pos.y--; 
							}
							
							if( numsamples == 1){
								uint32 grey = raster8[i] & 0x00FFFFFF; // Remove alpha channel
								
								if ( ! (raster8[i] & 0xFF000000) ) 
									continue; // skip fully transparent
								
								switch( mode ){
									case CELLS:
									{
										addNode( offset_+pos, (uint32)grey );
										break;
									}
									case PDE:
									{
										//NOTE: normalized to 1.0
										pde_layer->set( offset_+pos, ((double)grey/255.0)*scaleToMax() );
										break;
									}
								}
							}
							else if(numsamples == 3 ){
								cerr << "TIFFReader: 8-bit RGB format not supported. Please convert the input image to 8-bit greyscale or 16-bit (RGB or greyscale).";
								exit(-1);
							}
						}
					}
					break;
				}
				case 16:
				{
					for (uint32 row=0; row < h; row++) {
						if( TIFFReadScanline( tif, raster16, row) < 0){
							cerr << "TIFFReader: Error during reading TIFF image '" << filename() << "'." << endl;
							exit(-1);
						}
						pos.y = row;
						pos.x = 0; // TODO: Check whether scanline is necessarily equal to a x-row (if not, remove this line)
						for(uint32 col=0; col < w; col++, pos.x++){
							
							if ( pos.x >= w ){
								pos.x=0; 
								pos.y--; 
							}
							
							if( numsamples == 1 ){
								uint16 gray = static_cast<uint16*>(raster16)[col*numsamples+0];
								//cout << gray << "\n";
								
								switch( mode ){
									case CELLS:
									{
										//cout << pos << endl;
										addNode( offset_+pos, (uint32)gray );
										break;
									}
									case PDE:
									{
										pde_layer->set( offset_+pos, double( (double)gray/(65535.0) )*scaleToMax() );
										break;
									}
								}

							}
							else if(numsamples == 3 ){
								uint16 red		= static_cast<uint16*>(raster16)[col*numsamples+0];
								uint16 green	= static_cast<uint16*>(raster16)[col*numsamples+1];
								uint16 blue		= static_cast<uint16*>(raster16)[col*numsamples+2];
// 								cout << red << " + " << green << " + " << blue << " = " << (uint16)(red+green+blue) << "\n";
								
								switch( mode ){
									case CELLS:
									{
										addNode( offset_+pos, (uint16)(red+green+blue) );
										break;
									}
									case PDE:
									{
										pde_layer->set( offset_+pos, double( (double)(red+green+blue)/(65535.0) )*scaleToMax() );
										break;
									}
								}
							}
						}
					}
					break;
				}
				case 32:
				{
					for (uint32 row=0; row < h; row++) {
						if( TIFFReadScanline( tif, raster32, row ) < 0 ){
							cerr << "TIFFReader: Error during reading TIFF image '" << filename() << "'." << endl;
							exit(-1);
						}
						pos.y = row;
						pos.x = 0; // TODO: Check whether scanline is necessarily equal to a x-row (if not, remove this line)

						for(uint32 col=0; col < w; col++, pos.x++){
							
							if ( pos.x >= w ){
								pos.x=0; 
								pos.y--; 
							}
							
							if( numsamples == 1 ){
								
								switch( mode ){
									case CELLS:
									{
										uint32 value = uint32(raster32[col]);
										// Cells need to be initialized with an Integer
										if( std::floor( value ) != raster32[col] ){
											cerr << "TIFFReader: Cannot initialize cells with non-integer value: " << raster32[col] << " @ " << row << ", " << col << endl;
											exit(-1);
										}
										addNode( offset_+pos, (uint32)(value) );
										break;
									}
									case PDE:
									{
										float value = raster32[col];
										if( scaleToMax.isDefined() )
											value = (double)value*scaleToMax();
										pde_layer->set( offset_+pos, (double)value );
										break;
									}
								}
							}
						}
					}
					break;
				}
			}
			pos.z++;
		} while (TIFFReadDirectory(tif));
		
		if( numbits == 8 )
			_TIFFfree(raster8);
		else if( numbits == 16 )
			_TIFFfree(raster16);
		TIFFClose(tif);
	}
	else{
		cerr << "TIFFReader: File '" << filename() << "' cannot be opened." << endl;
		exit(-1);
	}
	if( mode == TIFFReader::CELLS ){
		cout << "TIFFReader: Initialized " << created_cells << " cells occupying " << created_nodes << " nodes. Note: " << skipped_nodes << " nodes were skipped!" << endl;
	
		vector<CPM::CELL_ID> cells = celltype->getCellIDs();
		for (uint i=0; i < cells.size(); i++) {
			const Cell& cell = celltype->getCell( cells[i] );
			if( cell.getNodes().size() == 0 ){
				cout << "Warning: Removing cell " << cells[i] << " with volume " << cell.getNodes().size() << "\n";
				celltype->removeCell( cells[i] );
			}
		}
	}
	return true;
}

void TIFFReader::addNode(VINT pos, uint32 color){
	
	if ( !color )
		return; // skip black
		
	map<uint32, uint>::iterator it;
	it = color_to_cellid.find(color);
	
	// no cell with this color found
	if ( it == color_to_cellid.end() ) { 
		// create new cell
		CPM::CELL_ID new_cell_id;
		if( keepIDs() ){
			cout << "TIFFReader: KeepID: " << (CPM::CELL_ID)color << endl;
			new_cell_id = celltype->createCell( (CPM::CELL_ID)color );
		}
		else
			new_cell_id = celltype->createCell();
		color_to_cellid[color] = new_cell_id;
		//cout << "Created new cell " << new_cell_id << "\n";
	}

	//a cell with this color was already created
	if( CPM::getNode(pos).cell_id != empty_state ) {
		skipped_nodes++;
		cout << "Cannot initialize cell at pos " << pos << ", node is occupied !\n";
	}
	else {
		it = color_to_cellid.find(color);
		
		// correct for periodic boundary conditions
		VINT current_center = CPM::getCell( it->second ).getCenterL();
		VINT pos_c = current_center - SIM::getLattice()->node_distance( current_center,  SIM::getLattice()->to_orth(pos));
		
		CPM::setNode(pos_c, it->second );
		//cout << "Added node ("<<pos<<") to existing cell " << it->second << "\n";
		created_nodes++;
	}
	created_cells = color_to_cellid.size();
}

