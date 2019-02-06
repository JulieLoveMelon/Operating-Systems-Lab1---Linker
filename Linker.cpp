#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include <sstream>
#include <stdlib.h>
#include <regex>
#include <vector>
#include <iomanip>
#include <math.h>

using namespace std;



class Symbol{
public:
	string name;
	int linenum;
	int offset;
	int value;
	string error;
	int modulenum;
	int moduleoffset;
	int used;
};

class Module{
public:
	int modulenum;
	int modulesize;
};

vector<Symbol> SymbolTable;

void _parseerror(int linenum, int offset, int errcode){
	static char* errstr[] = {
		"NUM_EXPECTED",             // Number expected
		"SYM_EXPECTED",             // Symbol Expected
		"ADDR_EXPECTED",            // Adressing Expected which is A/E/I/R
		"SYM_TOO_LONG",             // Symbol Name is too long
		"TOO_MANY_DEF_IN_MODULE",   // >16
		"TOO_MANY_USE_IN_MODULE",   // >16
		"TOO_MANY_INSTR",           // total num_instr exceeds memory size
	};
	cout << "Parse Error line " << linenum << " " << "offset " << offset << ": " << errstr[errcode] << "\n";
}

static char* err[] = {
	"Error: Absolute address exceeds machine size; zero used",                 //rule 8
	"Error: Relative address exceeds module size; zero used",                  //rule 9
	"Error: External address exceeds length of uselist; treated as immediate", //rule 6
	" is not defined; zero used",                                               //rule 3
	"Error: This variable is multiple times defined; first value used",        //rule 2
	"Error: Illegal immediate value; treated as 9999",                         //rule 10
	"Error: Illegal opcode; treated as 9999",                                  //rule 11
};

char* getToken(char* buffer){
	return strtok(buffer, " ,\t\n");
} 

int readInt(char* buffer){
	char* num = getToken(buffer);
	if(num == NULL)
		return -2;
	string str = num;
	regex rg("[0-9]+");
	bool isnum = regex_match(str.begin(), str.end(), rg);
	if(isnum){
		return atoi(num);
	}
	else
		return -1;
}


char readIAER(char* buffer){
	char* ch = getToken(buffer);
	if(ch == NULL)
		return 'N';
	if(! strcmp(ch, "I"))
		return 'I';
	else if(!strcmp(ch, "A"))
		return 'A';
	else if(!strcmp(ch, "E"))
		return 'E';
	else if(!strcmp(ch, "R"))
		return 'R';
	else
		return 'O';
}

Symbol readSymbol(char* buffer){
	char* token = getToken(buffer);
	Symbol sym;
	if(token != NULL){
		if(isalpha(token[0])){
			sym.name = token;
			return sym;
		}
		else{
			sym.error = "NOT CHAR";
			return sym;
		}
	}
	else{
		sym.name = "";
		return sym;
	}
}

void CreateSymbol(Symbol symbol){
	symbol.used = 0;
	SymbolTable.push_back(symbol);
	//cout << symbol.name << " " << symbol.value << "\n";
}

