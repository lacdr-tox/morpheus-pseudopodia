#include "tiff_plotter.h"

int TiffPlotter::instances=0;
using namespace SIM;

//-------------------------------------------------------------------

REGISTER_PLUGIN(TiffPlotter);

TiffPlotter::TiffPlotter(){ 
	TiffPlotter::instances++;
	instance_id = TiffPlotter::instances;
	im_desc.reserve(10);
};

TiffPlotter::~TiffPlotter(){
	TiffPlotter::instances--;
};

//-------------------------------------------------------------------


void TiffPlotter::loadFromXML(const XMLNode xNode)	//einlesen der Daten aus der XML
{
	stored_node = xNode;
	
	ome_header.setXMLPath("OME-header");
	ome_header.setDefault("true");
	registerPluginParameter(ome_header);

	timelapse.setXMLPath("timelapse");
	timelapse.setDefault("true");
	registerPluginParameter(timelapse);

	compression.setXMLPath("compression");
	compression.setDefault("true");
	registerPluginParameter(compression);

	map<string, uint> format_map;
	format_map["8bit"] = 8;
	format_map["16bit"] = 16;
	format_map["32bit"] = 32;
	format_map["guess"] = 0;
	
	bitformat.setXMLPath("format");
// 	bitformat.setDefault("guess");
	bitformat.setConversionMap(format_map);
	registerPluginParameter(bitformat);
	
	
	// Define PluginParameters for all defined Output tags
	for (uint i=0; i<xNode.nChildNode("Channel"); i++) {
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
		
		plot.channels.push_back(c);
	}

	plot.cropToCell = false;
	if( xNode.nChildNode("CropToCell") ){
		plot.cropToCell = true;
		
		plot.cellids_str.setXMLPath("CropToCell/cell-ids");
		registerPluginParameter( plot.cellids_str );
		
		plot.padding.setXMLPath("CropToCell/padding");
		plot.padding.setDefault("0");
		registerPluginParameter( plot.padding ); 
	}
	

	if( plot.channels.size() > 1 )
		multichannel = true;
	else
		multichannel = false;

	AnalysisPlugin::loadFromXML(xNode);

};

vector<CPM::CELL_ID> TiffPlotter::parseCellIDs(string cell_ids_string){

	vector<CPM::CELL_ID> ids;
	vector<string> tokens;
	tokens = tokenize(cell_ids_string, ",");
	for(uint i=0; i < tokens.size(); i++){
		cout << tokens[i] << "\n";
		vector<string> cell_ids2 = tokenize(tokens[i], "-");
		if(cell_ids2.size() == 2){
			uint s = atoi(cell_ids2[0].c_str());
			uint e = atoi(cell_ids2[1].c_str());
			for(uint j = s; j <= e; j++){
				cout << j << "\n";
				ids.push_back( j );
			}
		}
		else
			ids.push_back( atoi(tokens[i].c_str()) );
	}
	if ( ids.empty() ){
		throw string("No cell IDs were extracted from provided string: "+cell_ids_string);
	}
	return ids;	
}

