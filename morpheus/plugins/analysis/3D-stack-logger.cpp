#include "3D-stack-logger.h"

using namespace SIM;

// TODO
//
// - Include Alpha channel (ARGB or RGBA -- both do not seem to work)
//

//-------------------------------------------------------------------

REGISTER_PLUGIN(Stack3D);

Stack3D::Stack3D()
{
	vec_ct = CPM::getCellTypes();
};

Stack3D::~Stack3D(){};

//-------------------------------------------------------------------


void Stack3D::loadFromXML(const XMLNode node)	//einlesen der Daten aus der XML
{
	Analysis_Listener::loadFromXML(node);
	
    getXMLAttribute(node,"symbol-ref", symbol_str); // required
    getXMLAttribute(node,"celltype", celltype_str); // optional
    getXMLAttribute(node,"colormap", colormap_str); // optional
    getXMLAttribute(node,"exclude-medium", exclude_medium_str); // optional

};


void Stack3D::init(double time)
{
	Analysis_Listener::init( time );

	if( !CPM::isEnabled() ){
		cerr << "Stack3D cannot be used without CPM" << endl;
		exit(-1);
	}
	cpm_layer = CPM::getLayer();
	latticeDim = cpm_layer->size();
	
    single_celltype = false;

    if( celltype_str.empty() ){
        symbol = SIM::findSymbol<double>(symbol_str);
        if( !symbol.valid() ){
            cerr << "Stack3D: Symbol '" << symbol_str << "' is invalid. " << endl;
            exit(-1);
        }
    }
    else{
        celltype = CPM::findCellType(celltype_str);
        if (!celltype){
            cerr << "Stack3D: Celltype '" << celltype_str << "' does not exist! " << endl;
            exit(-1);
        }
        symbol = SIM::findSymbol<double>(symbol_str, celltype.get());
        if( !symbol.valid() ){
            cerr << "Stack3D: Symbol '" << symbol_str << "' is invalid. " << endl;
            exit(-1);
        }
        single_celltype = true;
    }

    cout << "Stack3D: Plotting symbol " << symbol_str << " (" << symbol.getFullName() << ")" << endl;

    //enum colorMap { JET, HOT, COLD, BLUE, POSITIVE, NEGATIVE, GREY, CYCLIC, RANDOM };
    if( colormap_str.empty()){
        clrmp = GREY;
    }
    else if ( colormap_str == "jet" )
        clrmp = JET;
    else if ( colormap_str == "hot" )
        clrmp = HOT;
    else if ( colormap_str == "cold" )
        clrmp = COLD;
    else if ( colormap_str == "blue" )
        clrmp = BLUE;
    else if ( colormap_str == "positive" )
        clrmp = POSITIVE;
    else if ( colormap_str == "negative" )
        clrmp = NEGATIVE;
    else if ( colormap_str == "grey" )
        clrmp = GREY;
    else if ( colormap_str == "cyclic" )
        clrmp = CYCLIC;
    else if ( colormap_str == "random" )
        clrmp = RANDOM;

    cout << "Stack3D: Colormap " << colormap_str << endl;

    exclude_medium = false;
    if( exclude_medium_str == "true" )
        exclude_medium = true;

};

//-------------------------------------------------------------------

void Stack3D::notify(double time)
{
	Analysis_Listener::notify(time);
	Stack3D::writeCPMLayer( time );
};

void Stack3D::finish(double time){};



