#include <iostream>
#include <fstream>
#include <StringUtil.h>
#include <vector>
#include <map>
#include "AdvGetOptCpp/AdvGetOpt.h"
#include <ColumnsDirectives.h>
#include <ListPermutator.h>


using namespace std;

class KeyedLines{
public:
    map<string,vector<string>* > keyedLines;
    string fillLine;
    
    int numPrintCols;
    
    static string joinFieldsOnIndices(const vector<string>& fields,const vector<int>& indices,const string& sep,const string& prefix=""){
        string fullString;
        vector<int>::const_iterator i=indices.begin();
        fullString=prefix+fields[*i];
        i++;
        for(;i!=indices.end();i++){
            fullString+=sep+prefix+fields[*i];
        }
        return fullString;
    }
    
    
    
    
    void print(ostream& os){
        for(map<string,vector<string>* >::iterator i=keyedLines.begin();i!=keyedLines.end();i++){
            string key=i->first;
            vector<string>* lines=i->second;
            os<<key<<endl;
            for(vector<string>::iterator j=lines->begin();j!=lines->end();j++){
                os<<"\t"<<(*j)<<endl;
            }
        }
    }
    
    vector<string>* getLinesByKey(const string& key){
    	map<string, vector<string>* >::iterator i=keyedLines.find(key);
    	if(i==keyedLines.end())
    		return NULL;
    	
    	return i->second;
    }
    
    void readFile(const string& filename,const string& sep, const vector<int>& keyCols0,int numFields,uint64_t startRow,string fillValue,bool skipKeyCols){
        vector<string> fields;
        ifstream fil(filename);
        string line;
        
        bool *printBool;
        vector<int> printCols;
        
        numPrintCols=numFields;
        
        if(skipKeyCols){
            
            printBool=new bool[numFields];
           
            for(int i=0;i<numFields;i++){
                printBool[i]=true;
            }
            
            for(vector<int>::const_iterator i=keyCols0.begin();i!=keyCols0.end();i++){
                printBool[(*i)]=false;
            }
            
            
            for(int i=0;i<numFields;i++){
                if(printBool[i]){
                    printCols.push_back(i);
                    
                }
            }
            
            delete[] printBool;
            
            numPrintCols=printCols.size();
            
        }
        
        
        this->fillLine=fillValue;
        
        for(int i=1;i<numPrintCols;i++){
            this->fillLine+=sep+fillValue;
        }
        
       
        
        uint64_t lino=0;
        while(fil.good()){
            
            line="";
            getline(fil,line);
            lino++;
            if(lino<startRow){
                continue;
            }
            if(fil.good() && line.length()>0){
                StringUtil::split(line,sep,fields);
                string key=joinFieldsOnIndices(fields,keyCols0,sep);
                
                
                
                
                map<string,vector<string>* >::iterator kli=keyedLines.find(key);
                if(kli==keyedLines.end()){
                    pair<map<string,vector<string>* >::iterator,bool> insertStat=keyedLines.insert(map<string,vector<string>* >::value_type(key,new vector<string>));
                    kli=insertStat.first;
                }
                
                if(skipKeyCols){
                    line=joinFieldsOnIndices(fields,printCols,sep);
                }
                
                kli->second->push_back(line);
            }
        }
        fil.close();
    }
    
    ~KeyedLines(){
        for(map<string,vector<string> *>::iterator i=keyedLines.begin();i!=keyedLines.end();i++){
        	delete i->second;
        }
    }
};


void outArgsHelp(string argname,string help){
    cerr<<argname<<"\t"<<help<<endl;
}