int CreateModule(char* buffer, ifstream &file, int linenum, 
				 int offset, int total_instr, int totalmodulesize,
				 int modulenum){
	int i;
	char buf[512];
	int multiple = 0;
	modulenum += 1;
	/****************************
			   DEFLIST
	****************************/
	int defcount = readInt(buffer);
	while(defcount == -2){
		if(!file.eof()){
			file.getline(buf, 512);
			if(!file.eof()){
				linenum += 1;
				offset = 0;
				defcount = readInt(buf);
				if(buf[0] == ' ')
					offset++;
			}
			else{
				return 1;
			}
		}
		else{
			/*
			//number expected, parse error 0, rule 1
			_parseerror(linenum, offset, 0);
			return 0;
			*/
			return 1;
		}
	}
	if(defcount == -1){
		//number expected, parse error 0, rule 1
		_parseerror(linenum, offset + 1, 0);
		return 0;
	}
	//a read defcount got
	total_instr += defcount;
	if(total_instr > 512){
		//too many instructions, parse error 6, rule 1
		_parseerror(linenum, offset + 1, 6);
		return 0;
	}
	if(defcount > 16){
		//too many def, parse error 4, rule 1
		_parseerror(linenum, offset + 1, 4);
		return 0;
	}
	//no parse error
	offset += floor(log(defcount)/log(10)) + 1;
	offset ++; //tab
	for(i = 0; i < defcount; i++){
		Symbol sym = readSymbol(NULL);
		if(sym.error == "NOT CHAR"){
			//symbol expected, parse error 1, rule 1
			_parseerror(linenum, offset + 1, 1);
		}
		while(sym.name == ""){
			if(!file.eof()){
				file.getline(buf, 512);
				if(file.eof()){
					//symbol expected, parse error 1, rule 1
					if(offset == 0)
						offset = 1;
					_parseerror(linenum, offset, 1);
					return 0;
				}
				else{
					sym = readSymbol(buf);
					linenum += 1;
					offset = 0;
				}
			}
			else{
				//symbol expected, parse error 1, rule 1
				if(offset == 0) offset = 1;
				_parseerror(linenum, offset, 1);
				return 0;
			}
		}
		if(sym.name.length() > 16){
			//symbol name too long, parse error 3, rule 1
			_parseerror(linenum, offset + 1, 3);
			return 0;
		}
		//a real symbol got
		sym.offset = offset + 1;
		sym.linenum = linenum;
		sym.modulenum = modulenum;
		offset += sym.name.length();
		offset ++; //tab
		int val = readInt(NULL);
		while(val == -2){
			if(!file.eof()){
				file.getline(buf,512);
				if(file.eof()){
					//number expected, parse error 0, rule 1
					if(offset == 0) offset = 1;
					_parseerror(linenum, offset, 0);
					return 0;
				}
				else{
					val = readInt(buf);
					linenum += 1;
					offset = 0;
				}
			}
			else{
				//number expected, parse error 0, rule 1
				if(offset == 0) offset = 1;
				_parseerror(linenum, offset, 0);
				return 0;
			}
		}
		if(val == -1){
			//not a number after symbole, parse error 0, rule 1
			//notice the offset
			_parseerror(linenum, offset + 1, 0);
			return 0;
		}
		//a read value got
		offset += floor(log(val) / log(10)) + 1;
		offset ++;//tab
		for(vector<Symbol>::iterator iter = SymbolTable.begin(); iter != SymbolTable.end(); iter++){
			if((*iter).name == sym.name){
				//rule 2, multiply defined symbol
				(*iter).error = err[4];
				multiple = 1;
			}
		}
		if(multiple == 0){
			sym.moduleoffset = val;
			sym.value = sym.moduleoffset + totalmodulesize;
			CreateSymbol(sym);
		}
	}

	/****************************
			   USELIST
	****************************/
	int usecount = readInt(NULL);  
	while(usecount == -2){
		if(!file.eof()){
			file.getline(buf, 512);
			if(file.eof()){
				//number expected, parse error 0, rule 1
				if(offset == 0) offset = 1;
				_parseerror(linenum, offset, 0);
				return 0;
			}
			else{
				usecount = readInt(buf);
				linenum += 1;
				offset = 0;
				if(buf[0] == ' ')
					offset ++;
			}
		}
		else{
			//number expected, parse error 0, rule 1
			if(offset == 0) offset = 1;
			_parseerror(linenum, offset, 0);
			return 0;
		}
	}
	if(usecount == -1){
		//not a number, parse error 0, rule 1
		_parseerror(linenum, offset + 1, 0);
		return 0;
	}
	total_instr += usecount;
	if(total_instr > 512){
		//too many instructions, parse error 6, rule 1
		_parseerror(linenum, offset + 1, 6);
		return 0;
	}
	if(usecount > 16){
		//too many use, parse error 5, rule 1
		_parseerror(linenum, offset + 1, 5);
		return 0;
	}
	//a real usecount got
	offset += floor(log(usecount)/log(10)) + 1;
	offset += 1; //tab
	for(i = 0; i < usecount; i++){
		Symbol sym = readSymbol(NULL);
		if(sym.error == "NOT CHAR"){
			//symbol expected, parse error 1, rule 1
			_parseerror(linenum, offset + 1, 1);
			return 0;
		}
		while(sym.name.empty()){
			if(!file.eof()){
				file.getline(buf, 512);
				if(file.eof()){
					//symbol expected, parse error 1, rule 1
					if(offset == 0) offset = 1;
					_parseerror(linenum, offset, 1);
					return 0;
				}
				else{
					sym = readSymbol(buf);
					linenum += 1;
					offset = 0;
					if(buf[0] == ' ')
						offset ++;
				}
			}
			else{
				//symbol expected, parse error 1, rule 1
				if(offset == 0) offset = 1;
				_parseerror(linenum, offset, 1);
				return 0;
			}
		}
		if(sym.name.length() > 16){
			//symbol name too long, parse error 3, rule 1
			_parseerror(linenum, offset + 1, 3);
			return 0;
		}
		//a real use symbol got
		offset += sym.name.length();
		offset += 1; //tab
	}

	/****************************
			PROGRAM TEXT
	****************************/
	int modulesize = readInt(NULL);
	while(modulesize == -2){
		if(!file.eof()){
			file.getline(buf, 512);
			if(file.eof()){
				//number expected, parse error 0, rule 1
				if(offset == 0) offset = 1;
				_parseerror(linenum, offset, 0);
				return 0;
			}
			else{
				modulesize = readInt(buf);
				linenum ++;
				offset = 0;
				if(buf[0] == ' ')
					offset ++;
			}
		}
		else{
			//number expected, parse error 0, rule 1
			if(offset == 0) offset = 1;
			_parseerror(linenum, offset, 0);
			return 0;
		}
	}
	if(modulesize == -1){
		//not a number, parse error 0, rule 1
		_parseerror(linenum, offset + 1, 0);
		return 0;
	}
	//a real modulesize got, test if any symbol exceeds the size
	for(vector<Symbol>::iterator iter = SymbolTable.begin(); iter != SymbolTable.end(); iter++){
		if((*iter).modulenum == modulenum){
			if((*iter).moduleoffset > modulesize){
				//symbol too large, rule 5
				cout << "Warning Module " << modulenum << ": " << (*iter).name << " too big "
					<< (*iter).moduleoffset << " (max = " << modulesize - 1 << ") assume zero relative\n";
				(*iter).value -= (*iter).moduleoffset;
				(*iter).moduleoffset = 0;
			}
		}
	}
	total_instr += modulesize;
	if(total_instr > 512){
		//too many instructions, parse error 6, rule 1
		_parseerror(linenum, offset + 1, 6);
		return 0;
	}
	totalmodulesize += modulesize;
	offset += floor(log(modulesize) / log(10)) + 1;
	offset += 1; //tab
	for(i = 0; i < modulesize; i++){
		char instr = readIAER(NULL);
		while(instr == 'N'){
			if(!file.eof()){
				file.getline(buf, 512);
				if(file.eof()){
					//address expected, parse error 2, rule 2
					if(offset == 0) offset = 1;
					_parseerror(linenum, offset, 2);
					return 0;
				}
				else{
					instr = readIAER(buf);
					linenum += 1;
					offset = 0;
					if(buf[0] == ' ')
						offset ++;
				}
			}
			else{
				//address expected, parse error 2, rule 2
				if(offset == 0) offset = 1;
				_parseerror(linenum, offset, 2);
				return 0;
			}
		}
		if(instr == 'O'){
			//address expected, parse error 1, rule 2
			_parseerror(linenum, offset + 1, 2);
			return 0;
		}
		//a real instruction got
		offset += 1; 
		offset += 1; //tab
		int op = readInt(NULL);
		while(op == -2){
			if(!file.eof()){
				file.getline(buf, 512);
				if(file.eof()){
					//number expected, parse error 0, rule 1
					if(offset == 0) offset = 1;
					_parseerror(linenum, offset, 0);
					return 0;
				}
				else{
					op = readInt(buf);
					linenum ++;
					offset = 0;
					if(buf[0] == ' ')
						offset ++;
				}
			}
			else{
				//number expected, parse error 0, rule 1
				if(offset == 0) offset = 1;
				_parseerror(linenum, offset, 0);
				return 0;
			}
		}
		if(op == -1){			
			//number expected, parse error 0, rule 1
			_parseerror(linenum, offset + 1, 0);
			return 0;
		}
		/*
		else if(op < 1000){
			//address expected, address digits less than 4, parse error 0, rule 2
			offset = offset + floor(log(op)/log(10)) + 3;
			_parseerror(linenum, offset, 2);
			return 0;
		}
		*/
		//a real opcode got
		offset += floor(log(op) / log(10));
		offset += 1; //tab
	}
	int cm;
	if(!file.eof()){
		cm = CreateModule(NULL, file, linenum, offset, total_instr, totalmodulesize, modulenum);
	}
	return cm;
}

