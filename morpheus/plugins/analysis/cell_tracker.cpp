#include "cell_tracker.h"

REGISTER_PLUGIN(CellTracker);

CellTracker::CellTracker() {
//	celltype.setXMLPath("celltype");
//	registerPluginParameter(celltype);

    map<string, Format> formatMap;
    formatMap["ISBI 2012 (XML)"] = ISBI2012;
    formatMap["MTrackJ (MDF)"] = MTrackJ;
    format.setConversionMap(formatMap);
    format.setXMLPath("format");
    registerPluginParameter(format);
}

void CellTracker::loadFromXML(const XMLNode xNode)
{
    stored_node = xNode;
	AnalysisPlugin::loadFromXML(xNode);
}

void CellTracker::init(const Scope* scope)
{
//	celltype.init();
	AnalysisPlugin::init(scope);
    frame=0;

    // Specify dependency on cell position (not necessary as this plugin is not automaticallz scheduled anyway)
    registerCellPositionDependency();

    cout << "FORMAT: " << format() << endl;
    switch( format() ){
    case ISBI2012:
        cout << "CellTracker: writing ISBI 2012 XMl Format" << endl;
        break;
    default:
        throw MorpheusException("CellTracker: Format not implemented!", stored_node);
        break;
    }

}

void CellTracker::analyse(double time)
{
    auto celltypes = CPM::getCellTypes();
    for(uint ct=0; ct<celltypes.size(); ct++){

        if( celltypes[ct].lock()->isMedium() )
            continue;

        // iterate over all cells of the cell type
        vector< CPM::CELL_ID > cell_ids = celltypes[ct].lock()->getCellIDs();
        for(auto &c: cell_ids){

            // record the time frame and the position of the cell center (centroid)
            Spot spot;
            spot.frame = frame;
            spot.position = CPM::getCell( c ).getCenter();

            // add the spot to the cell track with the corresponding cell ID
            // note: new cell ids will be added to the STL map automatically (i.e. no need to check for prior existence)
            tracks[ c ].push_back( spot );
        }
    }

    // increment the time frame
    frame++;
}

void CellTracker::finish() {
    switch( format() ){
    case ISBI2012:
        write_ISBI_XML();
        break;
    default:
        throw MorpheusException("CellTracker: Format not implemented!", stored_node);
        break;
    }
}

void CellTracker::write_ISBI_XML( void ) {

    cout << "Writing cell tracks" << endl;

    string CONTENT_KEY = "TrackContestISBI2012";
    string DATE_ATT = "generationDateTime";
    string SNR_ATT = "snr";
    string DENSITY_ATT = "density";
    string SCENARIO_ATT = "scenario";

    string TRACK_KEY = "particle";
    string SPOT_KEY = "detection";
    string X_ATT = "x";
    string Y_ATT = "y";
    string Z_ATT = "z";
    string T_ATT = "t";

    XMLNode xDocument;
    xDocument = XMLNode::createXMLTopNode("root");

    XMLNode xTracks = XMLNode::createXMLTopNode( CONTENT_KEY.c_str() );
    xTracks.addAttribute( SNR_ATT.c_str(), "1" );
    xTracks.addAttribute( DENSITY_ATT.c_str(), "medium" );
    xTracks.addAttribute( SCENARIO_ATT.c_str(), "vesicle" );

    for(auto &track: tracks){

        XMLNode xTrack = XMLNode::createXMLTopNode( TRACK_KEY.c_str() );
        vector<Spot> spots = track.second;

        stringstream ss;
        for(auto &spot : spots){
            XMLNode xSpot= XMLNode::createXMLTopNode( SPOT_KEY.c_str() );
            ss.str(""); ss.clear(); ss << spot.frame;
            xSpot.addAttribute( T_ATT.c_str(), ss.str().c_str() );
            ss.str(""); ss.clear(); ss << spot.position.x;
            xSpot.addAttribute( X_ATT.c_str(), ss.str().c_str() );
            ss.str(""); ss.clear(); ss << spot.position.y;
            xSpot.addAttribute( Y_ATT.c_str(), ss.str().c_str() );
            ss.str(""); ss.clear(); ss << spot.position.z;
            xSpot.addAttribute( Z_ATT.c_str(), ss.str().c_str() );
            xTrack.addChild( xSpot );
        }

        xTracks.addChild( xTrack );
    }

    xDocument.addChild( xTracks );
    xDocument.writeToFile("celltracks.xml", "utf-8");
}
