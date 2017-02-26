#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>
#include <vector>
#include <unordered_map>
//LINUX
#include <unistd.h>
#include <pwd.h>

#include "cppsockets/sockets.hpp"

using namespace std;
using namespace sockets;

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
	char buf[4];
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
		cout<<"Syntax: remexec [flags...] [-g=file2fetch] <task_name> <files...>\n";
		return 0;
	}
	
	// get flags from argv
	string file2fetch;
	string flags;
	int i=1;
	for(; i<argc; i++){
		if(argv[i][0] == '-' && argv[i][1] == 'g'){
			int j=3;
			while(argv[i][j]){
				file2fetch+=argv[i][j];
				j++;
			}
		}else if(argv[i][0] == '-'){
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
	
	//configure and create connection
	socket_tcp cli(address_ip4(server["address"], stoi(server["port"])));
	cli.open();
	if(cli.valid()){
		string ok, tmp;
		// if OK then transmit files
		for(int i=0; i<filesv.size(); i++){
			cli<<"FILE\nName: "<<argv[firstFilenamePosition+i]<<"\nSize: "<<filesv[i]->tellg()<<"\n\n";
			filesv[i]->seekg(0);
			cli<<*filesv[i]<<"\n\n";
			filesv[i]->close();
			delete filesv[i];
			cli>>ok;
			if(ok != "OK"){
				cout<<"Server error while transmitting file: "<<ok<<endl;
				return 1;
			}
		}
		// command to fun with flags
		cli<<"EXEC "<<templateName<<endl<<endl;
		if(flags.length() != 0) 
			cli<<"Flags: "<<flags<<endl<<endl;
		cli>>ok;
		if(ok != "OK"){
			cout<<"Server error while execution: "<<ok<<endl;
			return 1;
		}
		
		// server answer's parser
		string line;
		while (getline(cli, line)){
			if (line.empty())
				continue;

			istringstream stream_line(line);
			string cmd;
			stream_line>>cmd;
			if(cmd == "STREAM"){
				int number, size;
				stream_line>>number; // get number of stream
				getline(cli, line);
				istringstream ss_size(line);
				ss_size>>tmp>>size; // get size of stream part
				char buf[size];
				cli.get();
				cli.read(buf, size);
				if(number == 1)
					cout.write(buf, size); // output to stdout
				else
					cerr.write(buf, size); // output to stderr
			} else if(cmd == "END")
				break;
		}
		
		// fetch file from server
		if(file2fetch != ""){
			ok = "";
			cli.get();
			cli<<"FETCH "<<file2fetch<<endl<<endl;
			cli>>ok;
			if(ok == "ERROR"){
				int code;
				cli>>code;
				getline(cli, ok);
				cout<<"Server error while fetching: "<<code<<ok<<endl;
				return 1;
			}else{
				string tmp;
				getline(cli, tmp);
				int size;
				cli>>tmp>>size; // get size of file
				getline(cli, tmp);
				getline(cli, tmp);
				ofstream of(file2fetch);
				char buf[size];
				cli.get();
				cli.read(buf, size);
				of.write(buf, size);
				of.close();
			}
		}
		
		// close connection
		cli<<"EXIT\n\n";
	}else{
		cout<<"Error while connecting to "<<server["address"]<<":"<<stoi(server["port"])<<endl;
	}
	
	return 0;
}