void TiffPlotter::init(const Scope* scope)
{
	for(uint c=0; c<plot.channels.size(); c++){
		plot.channels[c]->celltype.init();
		if( plot.channels[c]->celltype.isDefined() ){
			plot.channels[c]->symbol.setScope(plot.channels[c]->celltype()->getScope());
		}
	}
	
	AnalysisPlugin::init( scope );
	
	if(plot.cropToCell){
		file_boundingbox.open("tiffplotter_boundingbox.txt", ios::out);
		plot.cellids = parseCellIDs( plot.cellids_str.get() );
		cout << "TIFFPlotter: CellIDs: ";
		for(uint i=0; i < plot.cellids.size(); i++)
			cout << plot.cellids[i] << ", ";
		cout << "\n";
	}

	if( ome_header() && compression() && timelapse() )
		throw MorpheusException("TIFFPlotter: Cannot write OME header with compressed timelapse image.\n \
(Reason: libTiff does not provide random access to compressed image data. \
Therefore, cannot write OME header to first image.)", stored_node);

// 	// find symbols for all channels
// 	// 	cout << "TIFFPlotter: channels: \n";
 	for(uint c=0; c<plot.channels.size(); c++){

//		plot.channels[c]->symbol = SIM::findGlobalSymbol<double>( plot.channels[c]->symbolstr, 0.0 );
	
		if(plot.channels[c]->symbol.accessor().getLinkType() == SymbolData::PDELink )
			plot.channels[c]->pde_layer = SIM::findPDELayer( plot.channels[c]->symbol.name() );
		
		if( plot.channels[c]->symbol.accessor().getLinkType() == SymbolData::CellMembraneLink ){
			
			// look for membrane property
			if( plot.channels[c]->celltype.isDefined() ){
				plot.channels[c]->membrane = plot.channels[c]->celltype.get()->findMembrane( plot.channels[c]->symbol.name() );
				
				if(!plot.channels[c]->membrane.valid()) {
					cerr << "TiffPlotter: MembraneProperty \"" << plot.channels[c]->membrane.getFullName() << "\" not valid!" << endl;
					exit(-1);
				}
			}
			//plot.channels[c]->outline = true;
		}
		
	}
		
	// if not specified, try to guess the Format (as small as possible, but uniform within multichannel stack)
	if( bitformat.isDefined() && bitformat.get() > 0){
		format = bitformat.get();
	}
 	else {
		uint format_temp;		
		for(uint c=0; c<plot.channels.size();c++){
			
			switch(  plot.channels[c]->symbol.accessor().getLinkType() ){
				
				case SymbolData::CellTypeLink: // assuming no more than 255 celltypes
					format_temp = 8;
					break;
				case SymbolData::CellIDLink: // assuming no more than 65526 cells
				case SymbolData::CellVolumeLink: // assuming cells are not larger than 65526 nodes
					format_temp = 16;
					break;
				case SymbolData::CellPropertyLink:
				case SymbolData::CellMembraneLink:
				case SymbolData::GlobalLink:
				case SymbolData::SingleCellPropertyLink:
				case SymbolData::SingleCellMembraneLink:
				case SymbolData::CellLengthLink:
					format_temp = 32;
					break;
				default:
					format_temp = 32;
			} 
			if(format_temp > format )
				format = format_temp;
		}
		cout << "TiffPlotter: File format of " <<  format << " bit.\n"; 
	}
	
	if( !CPM::getCellTypes().empty() )
		cpm_layer = CPM::getLayer();
	latticeDim = SIM::getLattice()->size();
	
	ImageDescription imdesc;
	imdesc.images = 0;
	imdesc.frames = 1;
	imdesc.max = 0.0;
	imdesc.min = 0.0;
	im_desc[instance_id] = imdesc;
};
	
	//-------------------------------------------------------------------
	
void TiffPlotter::analyse(double time)
{
	
	if(plot.cropToCell){
		for(uint i=0; i < plot.cellids.size(); i++)
			writeTIFF( plot.cellids[i] );
	}
	else
		writeTIFF();
};

void TiffPlotter::finish(double time){
	if(file_boundingbox.is_open())
		file_boundingbox.close();
};
	
string TiffPlotter::getFileName(CPM::CELL_ID cellid){
	
	stringstream ts;
	if(!timelapse.get()){
//		if(timename)
			ts << "_" << SIM::getTimeName();
//		else
//			ts << "_" << setfill('0') << setw(6) <<  int(rint( SIM::getTime() / SIM::getInterval()));
	}
	
	stringstream fn;
	string propname0 = plot.channels[0]->symbol.name(); // getFullName()
	string propname1 = remove_spaces( propname0 );
	fn << propname1;
	for(uint c=1; c<plot.channels.size();c++){
		propname0 = plot.channels[c]->symbol.name(); // getFullName()
		propname1 = remove_spaces( propname0 );
		fn << "_" << propname1 ;
	}
	
	if( plot.cropToCell ){
		fn << "_cell_"<<  setw(4) << setfill('0') << cellid ;
	}
	
	fn << ts.str() << ".tif";
	return fn.str();
} 
	
bool TiffPlotter::fileExists(string filename){
	ifstream ifile(filename.c_str());
	return ifile.is_open();
}

