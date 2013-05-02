#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <cstring>
#include <ctime>
#include <string>
#include <limits>
#include <iostream>
#include <fstream>
#include "PdfConverter.h"
#include "util/path.h"
#include "boost/lexical_cast.hpp"
#include "pdf_worker.h"
#include <Magick++.h>

using namespace std;
using namespace pdf2htmlEX;
using namespace Magick;

const bool ISDEBUG = true;
class ConvertService{
 private:
	string destBasePath = "/data/htmls";
 public:
  ConvertService() {
  }
  void convert(PdfJobResult& _return, const PdfJob& job) {
	clock_t t0 = clock();
	_return.isSuccess = false;
    cout << "converting ... " <<endl;

	//generate dest file path
	int64_t id = job.id;
    char str[sizeof(int64_t)*8 +1]="";
	sprintf(str,"%03ld", id % 1000);
	string path = str;
	while(id>=1000) id /= 10;
	sprintf(str,"%03ld", id);
	path = destBasePath + "/" + path + "/" + str + "/" + boost::lexical_cast <string>(job.id);
	if(ISDEBUG){
		printf("des dir: %s \n",path.c_str());
	}
	try{
        create_directories(path);
    }
    catch (const string & s)
    {
        cerr << s << endl;
		_return.isSuccess = false;
		return;
    }
	if(job.pdfFile == ""){
		_return.isSuccess = false;
		return;
	}
	//generate the thumbnail of the first page
	try{
		Blob blob(job.pdfFile.data(),job.pdfFile.length());
		Image image;
		image.read(blob);
		image.profile ("*", Blob ());
        //const int width = image.columns (); 
        //const int height = image.rows (); 
        //cout << "w: " << width << " h: "<< height << endl; 
        image.resize("86x112");
        image.write(path + "/thumb.png");
    } catch (Exception & error_){
        cout << "Caught exception when generating thumbnail: " << error_.what () << endl;
    }
    try
    {
		const unsigned char* data = (const unsigned char*)(job.pdfFile.c_str());
		unsigned char* pdata = const_cast<unsigned char*> (data);
		int pageCount = render(path.c_str(),pdata,job.pdfFile.length(),100,0);
		if(pageCount>0){
			_return.isSuccess = true;
			_return.pageCount = pageCount;
		}
		printf("done\n");
    }
    catch (const char * s)
    {
        cerr << "Error: " << s << endl;
    }
    catch (const string & s)
    {
        cerr << "Error: " << s << endl;
    }

	
	clock_t t1 = clock();
	cout << "cost: " << (t1-t0)  << endl;
  }

};
