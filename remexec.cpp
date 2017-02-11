#include <iostream>
#include <fstream>
#include <string.h>
#include <vector>

std::ostream &operator << (std::ostream &os, std::istream &is){
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
		std::cout<<"Syntax: remexec [flags...] <task_name> <files...>\n";
		return 0;
	}
	
	// get flags from argv
	std::string flags;
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
		std::cout<<"Not enough arguments. At least you need to specify the template name.\n";
		return 1;
	}
	
	// last processed element without '-' from ARGV will be template-name
	std::string templateName = argv[i];
	int firstFilenamePosition = i+1;
	
	// get filenames and try to open them
	std::vector<std::ifstream*> filesv; // argc-1-i = this is number of filenames
	for(int j=0, argvc=i+1; argvc<argc; j++, argvc++){
		filesv.push_back(new std::ifstream(argv[argvc], std::ifstream::ate | std::ifstream::binary));
		if(!filesv[j]->good()){
			std::cout<<"Can't open file "<<argv[argvc]<<".\n";
			return 1;
		}
	}
	
	// if OK then output
	std::cout<<"EXEC "<<templateName<<std::endl;
	if(flags.length() != 0) 
		std::cout<<"Flags: "<<flags<<std::endl;
	for(int i=0; i<filesv.size(); i++){
		std::cout<<"File: "<<filesv[i]->tellg()<<" "<<argv[firstFilenamePosition+i]<<std::endl;
		filesv[i]->seekg(0);
		std::cout<<*filesv[i]<<"\n";
		filesv[i]->close();
		delete filesv[i];
	}
	
	return 0;
}