void printUsage(string programName){
    
    cerr<<"Usage: "<<programName<<" [options --cols col (--add-prefix pref) --of-file fileArg+] [file1 [ ... file10 ] ]"<<endl;
    cerr<<"preprocessor options:"<<endl;
    outArgsHelp("--@import-args filename","load arguments from tab delimited file");
    cerr<<"options:"<<endl;
    outArgsHelp("-1 cols1 -2 cols2  ... -9 cols9 -0 cols10","specify join fields of file1, file2, ... file9, file10");
    outArgsHelp("--cols cols --of-file filename","add more files and join fields after the args-specified files");
    outArgsHelp("--headerRow row","specify the header row number");
    outArgsHelp("--startRow row","specify the start row number");
    //outArgsHelp("--fs sep","specify the field separator");
    outArgsHelp("-f fill","fill the non-joint field with this character");
    outArgsHelp("-w warningLevel [default:1]","set warning level. level 0=print no warnings. Level 1=print matching summary. Level 2=print unmatched keys");
    
}


class JoinOpts:public ListPermutator{
public:
    vector<FileColSelector> filesels;
    vector<string> addPrefix;
    vector<KeyedLines*> fileKeyedLines;
    vector< vector<int> > printCols;
    vector<string>** foundsPerFile;
    string line;
    
    int warningLevel;
    
    
    string fs;
    int startRow;
    int headerRow;

    int sizes(int i){
        //cerr<<"sizes("<<i<<")="<<foundsPerFile[i+1]->size()<<endl;
        if(!foundsPerFile[i+1])
            return 1;
        return foundsPerFile[i+1]->size();
    }
    
    int numOfLists(){
        //cerr<<"numOfLists="<<(this->filesels.size()-1)<<endl;
        return this->filesels.size()-1;
    }
    
    bool onItem(const vector<int>& idx){
        //here is the hardcore printing stuff
        
        
        string thisLineAddon;
        
        for(int i=0;i<idx.size();i++){
           // cerr<<i<<endl;
            if(!foundsPerFile[i+1]){
                thisLineAddon+=this->fs+this->fileKeyedLines[i+1]->fillLine;
            }else{
                thisLineAddon+=this->fs+((*(foundsPerFile[i+1]))[idx[i]]);
            }
        }
        cout<<line<<thisLineAddon<<endl;
        return true;
    }

    
    string fillstring;
    JoinOpts(){
        fs="\t";
        startRow=2;
        headerRow=1;
        foundsPerFile=NULL;
        fillstring="";
        warningLevel=1;
        
    }
    ~JoinOpts(){
        for(vector<KeyedLines*>::iterator i=fileKeyedLines.begin();i!=fileKeyedLines.end();i++){
            delete (*i);
        }
    }

    
    
    
    void join(){
        //basefile is this->filesels[0]
        
        vector<string> fields;
        
        ifstream fil(this->filesels[0].filename);
        
        
        uint64_t lino=0;
        uint64_t lineMatched=0;
        uint64_t lineUnMatched=0;
        
        int numFiles=this->filesels.size();
        
        foundsPerFile=new vector<string>*[numFiles];
        
        while(fil.good()){
            
            
            
            
            line="";
            getline(fil,line);
            lino++;
            
           // cerr<<"lino="<<lino<<endl;

            if(fil.good() && line.length()>0){
                
                

                
                if(lino==this->headerRow){

                    int numFields=this->filesels[0].columns0.size();
                    
                    bool *printBool=new bool[numFields];
                    
                    
                    vector<int> curPrintCols;
                    for(int i=0;i<numFields;i++){
                        printBool[i]=true;
                    }
                    
                    for(vector<int>::iterator i=this->filesels[0].columns0.begin();i!=this->filesels[0].columns0.end();i++){
                        printBool[(*i)]=false;
                    }
                
                    
                    

                  
                    
                    
                    if(this->addPrefix[0]!=""){
                        vector<string> thisSplit;
                        StringUtil::split(this->filesels[0].preStartRows[lino-1],this->fs,thisSplit);
                        
                        line=((printBool[0])?this->addPrefix[0]:"")+thisSplit[0];
                        for(int i=1;i<thisSplit.size();i++){
                            line+=this->fs+((printBool[i])?this->addPrefix[0]:"")+thisSplit[i];
                        }
                        
                    }
                    
                    delete[] printBool;
                    
                    
                    
                    for(int i=1;i<numFiles;i++){
                        vector<string> thisSplit;
                        StringUtil::split(this->filesels[i].preStartRows[lino-1],this->fs,thisSplit);
                        line+=this->fs+KeyedLines::joinFieldsOnIndices(thisSplit,this->printCols[i],this->fs,this->addPrefix[i]);
                    }
                    
                    cout<<line<<endl;
                    
                    continue;
                }
                
                
                
                if(lino<this->startRow){
                    for(int i=1;i<numFiles;i++){
                        vector<string> thisSplit;
                        StringUtil::split(this->filesels[i].preStartRows[lino-1],this->fs,thisSplit);
                        line+=this->fs+KeyedLines::joinFieldsOnIndices(thisSplit,this->printCols[i],this->fs);
                    }
                    
                    cout<<line<<endl;
                    
                    continue;
                }
                
                
                StringUtil::split(line,this->fs,fields);
                string key=KeyedLines::joinFieldsOnIndices(fields,this->filesels[0].columns0,this->fs);
                
                
                bool toPrint=true;
                
                for(int i=1;i<numFiles;i++){
                    KeyedLines* thisKeyedLines=this->fileKeyedLines[i];
                    foundsPerFile[i]=thisKeyedLines->getLinesByKey(key);
                    if(!foundsPerFile[i] && this->fillstring==""){
                        //not found in one of the file and no fill option specified, skip this
                        toPrint=false;
                        break;
                    }
                }
                
                if(!toPrint){
                    if(this->warningLevel>=2){
                        cerr<<"line "<<lino<<" key "<<key<<" skipped"<<endl;
                    }
                    
                    lineUnMatched++;
                    
                    
                }else{
                
                    lineMatched++;
                    this->runThroughLists();
                }
                
                
            }
            
            
            
            
        }
        
        
        
        
        delete[] foundsPerFile;
        
        if(this->warningLevel>=1){
            cerr<< lineMatched <<" line(s) of file1 matched. "<<lineUnMatched<<" line(s) unmatched"<<endl;
        }
        
        
        fil.close();
    }
    
};


