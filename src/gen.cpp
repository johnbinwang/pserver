// Rendering a page of a PDF document to a PNG image in less than 100 lines.

// Compile a debug build of mupdf, then compile and run this example:
//
// gcc -g -o build/debug/example -I fitz doc/example.c \
//	build/debug/libfitz.a build/debug/libfreetype.a \
//	build/debug/libopenjpeg.a build/debug/libjbig2dec.a \
//	build/debug/libjpeg.a -lpng -lm
//
// build/debug/example /path/to/document.pdf 1 200 25

// Include the MuPDF header file.
#include <fitz.h>
#include <wchar.h>
#include <string.h>
#include <locale.h>
#define MAX_SEARCH_HITS (500)


static int textlen(fz_text_page *page)
{
	return 5;
}

void
render(char *filename, int pagenumber, int zoom, int rotation)
{
}

int main(int argc, char **argv)
{
	if(!setlocale(LC_CTYPE,"")){
		fprintf(stderr,"Can't set the specific locale!"
				"Check LANG,LC_CTYPE,LC_ALL.\n");
		return 1;
	}

	char *filename = argv[1];
	int pagenumber = argc > 2 ? atoi(argv[2]) : 1;
	int zoom = argc > 3 ? atoi(argv[3]) : 100;
	int rotation = argc > 4 ? atoi(argv[4]) : 0;

	render(filename, pagenumber, zoom, rotation);
	return 0;
}
