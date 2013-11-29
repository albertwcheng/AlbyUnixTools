#include <iostream>
#include <fstream>
#include <StringUtil.h>
#include <vector>
#include <map>
#include "AdvGetOptCpp/AdvGetOpt.h"
#include <ColumnsDirectives.h>


using namespace std;

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
    //outArgsHelp("-w warningLevel [default:1]","set warning level. level 0=print no warnings. Level 1=print matching summary. Level 2=print unmatched keys");
    
}


class JoinOpts{
public:
    vector<FileColSelector> filesels;
    vector<string> addPrefix;
    string line;
    string fillstring;
    int warningLevel;
    
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
  
    
    string fs;
    int startRow;
    int headerRow;
    
    JoinOpts(){
        fs="\t";
        startRow=2;
        headerRow=1;
        
        fillstring="";
        warningLevel=1;
        
    }
    
    ~JoinOpts(){
    
    }
    
    class KeyedFileReader{
    public:
        string filename;
        vector<int> columns0;
        vector<int> printCols;
        int numPrintCols;
        string NAValue;
        int numFields;
        string NAString;
        string line;
        vector<string> fields;
        string key;
        string fs;
        ifstream fil;
        int lino;
        bool status;
        
        int getLineNumber(){
            return lino;
        }
        
        KeyedFileReader(string _filename,const vector<int>& _columns0,string _fs,int _numFields,string _NAValue):filename(_filename),fil(_filename),columns0(_columns0),numFields(_numFields),NAValue(_NAValue),fs(_fs),lino(0),status(true){

            bool *printBool=new bool[numFields];
            
            for(int i=0;i<numFields;i++){
                printBool[i]=true;
            }
            
            for(vector<int>::const_iterator i=columns0.begin();i!=columns0.end();i++){
                printBool[(*i)]=false;
            }
            
            
            for(int i=0;i<numFields;i++){
                if(printBool[i]){
                    printCols.push_back(i);
                    
                }
            }
            
            delete[] printBool;
            
            numPrintCols=printCols.size();
            
            NAString=NAValue;
            for(int i=1;i<numPrintCols;i++){
                NAString+=fs+NAValue;
            }
            
            
            
        }
        
        string getPrintLine(const string& prefix=""){
            if(status)
                return joinFieldsOnIndices(this->fields,printCols,fs,prefix);
            
            return NAString;
        }
        
        bool readLine(){
            
            if(!fil.good()){
                status=false;
                return status;
            }
            
            
            getline(fil,this->line);

            if (fil.good() && this->line.length()>0) {
                StringUtil::split(this->line,fs,this->fields);
                
                this->key=joinFieldsOnIndices(this->fields,columns0,fs);
                
            
                status=true;
            }else{
                status=false;
            }
            
            return status;
        }
        ~KeyedFileReader(){
            fil.close();
        }
    };
    
    
    void join(){

        
        vector<string> fields;
        
        ifstream fil(this->filesels[0].filename);
        
        
        uint64_t lino=0;
        
        int numFiles=this->filesels.size();
        
        //init the files
        KeyedFileReader** fils=new KeyedFileReader*[numFiles];
        
        for(int i=0;i<numFiles;i++){
            //KeyedFileReader(string _filename,const vector<int>& _columns0,string _fs,int _numFields,string _NAValue):
            fils[i]=new KeyedFileReader(this->filesels[i].filename,this->filesels[i].columns0,this->fs,this->filesels[i].numFields(),this->fillstring);
            fils[i]->readLine();
        }
        
        //now the real deal
        
        while(true)
        {
            lino++;
            
            string outline;
            
            if(lino==this->headerRow && this->addPrefix[0]!="")
            {
                bool *printBool=new bool[this->filesels[0].numFields()];
                
                for(int i=0;i<this->filesels[0].numFields();i++){
                    printBool[i]=true;
                }
                
                for(vector<int>::iterator i=this->filesels[0].columns0.begin();i!=this->filesels[0].columns0.end();i++){
                    printBool[*i]=false;
                }
                
                
                outline=(printBool[0]?this->addPrefix[0]:"")+fils[0]->fields[0];
               
                
                for(int i=1;i<fils[0]->fields.size();i++){
                    outline+=this->fs+(printBool[i]?this->addPrefix[0]:"")+fils[0]->fields[i];
                }
                
                
                delete[] printBool;
            }
            else
            {
                outline=fils[0]->line;
            }
            
            string key=fils[0]->key;
            int fileUnmatched=0;
            
            
            for(int i=1;i<numFiles;i++){
                
                if(!fils[i]->status || fils[i]->key!=key){
                    fileUnmatched++;
                    outline+=this->fs+fils[i]->NAString;
                }else{ //implies fils[i]->key == fils[0]->key
                    outline+=this->fs+fils[i]->getPrintLine((lino==this->headerRow)?this->addPrefix[i]:"");
                    fils[i]->readLine();
                }
            }
            
            
            if(this->fillstring!="" || fileUnmatched==0)
                cout<<outline<<endl;
            
            
            fils[0]->readLine();
            
            if(!fils[0]->status)
            {
                break;
            }
            
            
        }
        
        
        for(int i=0;i<numFiles;i++){
            delete fils[i];
        }
        
        delete[] fils;

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
    
   // opts.fillstring="NA";
    
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
            

        }
        
        if(opts.filesels.size()<2){
            cerr<<"less than 2 files specified. Abort"<<endl;
            printUsage(argsFinal.programName);
            return 1;
        }
        
        
        //now, do the cljoin!
        opts.join();
        
        
        
        return 0;
    }
    else{
        
        printUsage(argsFinal.programName);
        return 1;
    }
    
}