int main(int argc,char** argv)
{
    vector<string> long_options;
    //required:
    long_options.push_back("headerRow=");
    long_options.push_back("startRow=");
    long_options.push_back("fs=");
    long_options.push_back("cols=");
    long_options.push_back("of-file=");
    long_options.push_back("add-prefix=");
    
    //vector<string> filenames;
    //vector<FileColSelector> filesels;
    
    //multimap<string,string> optmap;
    JoinOpts opts;
    
    
    string curAddPrefix="";
    
    EasyAdvGetOptOut argsFinal=easyAdvGetOpt(argc,argv,"1:2:3:4:5:6:7:8:9:0:f:w:",&long_options);
    
    
    string curColString;
    
    if(argsFinal.success){
        //argsFinal.print(cerr);
        
        for(int i=0;i<argsFinal.args.size();i++){
            opts.addPrefix.push_back("");
            opts.filesels.push_back(FileColSelector(argsFinal.args[i]));
        }
        
        
        
        //parseOptsIntoMultiMap(argsFinal.opts,optmap);
        
        for(vector<OptStruct>::iterator i=argsFinal.opts.begin();i!=argsFinal.opts.end();i++){ //the latter values have more precedence!
            //optmap.insert(map<string,string>::value_type(i->opname,i->opvalue));
            if(i->opname=="--startRow"){
                opts.startRow=StringUtil::atoi(i->opvalue);
            }else if(i->opname=="--headerRow"){
                opts.headerRow=StringUtil::atoi(i->opvalue);
            }else if(i->opname=="--fs"){
                opts.fs=i->opvalue;
            }else if(i->opname=="-f"){
                opts.fillstring=i->opvalue;
            }
            else if(i->opname=="--cols"){
                curColString=i->opvalue;
            }else if(i->opname=="--add-prefix"){
                curAddPrefix=i->opvalue;
            }else if(i->opname=="--of-file"){
                opts.addPrefix.push_back(curAddPrefix);
                opts.filesels.push_back(FileColSelector(i->opvalue,curColString,opts.headerRow,opts.startRow,opts.fs,false));
            }else if(i->opname=="-1"){
                opts.filesels[0].setColString(i->opvalue,opts.headerRow,opts.startRow,opts.fs,false);
            }else if(i->opname=="-2"){
                opts.filesels[1].setColString(i->opvalue,opts.headerRow,opts.startRow,opts.fs,false);
            }else if(i->opname=="-3"){
                opts.filesels[2].setColString(i->opvalue,opts.headerRow,opts.startRow,opts.fs,false);
            }else if(i->opname=="-4"){
                opts.filesels[3].setColString(i->opvalue,opts.headerRow,opts.startRow,opts.fs,false);
            }else if(i->opname=="-5"){
                opts.filesels[4].setColString(i->opvalue,opts.headerRow,opts.startRow,opts.fs,false);
            }else if(i->opname=="-6"){
                opts.filesels[5].setColString(i->opvalue,opts.headerRow,opts.startRow,opts.fs,false);
            }else if(i->opname=="-7"){
                opts.filesels[6].setColString(i->opvalue,opts.headerRow,opts.startRow,opts.fs,false);
            }else if(i->opname=="-8"){
                opts.filesels[7].setColString(i->opvalue,opts.headerRow,opts.startRow,opts.fs,false);
            }else if(i->opname=="-9"){
                opts.filesels[8].setColString(i->opvalue,opts.headerRow,opts.startRow,opts.fs,false);
            }else if(i->opname=="-0"){
                opts.filesels[9].setColString(i->opvalue,opts.headerRow,opts.startRow,opts.fs,false);
            }else if(i->opname=="-w"){
                opts.warningLevel=StringUtil::atoi(i->opvalue);
            }
            
        }
        
        
        for(vector<FileColSelector>::iterator i=opts.filesels.begin();i!=opts.filesels.end();i++){
            
            i->parse();
            
            //cerr<<i->filename<<" cols0:";
            /*for(vector<int>::iterator j=i->columns0.begin();j!=i->columns0.end();j++){
                cerr<<" "<<(*j);
            }
            cerr<<endl;*/
        }
        
        if(opts.filesels.size()<2){
            cerr<<"less than 2 files specified. Abort"<<endl;
            printUsage(argsFinal.programName);
            return 1;
        }
        
        opts.fileKeyedLines.push_back(NULL);
        opts.printCols.push_back(vector<int>());
        
        //now read the file 2 ... file N into KeyedLines
        
        for(int fileno=1;fileno<opts.filesels.size();fileno++){
            KeyedLines* klines=new KeyedLines;
            
            
            int numFields=opts.filesels[fileno].numFields();
            
            
            klines->readFile(opts.filesels[fileno].filename,opts.fs, opts.filesels[fileno].columns0, numFields, opts.startRow, opts.fillstring, true);
            
            //cerr<<"read "<<opts.filesels[fileno].filename<<endl;
            //klines->print(cerr);
            
            
            
            bool *printBool=new bool[numFields];
            
            
            vector<int> curPrintCols;
            for(int i=0;i<numFields;i++){
                printBool[i]=true;
            }
            
            for(vector<int>::iterator i=opts.filesels[fileno].columns0.begin();i!=opts.filesels[fileno].columns0.end();i++){
                printBool[(*i)]=false;
            }
            
            //cerr<<numFields<<" printCols:";
            for(int i=0;i<numFields;i++){
                if(printBool[i]){
                    curPrintCols.push_back(i);
                    //cerr<<i;
                }
            }
            
            //cerr<<endl;
            
            
            delete[] printBool;
            
            opts.printCols.push_back(curPrintCols);
            opts.fileKeyedLines.push_back(klines);
            
            
        }
        
        //now, do the join!
        opts.join();
        
        
        
        return 0;
    }
    else{
        
        printUsage(argsFinal.programName);
        return 1;
    }
    
}