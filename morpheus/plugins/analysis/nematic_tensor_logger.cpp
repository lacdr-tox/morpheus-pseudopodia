#include "nematicTensorLogger.h"

REGISTER_PLUGIN(NematicTensorLogger);

using namespace Eigen;

void NematicTensorLogger::finish(double time){
// close file
}

set< string > NematicTensorLogger::getDependSymbols()
{
    set< string > s;
    s.insert(symbol_str);
    return s;
}

void NematicTensorLogger::loadFromXML(const XMLNode Node)
{
	Analysis_Listener::loadFromXML( Node );
    getXMLAttribute(Node, "celltype", celltype_str);
    getXMLAttribute(Node, "symbol-ref", symbol_str);
    threshold = 0;
    getXMLAttribute(Node, "threshold", threshold);
}


void NematicTensorLogger::init(double time)
{
	Analysis_Listener::init(time);
	celltype = CPM::findCellType( celltype_str );
    SymbolAccessor<double> symbol = SIM::findSymbol<double>(symbol_str);
    if ( (symbol.getLinkType() != SymbolData::CellMembraneLink) || (!celltype->findMembrane( symbol_str ).valid()) ){
        cerr << "NematicTensorLogger: Symbol '" <<  symbol_str << "' does not refer to a MembraneProperty." << endl;
        exit(-1);
    }
	
    if( SIM::getLattice()->getDimensions() != 3 ){
        cerr << "Error: NematicTensorLogger is only implemented for MembraneProperty in 3D." << endl;
		exit(-1);
	}
}

void NematicTensorLogger::notify(double time)
{
    Analysis_Listener::notify(time);

    vector<CPM::CELL_ID> cells = celltype -> getCellIDs();

    int dimensions = SIM::getLattice()->getDimensions()-1;
//#pragma omp parallel for
    for(int c=0; c < cells.size(); c++) {

        VDOUBLE center = CPM::getCell( cells[c] ).getCenter();
        vector<double> I(dimensions*3, 0.0);
        VDOUBLE P(0,0,0);
        int count_api=0, count_mem=0;

        const Cell::Nodes& membrane = CPM::getCell( cells[c] ).getSurface();
        for ( Cell::Nodes::const_iterator m = membrane.begin(); m != membrane.end(); ++m ) {
            CellMembraneAccessor membrane = celltype->findMembrane( symbol_str );
            if( membrane.get( SymbolFocus(*m) ) >= threshold ){
                VDOUBLE delta = (*m - center).norm(); // normalize to unit sphere
                I[LC_XX]+=sqr(delta.y)+sqr(delta.z);
                I[LC_YY]+=sqr(delta.x)+sqr(delta.z);
                I[LC_ZZ]+=sqr(delta.x)+sqr(delta.y);
                I[LC_XY]+=-delta.x*delta.y;
                I[LC_XZ]+=-delta.x*delta.z;
                I[LC_YZ]+=-delta.y*delta.z;
                P.x += delta.x;
                P.y += delta.y;
                P.z += delta.z;
                count_api++;
                //cout << "delta = " <<  delta << " | P = " << P << "\n";
            }
            count_mem++;
        }
        // calculate average direction
        P.x = P.x / count_api;
        P.y = P.y / count_api;
        P.z = P.z / count_api;
        //P = P.norm();

        tensor = calcLengthHelper3D(I,count_api);
        tensor.ave_direction = P;

        cout << "TENSOR of Cell " << cells[c] << "\t, center: " << center << "\n";
        for(int i=0;i<3;i++){
            cout    << "Axis " << i+1 << " = " << tensor.axes[i]
                    << "\tEigenvalues = " << tensor.eigenvalues[i]/count_mem
                    << "\tLength = " << tensor.lengths[i] << "\n";
        }
        cout << "\tAve_direction = " << tensor.ave_direction << "\n";
        cout << endl;

        // output file
        ofstream fout;
        stringstream ss;
        ss << "tensor_" << cells[c] << ".log";
        fout.open(ss.str().c_str(), ios::out );
        // cell center
        fout << "#cell  center" << "\n";
        fout << center << "\n";
        // eigenvectors
        fout << "#eigen vectors" << "\n";
        for(int i=0;i<3;i++){
            fout << tensor.axes[i] << "\n";
        }
        fout << "#lengths" << "\n";
        // eigenslength
        for(int i=0;i<3;i++){
            if(i<2)
                fout << tensor.lengths[i] << ",";
            else
                fout << tensor.lengths[i] << "\n";
        }
        fout << "#nodes" << "\n";
        for ( Cell::Nodes::const_iterator m = membrane.begin(); m != membrane.end(); ++m ) {
            CellMembraneAccessor membrane = celltype->findMembrane( symbol_str );
            if( membrane.get( SymbolFocus(*m) ) >= threshold ){
                fout << (*m - center) << "\n";
            }
        }
        fout.close();
    }

}


