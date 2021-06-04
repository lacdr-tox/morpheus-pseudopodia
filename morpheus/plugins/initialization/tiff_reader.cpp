#include "tiff_reader.h"

REGISTER_PLUGIN(TIFFReader);

TIFFReader::TIFFReader(){
	filename.setXMLPath("filename");
	registerPluginParameter( filename );

	offset.setXMLPath("offset");
	registerPluginParameter( offset );
	
	keepIDs.setXMLPath("keepIDs");
	keepIDs.setDefault("false");
	registerPluginParameter( keepIDs );
	
	scaling.setXMLPath("scaleToMax");
	scaling.setDefault("1.0");
	registerPluginParameter( scaling );
	
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

vector<CPM::CELL_ID>  TIFFReader::run(CellType* ct)
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
	
	return cells_created;
}

bool TIFFReader::loadTIFF(){
	
	empty_state = CPM::getEmptyState().cell_id;
	

	TIFFSetWarningHandler(0);	
	TIFF* tif = TIFFOpen(filename().c_str(), "r");
	
	if (tif) {
		uint32 w, h;
		unsigned short int numbits, numsamples;
		size_t npixels;
		uint8* raster8 = nullptr;
		uint16* raster16 = nullptr;
		float* raster32 = nullptr;
		double* raster64 = nullptr;
		
		TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
		TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
		TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &numbits);
		TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &numsamples);

		cout << "TIFF Reader, Image size = " << w << " , " << h << "; " << numsamples << " samples per pixel, " << numbits << " bits per sample. \n";
		
		if (numsamples != 1) {
			cerr << "TIFFReader: Only grayscale images are supported";
			exit(-1);
			// TODO offer channel/sample id selection in xml
		}
		
		VINT offset_ = offset.isDefined() ? offset() : VINT(0,0,0);
		// check whether it fits within lattice
		if ( offset_.x + w > SIM::getLattice()->size().x ) {
			cerr << "TIFFReader: Image width (x) too large for the lattice! ("<< w <<" > "<<SIM::getLattice()->size().x<<")";
			exit(-1);
		}
		if ( offset_.y + h > SIM::getLattice()->size().y ) {
			cerr << "TIFFReader: Image height (y) too large for the lattice! ("<< h <<" > "<<SIM::getLattice()->size().y<<")";
			exit(-1);
		}
		
		if (offset.isMissing()) {
			// centering 
			offset_.x = (SIM::getLattice()->size().x - w) / 2;
			offset_.y = (SIM::getLattice()->size().y - h) / 2;
			offset_.z = 0;
		}
		
		if( offset_.abs() > 0 ){
			cout << "TIFFReader::loadTIFF: Offset = " << offset_ << endl;
		}
		
		
		// allocate memory for buffer
		npixels = w * h;
		switch (numbits){
			
			case 8:
			{
				cout << "TIFFReader: TIFF image is 8 bit.\n";
				raster8 = (uint8*) _TIFFmalloc(TIFFScanlineSize(tif));
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
			case 64:
			{
				cout << "TIFF image is 64 bit. Using scanline method\n"; 
 				if( mode == TIFFReader::CELLS ){
 					cerr << "TIFFReader: Cannot initialize cells using 64-bit (double) format TIFF. Use 8 or 16 bit format with unsigned integers instead." << endl;
 					//exit(-1);
 				}
				if( mode == TIFFReader::PDE )
					cout << "TIFFReader: PDE values read from 64 bit file." << endl;
				raster64 = (double*) _TIFFmalloc( TIFFScanlineSize(tif) );
				break;
			}
			default:
			{
				cerr << "TIFF image is "<< numbits <<" bit, which is not supported! Only 8 (0-255), 16 (0-65535), 32 (single precision) and 64 (double precision) bit format are supported." << endl; 
				exit(-1);
			}
		}
		
		if ( ! (raster8 || raster16 || raster32 || raster64) ) {
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
				cerr << "TIFFReader: Mismatching images sizes in tiff stack ("<<w<<"?="<<w1<<";"<<h<<"?="<<h1<<")!" << endl;
				exit(-1);
			}

			switch( numbits ){
				
				case 8:
				{
					for (uint32 row=0; row < h; row++) {
						if( TIFFReadScanline( tif, raster8, row) < 0){
							cerr << "TIFFReader: Error during reading TIFF image '" << filename() << "'." << endl;
							exit(-1);
						}
						pos.y = row;
						pos.x = 0; // TODO: Check whether scanline is necessarily equal to a x-row (if not, remove this line)
						if (mode == CELLS) {
							for(uint32 col=0; col < w; col++, pos.x++){
								addNode( offset_+pos, (uint32) raster8[col]);
							}
						}
						else if ( mode==PDE ) {
							for(uint32 col=0; col < w; col++, pos.x++){
								pde_layer->set( offset_+pos, ((double)raster8[col]/255.0)*scaling() );
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
										pde_layer->set( offset_+pos, double( (double)gray/(65535.0) )*scaling() );
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
										double value = raster32[col] * scaling();
										pde_layer->set( offset_+pos, value );
										break;
									}
								}
							}
						}
					}
					break;
				}
				case 64:
				{
					for (uint32 row=0; row < h; row++) {
						if( TIFFReadScanline( tif, raster64, row ) < 0 ){
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
 										uint64_t value = uint64_t(raster64[col]);
										// Cells need to be initialized with an Integer
										if( std::floor( value ) != raster64[col] ){
											cerr << "TIFFReader: Cannot initialize cells with non-integer value: " << raster64[col] << " @ " << row << ", " << col << endl;
											exit(-1);
										}
										addNode( offset_+pos, (uint64_t)(value) );
										break;
									}
									case PDE:
									{
										double value = raster64[col];
										if( scaling.isDefined() )
											value = (double)value*scaling();
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
		
		if (raster8 ) _TIFFfree(raster8 );
		if (raster16) _TIFFfree(raster16);
		if (raster32) _TIFFfree(raster32);
		if (raster64) _TIFFfree(raster64);
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
		cells_created.push_back(new_cell_id);
		//cout << "Created new cell " << new_cell_id << "\n";
	}

	//a cell with this color was already created
	if( CPM::getNode(pos).cell_id != empty_state ) {
		skipped_nodes++;
		cout << "Cannot initialize cell at pos " << pos << ", node is occupied !\n";
	}
	else {
		it = color_to_cellid.find(color);
		VINT closest_pos = pos;
		if (CPM::getCell( it->second ).nNodes()>0) {
			// correct for periodic boundary conditions
			VINT current_center (CPM::getCell( it->second ).getCenterL());
			closest_pos = current_center - SIM::getLattice()->node_distance( current_center,  pos);
		}
		
		CPM::setNode(closest_pos, it->second );
// 		cout << "Added node ("<<pos<<") with pos_c "<< pos_c <<"to existing cell " << it->second << "\n";
		created_nodes++;
	}
	created_cells = color_to_cellid.size();
}

