#include "csv_reader.h"
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/tokenizer.hpp>

REGISTER_PLUGIN(CSVReader)

//TODO this code is often duplicated, but not really plugin dependent
CPM::CELL_ID createCell(CellType* ct, VINT newPos, CPM::CELL_ID cellID = CPM::NO_CELL) {
    if (CPM::getLayer()->get(newPos) == CPM::getEmptyState() && CPM::getLayer()->writable(newPos)) {
        auto newID = cellID == CPM::NO_CELL ? ct->createCell() : ct->createCell(cellID);
        CPM::setNode(newPos, newID);
        return newID;
    } else {
        // position is already occupied
        return CPM::NO_CELL;
    }
}
CSVReader::CSVReader() {
    filename.setXMLPath("filename");
    registerPluginParameter(filename);
}

vector<CPM::CELL_ID> CSVReader::run(CellType *ct) {
    cell_type = ct;
    vector<CPM::CELL_ID> cells;

    auto file_content = readTextFile(filename());

    boost::char_separator<char> sep("\n");
    boost::tokenizer<boost::char_separator<char> > lines(file_content, sep);
    boost::escaped_list_separator<char> els('\\', ',','\"');
    for(auto& l: lines ) {
        if(l.front() == '#') continue;
        //TODO make header detection
        boost::tokenizer<boost::escaped_list_separator<char>> tok(l, els);
        vector<string> tokens;
        copy(tok.begin(), tok.end(), back_inserter<vector<string> >(tokens));
        if(tokens.size() < 3) throw MorpheusException(string("Expecting at least 3 fields on line ") + l, getXMLNode());
        VINT newPos{stoi(tokens[1]), stoi(tokens[2]), tokens.size() == 4 ? stoi(tokens[3]) : 0};
        auto new_cell = createCell(cell_type, newPos, (CPM::CELL_ID) stoul(tokens[0]));
        if (new_cell != CPM::NO_CELL) cells.push_back(new_cell);
    }

    return cells;
}


string CSVReader::readTextFile(string filename) {
    //TODO C++17 has all this stuff, and boost is not necessary anymore
    namespace bfs = boost::filesystem;

    bfs::path p{filename};

    if (!exists(p) || !is_regular_file(p)) {
        throw MorpheusException(string("Can't find file ") + p.string(), getXMLNode());
    }

    bfs::ifstream file{p};
    const auto file_size = bfs::file_size(p);
    string result(file_size, '\0');
    file.read(&result[0], file_size);

    return result;
};

