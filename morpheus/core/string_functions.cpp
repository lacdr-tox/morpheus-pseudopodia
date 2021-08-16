#include "string_functions.h"

istream& operator >> (istream & stream, deque<double>& q){
	double tmp;
	int nread(0);

	while (1) {
		stream >> tmp;
		if ( stream.eof() ||  ! stream.fail() ) break;
		q.push_back(tmp);
		nread++;
	}
	return stream;
}
ostream& operator << (ostream & stream, const deque<double>& q) {
	copy(q.begin(), q.end(), ostream_iterator<double>(stream," "));
	return stream;
}


ostream& operator << (ostream& out, bool b) {
	if (b) out << "true";
	else out << "false";
	return out;
}

istream& operator >> (istream& in, bool& b){
	string val; char c;
	while (! in.fail()) {
		in.get(c);
		if ( in.fail()  || !isalnum(c)) break;
		val.push_back(c);
	}
	if (val.empty()) return in;
	if (val =="1")  { b=true; return in; }
	if (val =="0")  { b=false; return in; }
	lower_case(val);
	if ( val=="true") b= true;
	else b=false;
	return in;
}


// string to_str(int value, int width) {
// 	static string s;
// 	s.resize()
// 	snprintf()
// 	static bool init=false;
// 	if (!init)  { s.fill('0'); init=true; }
// 	else  { s.clear(); s.str("");}
// 	if (width>=0)
// 		s.width(width);
// 	else 
// 		s.width(0);
// 	s << value; 
// 	return s.str();
// }

/*string to_str(uint value, int width) {
	static stringstream s("");
	static bool init=false;
	if (!init)  { s.fill('0'); init=true; }
	else  { s.clear(); s.str("");}
	if (width>=0)
		s.width(width);
	else 
		s.width(0);
	s << value; 
	return s.str();
}*/
/*
string to_str(bool value, int prec) {
	if (value) return "true";
	return "false";
}*/

string strip_last_token(string& s, const string& del) {
	string token;
	string::size_type pos = s.find_last_of(del);
	if (pos == string::npos) {
		token = s;
		s="";
	}
	else {
		pos++;
		token = s.substr(pos);
		s.resize(pos-1);
	}
	return token;
}

string remove_spaces(string& str){
	str.erase (std::remove (str.begin(), str.end(), ' '), str.end());
	return str;
}

string replace_spaces(string& str, char replacement){
    std::replace(str.begin(), str.end(), ' ', replacement);
    return str;
}

bool replace_substring(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}



// vector<string> split_string(const string& str, const string& delimiters = " ");
vector<string> tokenize(const string& str, const string& delimiters, bool drop_empty_tokens) {

	vector<string> tokens;
	string::size_type fPos=0,lPos=0;
	do {
		// Skip delimiters at beginning.
		fPos = str.find_first_not_of(delimiters, lPos);
		// Don't add empty strings
		if (fPos == string::npos) break;
		// Iterate until first delimiter
		lPos = str.find_first_of(delimiters, fPos);
		// eat all the string left, if no further delimiter was found
		if (lPos == string::npos) lPos = str.length();
		// Add the token to the vector.
		if (!drop_empty_tokens || lPos>fPos) tokens.push_back(str.substr(fPos, lPos - fPos));
	} while (lPos != str.length());

	return tokens;
}

// string join(const vector<string>& strings, string delim) {
// 	string res;
// 	if (strings.size()==0)
// 		return res;
// 	res = strings.front();
//     for (int i=1; i<strings.size() ;i++) {
// 		res.append(delim).append(strings[i]);
// 	}
// 	return res;
// }
// 
// string join(const set<string>& strings, string delim) {
// 	string res;
// 	if (strings.size()==0)
// 		return res;
// 	res = *strings.begin();
// 	for (set<string>::const_iterator i=++strings.begin(); i!=strings.end(); i++) {
// 		res.append(delim).append(*i);
// 	}
// 	return res;
// }
// 
// string join(const multiset<string>& strings, string delim) {
// 	string res;
// 	if (strings.size()==0)
// 		return res;
// 	res = *strings.begin();
// 	for (set<string>::const_iterator i=++strings.begin(); i!=strings.end(); i++) {
// 		res.append(delim).append(*i);
// 	}
// 	return res;
// }

string& lower_case(string& a) {
	transform(a.begin(), a.end(), a.begin(), (int(*)(int))std::tolower);
	return a;
}

string lower_case(const char* a) {
	string l(a);
	lower_case(l);
	return l;
}
