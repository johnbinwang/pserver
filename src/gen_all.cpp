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
	fz_text_block *block;
	fz_text_line *line;
	fz_text_span *span;
	int len = 0;
	for (block = page->blocks; block < page->blocks + page->len; block++)
	{
		for (line = block->lines; line < block->lines + block->len; line++)
		{
			for (span = line->spans; span < line->spans + line->len; span++)
				len += span->len;
			len++; /* pseudo-newline */
		}
	}
	return len;
}

void
render(char *filename, int pagenumber, int zoom, int rotation)
{
	// Create a context to hold the exception stack and various caches.

	fz_context *ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);

	// Open the PDF, XPS or CBZ document.

	fz_document *doc = fz_open_document(ctx, filename);

	// Retrieve the number of pages (not used in this example).

	int pagecount = fz_count_pages(doc);

	// Load the page we want. Page numbering starts from zero.
	int count = fz_count_pages(doc);
	printf("count: %d",count);
	fz_page *page;
	
	// Calculate a transform to use when rendering. This transform
	// contains the scale and rotation. Convert zoom percentage to a
	// scaling factor. Without scaling the resolution is 72 dpi.

	fz_matrix transform;
	fz_rotate(&transform, rotation);
	fz_pre_scale(&transform, zoom / 100.0f, zoom / 100.0f);

	fz_rect bounds;
	fz_irect bbox;
	fz_pixmap *pix; 
	fz_device *dev;//txt dev
	fz_device *draw_dev;//png dev 
	fz_text_page *text = NULL;
	fz_text_sheet * sheet = NULL;
	fz_rect *hit_bbox = NULL;
	const char    *str;
	int i=0,n= 0;
	const fz_rect fz_empty_rect = { 0, 0, 0, 0 };
	for(; i<count; i++){
		printf("\n\npage %d :\n",i);
		char name[10] = "";
		sprintf(name,"page_%d.js",i);
		FILE *f = fopen(name,"w+");
		fprintf(f,"var words=[\n");
		fz_var(sheet);
		fz_var(text);
		fz_var(dev);
		fz_try(ctx){
			page = fz_load_page(doc, i);
			// Take the page bounds and transform them by the same matrix that
			// we will use to render the page.
		
			fz_bound_page(doc, page, &bounds);
			fz_transform_rect(&bounds, &transform);

			//png render
			fz_round_rect(&bbox, &bounds);
			pix = fz_new_pixmap_with_bbox(ctx, fz_device_rgb, &bbox);
			fz_clear_pixmap_with_value(ctx, pix, 0xff);

			draw_dev = fz_new_draw_device(ctx, pix);
			fz_run_page(doc, page, draw_dev, &transform, NULL);

			// Save the pixmap to a file.
			char out[10];
			sprintf(out,"out_%d.png",i);
			fz_write_png(ctx, pix, out, 0);
			fz_free_device(draw_dev);
			// Clean up.
			fz_drop_pixmap(ctx, pix);

			//text render
			sheet = fz_new_text_sheet(ctx);
			text = fz_new_text_page(ctx,&bounds);
			dev = fz_new_text_device(ctx,sheet,text);
			fz_run_page(doc,page,dev,&transform,NULL);
			int len = textlen(text);
			//printf("\n len is %d \n",len);
			int b,l,s,c;
			for(b = 0; b< text->len; b++){
				fz_text_block *block = &text->blocks[b];
				//printf("\n block %d\n",b);
				for(l = 0; l < block->len; l++){
					fz_text_line *line = &block->lines[l];
					//printf("\n line %d\n",l); 
					char word[1000] = "";
					fz_rect  word_box = fz_empty_rect ;
					int word_len = 0;
					for(s = 0; s < line->len; s++){
						fz_text_span *span = &line->spans[s];
						//printf("\n span %d\n",s);
						for(c = 0; c< span->len; c++){
							fz_text_char *ch = &span->text[c];
							if( (c == span->len-1 && s == line->len-1) || ch->c <= 32  ){
								if(strlen(word)>0){
									//printf("\n word: %s [%.3f,%.3f,%.3f,%.3f]\n",word,word_box.x0,word_box.y0,word_box.x1,word_box.y1);
									fprintf(f,"[\"");
									char *p = word;
									while(*p != '\0'){
										//printf(",%d",(unsigned char)*p);
										fprintf(f,"\\u%04x",(unsigned char)*p);
										p++;
									}
									fprintf(f,"\",%.3f,%.3f,%.3f,%.3f],\n",word_box.x0,word_box.y0,word_box.x1,word_box.y1);
									memset(word,0,1000);
									word_box = fz_empty_rect;
								}
							}else if(ch->c <= 255){
								word[strlen(word)] = ch->c;
								fz_union_rect(&word_box,&ch->bbox);
							}else{
								wchar_t p = ch->c;
								//printf("\n char: %lc [%.3f,%.3f,%.3f,%.3f]\n",p,ch->bbox.x0,ch->bbox.y0,ch->bbox.x1,ch->bbox.y1);
								fprintf(f,"[\"\\u%04x\",%.3f,%.3f,%.3f,%.3f],\n",ch->c,ch->bbox.x0,ch->bbox.y0,ch->bbox.x1,ch->bbox.y1);
								memset(word,0,1000);
								word_box = fz_empty_rect;
							}
								
												}
					}
				}
			}
			fseek(f,-2,SEEK_CUR);
			fprintf(f,"];\nwordsLoaded();");
			fclose(f);
		}fz_always(ctx){
			fz_free_text_page(ctx, text);
			fz_free_text_sheet(ctx, sheet);
			fz_free_device(dev);
		}fz_catch(ctx){
	
		}
			
	}

	fz_close_document(doc);
	fz_free_context(ctx);

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