void Stack3D::writeCPMLayer(double time)
{

    TIFF *output;
    uint32 width, height;
    char *buffer;
    int colors=3;

    // Open the output image
    char filename[255];
	string property_name = symbol.getFullName();
	remove_spaces(property_name);
    sprintf(filename, "%s_%s.tif", property_name.c_str(), SIM::getTimeName().c_str());
    if((output = TIFFOpen(filename, "w")) == NULL){
      cerr << "Could not open image " << filename << endl;
      exit(-1);
    }

    // We need to know the width and the height before we can malloc
    width  = latticeDim.x;
    height = latticeDim.y;

    double min = numeric_limits<double>::max();
    double max = numeric_limits<double>::min();

    // min and max values
    vector< shared_ptr < const CellType > > celltypes = CPM::getCellTypes();
    if( single_celltype  ){
        vector< CPM::CELL_ID > cell_ids = celltype->getCellIDs();
        for(uint j=0; j < cell_ids.size(); j++){
			if( symbol.getLinkType() == SymbolData::CellMembraneLink ){
				uint y_s = MembraneProperty::size.y;
				VINT pos(0,0,0);
				for(pos.y=0; pos.y<(y_s > 1 ? y_s : 1); pos.y++){
					for(pos.x=0; pos.x<MembraneProperty::size.x; pos.x++){
						double value = symbol.get( cell_ids[j], pos );
						if( value < min )
							min = value;
						else if ( value > max )
							max = value;
					}
				}
			}
			else{
				double value = symbol.get( cell_ids[j] );
				if( value < min )
					min = value;
				else if ( value > max )
					max = value;
			}

        }
    }
    else{
        for(uint i=0; i < celltypes.size(); i++){
            vector< CPM::CELL_ID > cell_ids = celltypes[i].get()->getCellIDs();
            for(uint j=0; j < cell_ids.size(); j++){
				if( symbol.getLinkType() == SymbolData::CellMembraneLink ){
					uint y_s = MembraneProperty::size.y;
					VINT pos(0,0,0);
					for(pos.y=0; pos.y<(y_s > 1 ? y_s : 1); pos.y++){
						for(pos.x=0; pos.x<MembraneProperty::size.x; pos.x++) {
							double value = symbol.get( cell_ids[j], pos );
							if( value < min )
								min = value;
							else if ( value > max )
								max = value;
						}
					}
				}
				else{
					double value = symbol.get( cell_ids[j] );
					if( value < min )
						min = value;
					else if ( value > max )
						max = value;
				}
			}
        }
    }

    // allocate buffer
    if((buffer = (char *) malloc(sizeof(char) * width * height * colors)) == NULL){
      cerr << "Could not allocate enough memory" << endl;
      exit(-1);
    }

    VINT pos(0,0,0);
    for (pos.z=0; pos.z<latticeDim.z; pos.z++)
    {
        for (pos.y=0; pos.y<latticeDim.y; pos.y++)
        {
            for (pos.x=0; pos.x<latticeDim.x; pos.x++)
            {
                // update position in buffer
                uint bufindex = (pos.y*latticeDim.x + pos.x)*colors;
                double value = 0;
                //unsigned char alpha = 0xFF;
                uint celltype_pos = CPM::getCellIndex( cpm_layer->get(pos).cell_id ).celltype;

                // if cell type is specified, only plot property of that cell type, and assume 0 for other cell types
                if( single_celltype ){

                    if( exclude_medium && celltypes[celltype_pos]->isMedium() ){
                        value = 0;
                        //alpha = 0x00;
                    }
                    // if pos is part of cell of chosen cell type, plot the value
                    else if( celltype_pos == celltype->getID() ){
						
						if( symbol.getLinkType() == SymbolData::CellMembraneLink ){
							if( CPM::isBoundary( pos ) )
								value = (double)symbol.get( pos );
							else
								value = 0;
						}
						else{
							value = (double)symbol.get( pos );
						}
                    }

                }
                // otherwise, plot property of all cell types
                else{
                    if( exclude_medium && celltypes[celltype_pos]->isMedium() ){
                        value = 0;
                        //alpha = 0x00;
                    }
                    else{
						if( symbol.getLinkType() == SymbolData::CellMembraneLink ){
							if( CPM::isBoundary( pos ) )
								value = (double)symbol.get( pos );
							else
								value = 0;
						}
						else{
							value = (double)symbol.get( pos );
						}
                    }
                }
                

                // value -> RGB
                unsigned char rgb[3];
                ColorMap(rgb, (double)value, (double)min, (double)max);

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
            /* Writing single frame of the multiframe tiff */
             TIFFSetField(output, TIFFTAG_SUBFILETYPE, FILETYPE_PAGE);
             /* Set the frame number */
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

    cout << endl;
    free(buffer);

    TIFFClose(output);


};


void Stack3D::ColorMap(unsigned char *rgb,double value,double min,double max)
{
//adapted from: http://www.koders.com/cpp/fidCFEC52F6E8D5CAF77FBCDB42FDB45A69EC48677B.aspx

    switch( clrmp ){
    case JET:{

      unsigned char c1=144;
      float max4=(max-min)/4;
      value-=min;
      if(value==HUGE_VAL)
        {rgb[0]=rgb[1]=rgb[2]=255;}
      else if(value<0)
        {rgb[0]=rgb[1]=rgb[2]=0;}
      else if(value<max4)
        {rgb[0]=0;rgb[1]=0;rgb[2]=c1+(unsigned char)((255-c1)*value/max4);}
      else if(value<2*max4)
        {rgb[0]=0;rgb[1]=(unsigned char)(255*(value-max4)/max4);rgb[2]=255;}
      else if(value<3*max4)
        {rgb[0]=(unsigned char)(255*(value-2*max4)/max4);rgb[1]=255;rgb[2]=255-rgb[0];}
      else if(value<max)
        {rgb[0]=255;rgb[1]=(unsigned char)(255-255*(value-3*max4)/max4);rgb[2]=0;}
      else {rgb[0]=255;rgb[1]=rgb[2]=0;}
      break;
    }
    case HOT:
        {
      float max3=(max-min)/3;
      value-=min;
      if(value==HUGE_VAL)
        {rgb[0]=rgb[1]=rgb[2]=255;}
      else if(value<0)
        {rgb[0]=rgb[1]=rgb[2]=0;}
      else if(value<max3)
        {rgb[0]=(unsigned char)(255*value/max3);rgb[1]=0;rgb[2]=0;}
      else if(value<2*max3)
        {rgb[0]=255;rgb[1]=(unsigned char)(255*(value-max3)/max3);rgb[2]=0;}
      else if(value<max)
        {rgb[0]=255;rgb[1]=255;rgb[2]=(unsigned char)(255*(value-2*max3)/max3);}
      else {rgb[0]=rgb[1]=rgb[2]=255;}

      break;
    }
    case COLD:
        {
      float max3=(max-min)/3;
      value-=min;
      if(value==HUGE_VAL)
        {rgb[0]=rgb[1]=rgb[2]=255;}
      else if(value<0)
        {rgb[0]=rgb[1]=rgb[2]=0;}
      else if(value<max3)
        {rgb[0]=0;rgb[1]=0;rgb[2]=(unsigned char)(255*value/max3);}
      else if(value<2*max3)
        {rgb[0]=0;rgb[1]=(unsigned char)(255*(value-max3)/max3);rgb[2]=255;}
      else if(value<max)
        {rgb[0]=(unsigned char)(255*(value-2*max3)/max3);rgb[1]=255;rgb[2]=255;}
      else {rgb[0]=rgb[1]=rgb[2]=255;}
      break;
    }
    case BLUE:
        {
      value-=min;
      if(value==HUGE_VAL)
        {rgb[0]=rgb[1]=rgb[2]=255;}
      else if(value<0)
        {rgb[0]=rgb[1]=rgb[2]=0;}
      else if(value<max)
        {rgb[0]=0;rgb[1]=0;rgb[2]=(unsigned char)(255*value/max);}
      else {rgb[0]=rgb[1]=0;rgb[2]=255;}
      break;
    }
    case POSITIVE:
        {
      value-=min;
      max-=min;
      value/=max;

      if(value<0){
      rgb[0]=rgb[1]=rgb[2]=0;
        return;
      }
      if(value>1){
      rgb[0]=rgb[1]=rgb[2]=255;
      return;
      }

      rgb[0]=192;rgb[1]=0;rgb[2]=0;
      rgb[0]+=(unsigned char)(63*value);
      rgb[1]+=(unsigned char)(255*value);
      if(value>0.5)
      rgb[2]+=(unsigned char)(255*2*(value-0.5));
      break;
    }
    case NEGATIVE:
        {
      value-=min;
      max-=min;
      value/=max;

      rgb[0]=0;rgb[1]=0;rgb[2]=0;
      if(value<0) return;
      if(value>1){
      rgb[1]=rgb[2]=255;
      return;
      }

      rgb[1]+=(unsigned char)(255*value);
      if(value>0.5)
      rgb[2]+=(unsigned char)(255*2*(value-0.5));
      break;
    }
    case CYCLIC:
        {
      float max3=(max-min)/3;
      value-=(max-min)*(float)floor((value-min)/(max-min));
      if(value<max3)
        {rgb[0]=(unsigned char)(255-255*value/max3);rgb[1]=0;rgb[2]=255-rgb[0];}
      else if(value<2*max3)
        {rgb[0]=0;rgb[1]=(unsigned char)(255*(value-max3)/max3);rgb[2]=255-rgb[1];}
      else if(value<max)
        {rgb[0]=(unsigned char)(255*(value-2*max3)/max3);rgb[1]=255-rgb[0];rgb[2]=0;}
      break;
    }
    case RANDOM:
        {
      srand((int)(65000*(value-min)/(max-min)));
      rgb[0]=(unsigned char)(255*rand());
      rgb[1]=(unsigned char)(255*rand());
      rgb[2]=(unsigned char)(255*rand());
      break;
    }
    case GREY:
        {
      max-=min;
      value-=min;
      rgb[0]=rgb[1]=rgb[2]=(unsigned char)(255*value/max);
      break;
    }
    default: // POSITIVE
        {
      value-=min;
      max-=min;
      value/=max;

      if(value<0){
      rgb[0]=rgb[1]=rgb[2]=0;
        return;
      }
      if(value>1){
      rgb[0]=rgb[1]=rgb[2]=255;
      return;
      }

      rgb[0]=192;rgb[1]=0;rgb[2]=0;
      rgb[0]+=(unsigned char)(63*value);
      rgb[1]+=(unsigned char)(255*value);
      if(value>0.5)
      rgb[2]+=(unsigned char)(255*2*(value-0.5));
      break;
    }
    }

}