NematicTensorLogger::nematicTensor NematicTensorLogger::calcLengthHelper3D(const std::vector<double> &I, int N) const
{
    NematicTensorLogger::nematicTensor nt;
    if(N<=1) {
        for (int i=0; i<3; i++) {
            nt.lengths.push_back(N);
            nt.axes.push_back(VDOUBLE(0,0,0));
            nt.eigenvalues.push_back(0);

        }
        return nt;
    }

//    double a=-I[LC_XX] - I[LC_YY] - I[LC_ZZ],
//    b=-I[LC_XY]*I[LC_XY] - I[LC_XZ]*I[LC_XZ] - I[LC_YZ]*I[LC_YZ] + I[LC_XX]*I[LC_YY] + I[LC_XX]*I[LC_ZZ] + I[LC_YY]*I[LC_ZZ],
//    c=+I[LC_XZ]*I[LC_XZ]*I[LC_YY] + I[LC_XX]*I[LC_YZ]*I[LC_YZ] + I[LC_XY]*I[LC_XY]*I[LC_ZZ] - 2.*I[LC_XY]*I[LC_XZ]*I[LC_YZ] - I[LC_XX]*I[LC_YY]*I[LC_ZZ];
//    double q=1./3.*b-1./9.*a*a,
//    r=1./6.*(a*b-3.*c)-1./27.*a*a*a;
//    std::complex<double> s1=pow(r+sqrt(std::complex<double>(q*q*q)+r*r),1./3.),
//                                    s2=pow(r-sqrt(std::complex<double>(q*q*q)+r*r),1./3.);
//    double lambda[3];
//    lambda[0]=real((s1+s2)-a/3.);
//    lambda[1]=real(-0.5*(s1+s2)-a/3.+std::complex<double>(0,0.5*sqrt(3))*(s1-s2));
//    lambda[2]=real(-0.5*(s1+s2)-a/3.-std::complex<double>(0,0.5*sqrt(3))*(s1-s2));
//    for(int i=0;i<3;i++)
//        lambda[i]=max(0.,lambda[i]);

//    for(int flag=1;flag;) {
//        flag=0;
//        for(int i=0;i+1<3;i++){  // sort
//            if (lambda[i]<lambda[i+1]){
//                swap(lambda[i],lambda[i+1]);
//                flag=1;
//                break;
//            }
//        }
//    }
//    for(int i=0;i<3;i++){  // sort
//        cout << "lambda["<< i <<"]: " << lambda[i] << endl;
//        nt.eigenvalues.push_back( lambda[i] );
//    }


    // From of the inertia tensor (principal moments of inertia) we compute the eigenvalues and
    // obtain the cell length by assuming the cell was an ellipsoid
    Matrix3f eigen_m;
    eigen_m << I[LC_XX], I[LC_XY], I[LC_XZ],
               I[LC_XY], I[LC_YY], I[LC_YZ],
               I[LC_XZ], I[LC_YZ], I[LC_ZZ];
    SelfAdjointEigenSolver<Matrix3f> eigensolver(eigen_m);
    if (eigensolver.info() != Success) {
        cerr << "NematicTensorLogger::calcLengthHelper3D: Computing eigenvalues failed!" << endl;
    }

    Vector3f eigen_values = eigensolver.eigenvalues();
    Matrix3f EV = eigensolver.eigenvectors();
    Matrix3f Am;
    Am << -1,  1,  1,
           1, -1,  1,
           1,  1, -1;
    Array3f axis_lengths = ((Am * eigen_values).array() * (2.5/double(N))).sqrt();
    //Vector3i sorted_indices;
    for (int i=0; i<3; i++) {
        nt.lengths.push_back(axis_lengths(i));
        nt.eigenvalues.push_back(eigen_values(i));
        nt.axes.push_back(VDOUBLE(EV(0,i),EV(1,i),EV(2,i)).norm());
    }
    // sorting axes by length
    bool done=false;
    while (!done) {
        for (int i=0;1;i++) {
            if (nt.lengths[i] < nt.lengths[i+1]) {
            //if (nt.eigenvalues[i] < nt.eigenvalues[i+1]) {
                swap(nt.lengths[i],nt.lengths[i+1]);
                swap(nt.eigenvalues[i], nt.eigenvalues[i+1]);
                swap(nt.axes[i],nt.axes[i+1]);
            }
            if (i==2) {
                done=true;
                break;
            }
        }
    }

     return nt;
}

