#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>
#include <vector>
#include <unordered_map>
//LINUX
#include <unistd.h>
#include <pwd.h>

using namespace std;

unordered_map<string,string> server;

int getConfig(){
	// get home dir and construct full name of config file
	char *homedir;
	if((homedir = getenv("HOME")) == NULL){
		homedir = getpwuid(getuid())->pw_dir;
	}
	string conffile = (string(homedir) + "/remexec.conf");
	
	// try to open config file
	ifstream conf(conffile.c_str());
	if(!conf.good()){
		return 1;
	}
	
	// read line from conf, turn it to string stream and separate to key and value
	string line;
	while(getline(conf, line)){
		istringstream stream_line(line);
		string key;
		if(getline(stream_line, key, ':')){
			string value;
			stream_line>>ws;
			if(getline(stream_line, value)){
				server[key]=value;
			}
		}
	}
	
	return 0;
}

ostream &operator << (ostream &os, istream &is){
	char buf[64];
	while (is.read(buf, sizeof(buf))){
		os.write(buf, sizeof(buf));
	}
	auto cnt = is.gcount();
	if (cnt > 0){
		os.write(buf, cnt);
	}
	return os;
}

int main(int argc, char **argv){
	// 0		1		1		2			3			4
	// remexec -flag0 -flag1 template-name	filename0	filename1
	// minimal args: remexec template-name
	if(argc == 1){
		cout<<"Syntax: remexec [flags...] <task_name> <files...>\n";
		return 0;
	}
	
	// get flags from argv
	string flags;
	int i=1;
	for(; i<argc; i++){
		if(argv[i][0] == '-'){
			int j=0;
			while(argv[i][j]){
				flags+=argv[i][j];
				j++;
			}
			flags+=' ';
		}else
			break;
	}
	
	if(i == argc){
		cout<<"Not enough arguments. At least you need to specify the template name.\n";
		return 1;
	}
	
	// last processed element without '-' from ARGV will be template-name
	string templateName = argv[i];
	int firstFilenamePosition = i+1;
	
	// get filenames and try to open them
	vector<ifstream*> filesv; // argc-1-i = this is number of filenames
	for(int j=0, argvc=i+1; argvc<argc; j++, argvc++){
		filesv.push_back(new ifstream(argv[argvc], ifstream::ate | ifstream::binary));
		if(!filesv[j]->good()){
			cout<<"Can't open file "<<argv[argvc]<<".\n";
			return 1;
		}
	}
	
	// read server address and port from conf file in home dir
	if(getConfig()){
		cout<<"Can't find or wrong data in config file.\n";
		return 1;
	};
	
	//configure web connection
	//cout<<server["address"]<<":"<<server["port"]<<endl;
	
	// if OK then output
	cout<<"EXEC "<<templateName<<endl;
	if(flags.length() != 0) 
		cout<<"Flags: "<<flags<<endl;
	for(int i=0; i<filesv.size(); i++){
		cout<<"File: "<<filesv[i]->tellg()<<" "<<argv[firstFilenamePosition+i]<<endl;
		filesv[i]->seekg(0);
		cout<<*filesv[i]<<"\n";
		filesv[i]->close();
		delete filesv[i];
	}
	
	return 0;
}