int pass1(ifstream &file){
	vector<Module> ModuleTable;
	vector<Symbol> Uselist;
	int i = 0;
	string token;
	
	//open the file
	if(!file.is_open()){
		perror("Error opening file.");
		return 0;
	}
	
	char buffer[512];
	file.getline(buffer, 512);
	int linenum = 1;
	int offset = 0;
	int total_instr = 0;
	int totalmodulesize = 0;
	int modulenum = 0;
	if(buffer[0] == ' ')
		offset ++;
	int cm = CreateModule(buffer, file, linenum, offset, total_instr, totalmodulesize, modulenum);
	if(cm == 1)
		return 1;
	else
		return 0;
}

void pass2(char* buffer){
	int totalsize = 0;
	int linenum = 0;
	int modulenum = 1;
	int defcount = 0, usecount = 0, modulesize = 0;
	int i = 0;
	Symbol sym;
	int val = 0;
	char* ch;
	ch = strtok(buffer, " ,\t\n");
	defcount = atoi(ch);
	while(true){
		/*************************
				 DEFLIST
		*************************/
		for(i = 0; i < defcount; i++){
			ch = strtok(NULL, " ,\t\n");
			sym.name = ch;
			ch = strtok(NULL, " ,\t\n");
			val = atoi(ch);
		}
		/****************************
				USELIST
		****************************/
		ch = strtok(NULL, " ,\t\n");
		usecount = atoi(ch);
		vector<Symbol> usesymbol;
		for(i = 0; i < usecount; i++){
			ch = strtok(NULL, " ,\t\n");
			sym.name = ch;
			sym.used = 0;
			sym.modulenum = modulenum;
			usesymbol.push_back(sym);
			int find = 0;
			for(vector<Symbol>::iterator iter = SymbolTable.begin(); iter != SymbolTable.end(); iter++){
				if((*iter).name == sym.name){
					sym.value = (*iter).value;
					find = 1;
				}
			}
			if(find == 0){
				//not defined, rule 3
				sym.error = "Error: " + sym.name + err[3];
				sym.value = 0;
				SymbolTable.push_back(sym);
			}
		}
	
		/*****************************
				PROGRAM TEXT
		*****************************/
		char add = NULL;
		int op = 0;
		ch = strtok(NULL, " ,\t\n");
		modulesize = atoi(ch);
		for(i = 0; i < modulesize; i++){
			ch = strtok(NULL, " ,\t\n");
			add = ch[0];
			ch = strtok(NULL, " ,\t\n");
			op = atoi(ch);
			int opcode = op / 1000;
			int operand = op % 1000;
			cout << "\n" << setw(3) << setfill('0') << linenum << ": ";		
			if((op >= 10000) && (add != 'I')){
				//illegal opcode, rule 11
				cout << 9999 << " " << err[6];
				linenum ++;
				continue;
			}
			if(add == 'I'){
				if(op >= 10000){
					//illegal immediate value, rule 10
					op = 9999;
					cout << op << " " << err[5];
					linenum ++;
					continue;
				}
				cout << setw(4) << setfill('0') << op;
				linenum ++;
			}
			else if(add == 'A'){
				if(operand >= 512){
					//absolute address >= machine size, rule 8
					op = opcode * 1000;
					operand = 0;
					cout << setw(4) << setfill('0') << op << " " << err[0]; 
				}
				else{
					cout << setw(4) << setfill('0') << op;
				}	
				linenum ++;
			}
			else if(add == 'E'){
				if(operand >= usecount){
					//external address exceeds the length of uselist
					cout << setw(4) << setfill('0') << op << " " << err[2];
				}
				else{
					string sym = usesymbol[operand].name;
					usesymbol[operand].used = 1;
					string error;
					for(vector<Symbol>::iterator iter = SymbolTable.begin(); iter != SymbolTable.end(); iter++){
						if((*iter).name == sym){
							(*iter).used = 1;
							operand = (*iter).value;
							if(!(*iter).error.empty())
								error = (*iter).error;
						}
					}
					op = opcode * 1000 + operand;
					cout << setw(4) << setfill('0') << op;
					if(!error.empty())
						cout << " " << error;
				}
				linenum ++;
			}
			else{
				//R
				if(operand > modulesize){
					//relative address exceeds module size, rule 9
					operand = totalsize;
					op = opcode * 1000 + operand;
					cout << setw(4) << setfill('0') << op << " " << err[1];
				}
				else{
					op += totalsize;
					cout << setw(4) << setfill('0') << op;
				}
				linenum ++;
			}
		}
		totalsize += modulesize;
		for(vector<Symbol>::iterator iter = usesymbol.begin(); iter != usesymbol.end(); iter++){
			if((*iter).used == 0){
				// appear in the uselist but never used, rule 7
				cout << "\nWarning: Module " << modulenum << ": " << (*iter).name 
					<< " appeared in the uselist but was not actually used";
				(*iter).used = 1;
				for(vector<Symbol>::iterator it = SymbolTable.begin(); it != SymbolTable.end(); it++){
					if((*it).name == (*iter).name){
						(*it).used = 1;
					}
				}
			}
		}
		ch = strtok(NULL, " ,\t\n");
		if(ch != NULL){
			modulenum ++;
			defcount = atoi(ch);
		}
		else
			break;
	}
	cout << "\n";
	for(vector<Symbol>::iterator iter = SymbolTable.begin(); iter != SymbolTable.end(); iter++){
		if((*iter).used == 0){
			//defined but not used, rule 4
			cout << "\nWarning: Module " << (*iter).modulenum << ": " << (*iter).name
				<< " was defined but never used";
		}
	}
}

int main(){
	string filename;
	cin >> filename;
	ifstream file(filename);
	int ps1 = pass1(file);
	if(ps1 == 1){
		cout << "Symbol Table";
		for(vector<Symbol>::iterator iter = SymbolTable.begin(); iter != SymbolTable.end(); iter++){
			cout << "\n" << (*iter).name << "=" << (*iter).value << " ";
			if(!(*iter).error.empty()){
				cout << (*iter).error ;
				(*iter).error = "";
			}
		}
		cout << "\n\n";
		file.close();
		file.open(filename);
		ostringstream oss;
		char ch;
		while(oss && file.get(ch))
			oss.put(ch);
		char* buffer;
		string str = oss.str();
		buffer = (char*)str.c_str();
		cout << "Memory Map";
		pass2(buffer);
		return 1;
	}
	else{
		return 0;
	}
}