void TiffPlotter::writeTIFF(CPM::CELL_ID cellid)
{
	TIFF *output;
	uint32 width, height, slices;
	bool append=false;
	
	// Open the output image
	string filename;
	if(plot.cropToCell && cellid ){
		if( !CPM::cellExists(cellid) ){
			cout << "TIFFPlotter: Cell " << cellid << "does not exist (anymore), skipping.." << endl;
			return;
		}
		filename = getFileName(cellid);
	}
	else
		filename = getFileName();
	
	if( timelapse.get() && fileExists(filename) ){
		append=true;
		if((output = TIFFOpen(filename.c_str(), "a")) == NULL){
			cerr << "Could not open image " << filename << " to append." <<endl;
			exit(-1);
		}
	}
	else{
		append=false;
		if((output = TIFFOpen(filename.c_str(), "w")) == NULL){
			cerr << "Could not open image " << filename << " to write." << endl;
			exit(-1);
		}
	}
	
	// concatenate a list of all celltype IDs
	// TODO: Only make this list if required
	vector< CPM::CELL_ID > cell_ids_all;
	if(plot.cropToCell)
		cell_ids_all = plot.cellids;
	else{
		auto celltypes = CPM::getCellTypes();
		for(uint ct=0; ct<celltypes.size(); ct++){
			vector< CPM::CELL_ID > cell_ids_ct= celltypes[ct].lock()->getCellIDs();
			cell_ids_all.insert( cell_ids_all.end(), cell_ids_ct.begin(), cell_ids_ct.end() );
		}
	}
	
	// Set min and max values and get them from PDE/CPM if necessary
	for(uint c=0; c<plot.channels.size();c++){ 

		if( plot.channels[c]->min_user.isDefined() ){
			plot.channels[c]->min = plot.channels[c]->min_user.get();
		}
		else{ // if minimum not specified by user
			if(plot.channels[c]->symbol.accessor().getLinkType() == SymbolData::PDELink){
				plot.channels[c]->min = plot.channels[c]->pde_layer->min_val();
			}
			else{
				vector< CPM::CELL_ID > cell_ids;
				if(plot.channels[c]->celltype.isDefined()){
					cell_ids = plot.channels[c]->celltype.get()->getCellIDs();
				}
				else{
					cell_ids = cell_ids_all;
				}
				
				for(uint j=0; j < cell_ids.size(); j++){
					
					if( plot.channels[c]->symbol.accessor().getLinkType() == SymbolData::CellMembraneLink ){
						if (plot.channels[c]->membrane.valid()) {
							auto min_val = plot.channels[c]->membrane.getMembrane( cell_ids[j] )->min_val();
							if (min_val < plot.channels[c]->min)
							plot.channels[c]->min = plot.channels[c]->membrane.getMembrane( cell_ids[j] )->min_val();
						}
					}
					else{
						double value = plot.channels[c]->symbol.get( cell_ids[j] );
						if( value < plot.channels[c]->min ){
							plot.channels[c]->min = value;
						}
					}
				}
			}
		}
		if( plot.channels[c]->max_user.isDefined() ){
			plot.channels[c]->max = plot.channels[c]->max_user.get();
		}
		else{ // if minimum not specified by user
			if(plot.channels[c]->symbol.accessor().getLinkType() == SymbolData::PDELink){
				plot.channels[c]->max = plot.channels[c]->pde_layer->max_val();
			}
			else{
				vector< CPM::CELL_ID > cell_ids;
				if(plot.channels[c]->celltype.isDefined()){
					cell_ids = plot.channels[c]->celltype.get()->getCellIDs();
				}
				else{
					cell_ids = cell_ids_all;
				}
				
				for(uint j=0; j < cell_ids.size(); j++){
					
					if( plot.channels[c]->symbol.accessor().getLinkType() == SymbolData::CellMembraneLink ){
						if(plot.channels[c]->membrane.valid()) {
							plot.channels[c]->max = plot.channels[c]->membrane.getMembrane( cell_ids[j] )->max_val();
						}
					}
					else{
						double value = plot.channels[c]->symbol.get( cell_ids[j] );
						if( value > plot.channels[c]->max ){
							plot.channels[c]->max = value;
						}
					}
				}
			}
		}		
		// set default min/max to 0 and 1
		//  and treat celltype IDs as special case
		if( plot.channels[c]->min == plot.channels[c]->max 
			|| plot.channels[c]->symbol.accessor().getLinkType() == SymbolData::CellTypeLink ){
			plot.channels[c]->min = 0.0;
			if( plot.channels[c]->max == 0.0)
				plot.channels[c]->max = 1.0;
		}
		
		// update global information (used in TIFFTAG_IMAGE_DESCRIPTION)
		if( plot.channels[c]->min < im_desc[ instance_id ].min )
			im_desc[ instance_id ].min = plot.channels[c]->min;
		if( plot.channels[c]->max > im_desc[ instance_id ].max )
			im_desc[ instance_id ].max = plot.channels[c]->max;
	}
	
	// determine bounding box in case of CropToCell
	VINT pmin(0,0,0), pmax(latticeDim.x, latticeDim.y, latticeDim.z);
	if(plot.cropToCell){
		if(cellid > 0){
			const Cell& cell = CPM::getCell( cellid );
			VINT pmin_temp = pmin;
			pmin = pmax; pmax = pmin_temp;
			// calculate bounding box
			const Cell::Nodes nodes = cell.getNodes();
			for (Cell::Nodes::const_iterator pt = nodes.begin(); pt != nodes.end(); pt++){
				pmin.x = std::min(pt->x, pmin.x);
				pmin.y = std::min(pt->y, pmin.y);
				pmin.z = std::min(pt->z, pmin.z);
				pmax.x = std::max(pt->x, pmax.x);
				pmax.y = std::max(pt->y, pmax.y);
				pmax.z = std::max(pt->z, pmax.z);
			}
		}
		pmin -= VINT(1,1,1);
		pmax += VINT(1,1,1);
		pmin -= VINT(plot.padding.get(), plot.padding.get(), plot.padding.get());
		pmax += VINT(plot.padding.get(), plot.padding.get(), plot.padding.get());
		
		// Note: not cutting off edges, because this disrupts 3D plots of boundary cells
// 		pmin.x = std::max(pmin.x, 0);
// 		pmin.y = std::max(pmin.y, 0);
// 		pmin.z = std::max(pmin.z, 0);
// 		pmax.x = std::min(pmax.x, latticeDim.x-1);
// 		pmax.y = std::min(pmax.y, latticeDim.y-1);
// 		pmax.z = std::min(pmax.z, latticeDim.z-1);
		
		// write bounding box per cell to file (useful to combine exported cells with poshoc analysed stacks)
		
		file_boundingbox << cellid << "\t" << pmin << "\t" << pmax << "\t"<< (pmax-pmin) <<"\n";
		
	}
	
	// We need to know the width and the height before make buffer
	width  = pmax.x - pmin.x;
	height = pmax.y - pmin.y;
	slices = pmax.z - pmin.z;

	if( width == 0 || height == 0 )
		return;
	//cout << "width\theight\tslices\tpmin\tpmax\n";
	//cout << width<<"\t"<<height<<"\t"<<slices<<"\t"<<pmin<<"\t"<<pmax<<"\n";
	
	// Allocate buffer according to file format
	char   buffer8 [width * height];
	uint16 buffer16[width * height];
	uint32 buffer32[width * height];
	uint16 bpp, spp;
	bpp = format; // bits per pixel
	spp = 1; // samples per pixel (1 = greyscale, 3 = RGB)
	
	VINT pos(0,0,0);
	int EmptyCellTypeID = CPM::getEmptyCelltypeID();
	for (pos.z=pmin.z; pos.z<pmax.z; pos.z++)
	{
		for (uint c=0; c<plot.channels.size(); c++)
		{
			for (pos.y=pmin.y; pos.y<pmax.y; pos.y++)
			{
				for (pos.x=pmin.x; pos.x<pmax.x; pos.x++)
				{
					// update position in buffer
					double value = 0.0;
					
					if(plot.channels[c]->symbol.accessor().getLinkType() == SymbolData::PDELink){
						value = plot.channels[c]->symbol.get(pos);
					}
					else if (cpm_layer) { // cell property or membrane
						uint celltype_at_pos = CPM::getCellIndex( cpm_layer->get(pos).cell_id ).celltype;
						
						// if cell type is specified, only plot property of that cell type, and assume 0 for other cell types
						if( plot.channels[c]->celltype.isDefined() ){
						
							if( plot.channels[c]->exclude_medium.get() 
								&& celltype_at_pos == EmptyCellTypeID ){
								value = 0;
							}
							// for cropToCell plots, only plot cell ID/type of specified cell, and not of surrounding cells. 
							//cout << "plot.cropToCell: " << (plot.cropToCell?"true":"false") << "\ncelltype_at_pos: " << celltype_at_pos <<"\nplot.channels[c]->symbol.getLinkType(): " << plot.channels[c]->symbol.getLinkType() << endl;  
							else if( plot.cropToCell 
								&& cpm_layer->get(pos).cell_id != cellid  ){
								value = 0;
							}
							// if pos is part of cell of chosen cell type, plot the value
							else if( celltype_at_pos == plot.channels[c]->celltype.get()->getID() ){

								if( plot.channels[c]->outline.get() ){
									if ( CPM::isSurface( pos ) ){
										value = (double)plot.channels[c]->symbol.get( pos );
									}
								}
								else{ // if not outline, plot property over whole cell
									value = (double)plot.channels[c]->symbol.get( pos );
									
									// when cropToCell, do not plot boundary (to avoid visualization problems: overlaying multiple transparent layers)
									if( plot.cropToCell && CPM::isSurface( pos ) )
										value = 0;
								}
							}
							else{
								value = 0;
							}
							
						}
						// otherwise, plot property of all cell types
						else{
							if( ( plot.channels[c]->exclude_medium.get() && celltype_at_pos == EmptyCellTypeID ) ){
								value = 0;
							}
							else{
								if( plot.channels[c]->outline.get() ){
									if ( CPM::isSurface( pos ) ){
										value = (double)plot.channels[c]->symbol.get( pos );
									}
								}
								else{

									value = (double)plot.channels[c]->symbol.get( pos );
/*									
									// when cropToCell, do not plot boundary (to avoid visualization problems: overlaying multiple transparent layers)
									if( plot.cropToCell && !CPM::isBoundary( pos ) )
										value = 0;*/
								}
							}
						}
					}
					
					double mmin = plot.channels[c]->min;
					double mmax = plot.channels[c]->max;
					uint bufindex = (pos.x-pmin.x) + (pos.y-pmin.y)*width;
					switch(format){
						case 8:
						{
							if(plot.channels[c]->scale.get()){
								//cout << "min: " << mmin << ", max: " << mmax << ", value: "<< value << ", normalized: " << ((((value-mmin) / (mmax-mmin)) * 255.0)) << endl;
								(int8&)buffer8[bufindex] = (int8)(((value-mmin) / (mmax-mmin)) * 255.0);
							}
							else
								(int8&)buffer8[bufindex] = (int8)value;
							break;
						}
						case 16:
						{
							if(plot.channels[c]->scale.get())
								(int16&)buffer16[bufindex] = (int16)(((value-mmin) / (mmax-mmin)) * 65536.0);
							else
								(int16&)buffer16[bufindex] = (int16)value;
							break;
						}
						case 32:
						{
							if(plot.channels[c]->scale.get())
								(float&)buffer32[bufindex] = (float)((value-mmin) / (mmax-mmin));
							else
								(float&)buffer32[bufindex] = (float)value;
							break;
						}
					}
				}
			}
			
			
			// Set TIFF info fields
			TIFFSetField(output, TIFFTAG_IMAGEWIDTH,  width); //x
			TIFFSetField(output, TIFFTAG_IMAGELENGTH, height); //y
			TIFFSetField(output, TIFFTAG_BITSPERSAMPLE, bpp);
			TIFFSetField(output, TIFFTAG_ROWSPERSTRIP, height); //bpp); 
			TIFFSetField(output, TIFFTAG_SAMPLESPERPIXEL, spp);
			uint sampleformat;
			switch(format){
				case 8: { sampleformat = 1; break; } // unsigned integer
				case 16:{ sampleformat = 2; break; } // signed integer
				case 32:{ sampleformat = 3; break; } // IEEE floating point
			}
			TIFFSetField(output, TIFFTAG_SAMPLEFORMAT, sampleformat);
			TIFFSetField(output, TIFFTAG_SOFTWARE,  "morpheus");
			TIFFSetField(output, TIFFTAG_PLANARCONFIG,  PLANARCONFIG_CONTIG);
			TIFFSetField(output, TIFFTAG_PHOTOMETRIC,   PHOTOMETRIC_MINISBLACK);
			//TIFFSetField(output, TIFFTAG_XRESOLUTION, 1);
			//TIFFSetField(output, TIFFTAG_YRESOLUTION, 1);	
			if( compression.get() ){
				TIFFSetField(output, TIFFTAG_COMPRESSION,   COMPRESSION_LZW);
			}
			else{
				TIFFSetField(output, TIFFTAG_COMPRESSION,   COMPRESSION_NONE);
			}
			
			// Multipage TIFF
			unsigned short page, numpages_before=0, numpages_after=0;
			if( multichannel || timelapse.get() || slices>1 ){
				TIFFSetField(output, TIFFTAG_SUBFILETYPE, 0);
			}
			
			// Now, write the buffer to TIFF
			for (uint i=0; i<height; i++){
				switch(format){
					case 8:
					{
						if(TIFFWriteScanline(output, &buffer8[i * width], i, 0) < 0){
							cerr << "Could not write TIFF image " << endl;
							exit(-1);
						}
						break;
					}
					case 16:
					{
						if(TIFFWriteScanline(output, &buffer16[i * width], i, 0) < 0){
							cerr << "Could not write TIFF image " << endl;
							exit(-1);
						}
						break;
					}
					case 32:
					{
						if(TIFFWriteScanline(output, &buffer32[i * width], i, 0) < 0){
							cerr << "Could not write TIFF image " << endl;
							exit(-1);
						}
						break;
					}
				}
			}
			
			if( multichannel || timelapse.get() || slices>1 ){
				TIFFWriteDirectory(output);
			}
		}
	}
	
	if( ome_header() ){ 
		// Generate OME-TIFF header.
		// Specs: https://www.openmicroscopy.org/site/support/ome-model/ome-tiff/specification.html
		// OME XSD Schema: http://www.openmicroscopy.org/Schemas/OME/2015-01/ome.xsd
		
		XMLNode omeXML = XMLNode::createXMLTopNode("OME");
		omeXML.addAttribute("xmlns:xsi","http://www.w3.org/2001/XMLSchema-instance");
		omeXML.addAttribute("xsi:schemaLocation","http://www.openmicroscopy.org/Schemas/OME/2015-01/ome.xsd");
		
		//omeXML.addChild( "Experimenter" );
		
		XMLNode omeImageXML = omeXML.addChild( "Image" );
		omeImageXML.addChild("Description").addText("Created by Morpheus TIFF Plotter");
		XMLNode omePixelsXML = omeImageXML.addChild("Pixels");
		
		string format_str;
		switch( format ){
			case 8:  format_str = "Int8"; break;
			case 16: format_str = "Int16"; break;
			case 32: format_str = "Float"; break;
		}
		if(timelapse() && append){
			im_desc[ instance_id ].frames += 1;
		}
		else{
			im_desc[ instance_id ].frames = 1;
		}

		omePixelsXML.addAttribute("PixelType", format_str.c_str());
		omePixelsXML.addAttribute("DimensionOrder", "XYCZT");	
		// image size
		omePixelsXML.addAttribute("SizeX", to_cstr(width));
		omePixelsXML.addAttribute("SizeY", to_cstr(height));
		omePixelsXML.addAttribute("SizeZ", to_cstr(slices));
		// number of channels
		omePixelsXML.addAttribute("SizeC", to_cstr(plot.channels.size()));
		// number of frame (time points), time increment will be interpreted as seconds
		omePixelsXML.addAttribute("SizeT", to_cstr(im_desc[ instance_id ].frames));
		omePixelsXML.addAttribute("TimeIncrement", to_cstr(this->timeStep()));
		// physical size (in microns)
		omePixelsXML.addAttribute("PhysicalSizeX", to_cstr(SIM::getNodeLength()));
		omePixelsXML.addAttribute("PhysicalSizeY", to_cstr(SIM::getNodeLength()));
		omePixelsXML.addAttribute("PhysicalSizeZ", to_cstr(SIM::getNodeLength()));

		omePixelsXML.addChild("TiffData");
		
		for(uint c=0; c<plot.channels.size(); c++){
			XMLNode omeChannelXML = omePixelsXML.addChild("Channel");
			stringstream ss; ss << "Channel:0:" << c;
			// channel id number
			omeChannelXML.addAttribute("ID", ss.str().c_str());
			// channel name (symbol name)
			string channel_name = plot.channels[c]->symbol.accessor().getName();
			omeChannelXML.addAttribute("Name", channel_name.c_str());
		}

		
		int xml_size;
		XMLSTR ome_data=omeXML.createXMLString(1,&xml_size);
		
		//cout << "OME-TIFF Header" << endl;
		//cout << string(ome_data) << endl;
		
		// only update the TIFFTAG_IMAGEDESCRIPTION of the first image
		// note: this is incompatible with compressed timelapse images because compression disables random access 
		TIFFSetDirectory(output, 0); 
		TIFFSetField(output, TIFFTAG_IMAGEDESCRIPTION, ome_data);
		TIFFWriteDirectory(output);
		
	//  	for(uint i=0; i<im_desc.size(); i++){
	//  		TIFFSetDirectory(output, i); // only update the TIFFTAG_IMAGEDESCRIPTION of the first image
	//  		TIFFSetField(output, TIFFTAG_IMAGEDESCRIPTION, ome_data);
	//  		TIFFWriteDirectory(output);
	//  	}

		free(ome_data);
		
		
	/*	<?xml version="1.0" encoding="UTF-8"?>
		<OME xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
			xsi:schemaLocation="http://www.openmicroscopy.org/Schemas/OME/2015-01/ome.xsd">

			<OME:Image ID="Image:0" Name="Series 1">
				<OME:AcquisitionDate>2008-02-06T13:43:19</OME:AcquisitionDate>
				<OME:Description></OME:Description>
				<OME:Pixels DimensionOrder="XYCZT" ID="Pixels:0" 
					PhysicalSizeX="0.207" PhysicalSizeY="0.207"
					SizeC="3" SizeT="16" SizeX="1024" SizeY="1024" SizeZ="1"
					TimeIncrement="120.1302" Type="uint16">
					<OME:Channel EmissionWavelength="523" ExcitationWavelength="488" ID="Channel:0:0"
						IlluminationType="Epifluorescence" Name="CH1" SamplesPerPixel="1"
						PinholeSize="103.5" AcquisitionMode="LaserScanningConfocalMicroscopy"/>
					<OME:Channel EmissionWavelength="578" ExcitationWavelength="561" ID="Channel:0:1"
						IlluminationType="Epifluorescence" Name="CH3" SamplesPerPixel="1"
						PinholeSize="127.24" AcquisitionMode="LaserScanningConfocalMicroscopy"/>
					<OME:Channel ExcitationWavelength="488" ID="Channel:0:2"
						IlluminationType="Transmitted"
						ContrastMethod="DIC" Name="TD1" SamplesPerPixel="1"  
						AcquisitionMode="LaserScanningConfocalMicroscopy"/>
					<BIN:BinData BigEndian="false" Length="0"/>
				</OME:Pixels>
			</OME:Image>
		</OME:OME>
	*/
	}
	
/*
	// Generate IMAGE_DESCRIPTION tiff tag for ImageJ / Fiji
	// NOTE: Does not work. Somehow, the description is updated, but the image turns 0.0/black completely
	if(timelapse() && append){
		im_desc[ instance_id ].frames += 1;
	}
	else{
		im_desc[ instance_id ].frames = 1;
	}
	int desc_images=-1, desc_channels=-1, desc_slices=-1;
	desc_channels = plot.channels.size();
	desc_slices = slices;
	desc_images = desc_channels * desc_slices * im_desc[ instance_id ].frames;
	
	stringstream desc;
	desc << "ImageJ=1.46k\n";
	desc << "images="<< desc_images <<"\n";
	desc << "channels="<< desc_channels <<"\n";
	desc << "slices="<< desc_slices <<"\n";
	desc << "hyperstack="<< (desc_slices>1?"true":"false")<<"\n";
	if(im_desc[ instance_id ].frames > 1){
		desc << "frames="<< im_desc[ instance_id ].frames <<"\n";
	}
	desc << "mode="<< (desc_channels>1?"composite":"color") <<"\n";
	desc << "loop="<< (timelapse && im_desc[ instance_id ].frames>1 ? "true":"false") <<"\n";
	desc << "min="<< im_desc[ instance_id ].min <<"\n";
	desc << "max="<< im_desc[ instance_id ].max <<"\n";
	
//	cout << "DESCRIPTION OUT:\n" << desc.str().c_str() << "\n";
	for(uint i=0; i<desc_images; i++){
		TIFFSetDirectory(output, i); // only update the TIFFTAG_IMAGEDESCRIPTION of the first image
		TIFFSetField(output, TIFFTAG_IMAGEDESCRIPTION, desc.str().c_str());
		TIFFWriteDirectory(output);
	}
*/

	TIFFClose(output);
	
};
			/*
			 * void TiffPlotter::writePDELayer()
			 * {
			 * 
			 *    TIFF *output;
			 *    uint32 width, height;
			 *    char *buffer;
			 *    int colors=3;
			 * 
			 *    // Open the output image
			 *    char filename[255];
			 *    string property_name = symbol.getFullName();
			 *    remove_spaces(property_name);
			 *	
			 *	if (timename)
			 *		if(channelnum!=-1)
			 *			sprintf(filename, "%s_%d_%s.tif", SIM::getTimeName().c_str(), channelnum, property_name.c_str());
			 *		else
			 *			sprintf(filename, "%s_%s.tif", SIM::getTimeName().c_str(), property_name.c_str());
			 *	else
			 *		if(channelnum!=-1)
			 *			sprintf(filename, "%06d_%d_%s.tif", int(SIM::getTime()/getInterval()), channelnum, property_name.c_str());
			 *		else
			 *			sprintf(filename, "%06d_%s.tif", int(SIM::getTime()/getInterval()), property_name.c_str());
			 * 
			 *	if((output = TIFFOpen(filename, "w")) == NULL){
			 *		cerr << "Could not open image " << filename << endl;
			 *		exit(-1);
			 }
			 
			 //     if (timename)
			 //         sprintf(filename, "%s_%s.tif", property_name.c_str(), SIM::getTimeName().c_str());
			 //     else
			 //         sprintf(filename, "%s_%06d.tif", property_name.c_str(), int(SIM::getTime()/getInterval()) );
			 //     if((output = TIFFOpen(filename, "w")) == NULL){
				 //       cerr << "Could not open image " << filename << endl;
				 //       exit(-1);
				 //     }
				 
				 // We need to know the width and the height before we can malloc
				 width  = latticeDim.x;
				 height = latticeDim.y;
				 
				 // allocate buffer
				 if((buffer = (char *) malloc(sizeof(char) * width * height * colors)) == NULL){
					 cerr << "Could not allocate enough memory" << endl;
					 exit(-1);
				 }
				 
				 // min and max values
				 if( !explicit_min ){
					 min = pde_layer->min_val();
				 }
				 if( !explicit_max ){
					 max = pde_layer->max_val();
				 }
				 
				 
				 VINT pos(0,0,0);
				 for (pos.z=0; pos.z<latticeDim.z; pos.z++)
				 {
					 for (pos.y=0; pos.y<latticeDim.y; pos.y++)
					 {
						 for (pos.x=0; pos.x<latticeDim.x; pos.x++)
						 {
							 // value -> RGB
							 double value = symbol.get(pos);
							 unsigned char rgb[3];
							 colorMap(rgb, (double)value, (double)min, (double)max);
							 
							 //cout << "min: " << min << ", max: " << max << "pos: " << pos << ", value: " << value << ", rgb: " << (int)rgb[0] << "," << (int)rgb[1] << "," << (int)rgb[2] << "\n";
							 
							 // update position in buffer
							 uint bufindex = (pos.y*latticeDim.x + pos.x)*colors;
							 
							 // write RGB values to buffer
							 (int8&)buffer[bufindex+0] = rgb[0]; // red
							 (int8&)buffer[bufindex+1] = rgb[1]; // green
							 (int8&)buffer[bufindex+2] = rgb[2]; // blue
							 //(int8&)buffer[bufindex+3] = alpha;  // alpha
							 
				 }
				 }
				 
				 // Write the tiff tags to the file
				 TIFFSetField(output, TIFFTAG_IMAGEWIDTH,    width);
				 TIFFSetField(output, TIFFTAG_IMAGELENGTH,   height);
				 TIFFSetField(output, TIFFTAG_COMPRESSION,   COMPRESSION_NONE);
				 TIFFSetField(output, TIFFTAG_PLANARCONFIG,  PLANARCONFIG_CONTIG);
				 TIFFSetField(output, TIFFTAG_PHOTOMETRIC,   PHOTOMETRIC_RGB);
				 TIFFSetField(output, TIFFTAG_BITSPERSAMPLE, 8);
				 TIFFSetField(output, TIFFTAG_SAMPLESPERPIXEL, colors); //RGB
				 
				 if( latticeDim.z>1 ){
					 // Writing single frame of the multiframe tiff 
					 TIFFSetField(output, TIFFTAG_SUBFILETYPE, FILETYPE_PAGE);
					 // Set the frame number 
					 TIFFSetField(output, TIFFTAG_PAGENUMBER, pos.z, latticeDim.z);
				 }
				 
				 // Actually write the image
				 if(TIFFWriteEncodedStrip(output, 0, buffer, width * height * colors) == 0){
					 cerr << "Could not write TIFF image " << endl;
					 exit(-1);
				 }
				 
				 if( latticeDim.z>1 )
					 TIFFWriteDirectory(output);
				 
				 }
				 free(buffer);
				 
				 TIFFClose(output);
			 };*/
			
