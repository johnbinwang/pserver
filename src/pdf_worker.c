#include "pdf_worker.h"

static char *output = NULL;
static float resolution = 72;
static int res_specified = 0;
static float rotation = 0;
static float scale = 1.0f;

static int showxml = 0;
static int showtext = 1;
static int showtime = 0;
static int showmd5 = 0;
static int showoutline = 0;
static int savealpha = 0;
static int uselist = 0;
static int alphabits = 8;
static float gamma_value = 1;
static int invert = 0;
static int width = 0;
static int height = 0;
static int fit = 0;
static int errored = 0;
static int ignore_errors = 0;

static fz_text_sheet *sheet = NULL;
static fz_colorspace *colorspace;
static int files = 0;
fz_output *out = NULL;


static struct {
	int count, total;
	int min, max;
	int minpage, maxpage;
	char *minfilename;
	char *maxfilename;
} timing;

struct pz_word {
	fz_rect rect;
	char *buf;	
};


void pz_print_reset_word(struct pz_word* word,FILE *f, int* has_data){
	if(word->buf && strlen(word->buf)>0 && !fz_is_empty_rect(&(word->rect)))
	{	
		*has_data = 1;
		fprintf(f,"[\"");
		char *p = word->buf;
		while(*p != '\0'){
			fprintf(f,"\\u%04x",(unsigned char)*p);
			p++;
		}
		fprintf(f,"\",%.3f,%.3f,%.3f,%.3f],\n",word->rect.x0,word->rect.y0,word->rect.x1,word->rect.y1);
		//fprintf(f,"%s\",%.3f,%.3f,%.3f,%.3f],\n",word->buf,word->rect.x0,word->rect.y0,word->rect.x1,word->rect.y1);
		word->rect = fz_empty_rect;
		free(word->buf);
		word->buf = NULL;
	}

}

void pz_array_add(char** buf,int c){  
    if (*buf == NULL) 
    {    
        *buf = (char *)malloc(ARRAY_SIZE * sizeof(char));  
        if(*buf){ 
            memset(*buf,0,ARRAY_SIZE); 
            **buf = c; 
        }else{ 
            printf("malloc error!"); 
        }    
    }else{ 
            char *p = *buf; 
            int count = 0; 
            while(*p){ 
                p++; 
                count ++; 
            }    
            if(count + 1 < ARRAY_SIZE) 
                *p = c; 
            else printf("string capacity is full!\n"); 
            p = NULL; 
    }    
 
}
void pz_union_word(struct pz_word *word,fz_text_span *span,int char_num,int c){
	if(word->buf == NULL)
		word->rect = fz_empty_rect;
	pz_array_add(&(word->buf),c);
	fz_rect bbox1 = fz_empty_rect;
	fz_text_char_bbox(&bbox1,span,char_num);
	//printf("debug: char: %c, [%.3f,%.3f,%.3f,%.3f]\n]",c,bbox1.x0,bbox1.y0,bbox1.x1,bbox1.y1);
	fz_union_rect(&(word->rect),&bbox1);
	//printf("word.rect: char: %s, [%.3f,%.3f,%.3f,%.3f]\n]",word.buf,word.rect.x0,word.rect.y0,word.rect.x1,word.rect.y1);
	//printf("union: word: %s, [%.3f,%.3f,%.3f,%.3f]\n",word.buf,bbox1.x0,bbox1.y0,bbox1.x1,bbox1.y1);
}

void
fz_print_text_page1(fz_context *ctx, fz_text_page *page,int page_num, FILE *f)
{
	fz_text_block *block;
	fz_text_line *line;
	fz_text_char *ch;
	char utf[10];
	int i, n, has_data;
	
	fprintf(f,"loadWords(%d,[\n",page_num);
	
	for (block = page->blocks; block < page->blocks + page->len; block++)
	{
		for (line = block->lines; line < block->lines + block->len; line++)
		{
			int span_num;
			struct pz_word word;
			word.rect = fz_empty_rect;
			word.buf = NULL;
			for (span_num = 0; span_num < line->len; span_num++)
			{
				fz_text_span *span = line->spans[span_num];
				int char_num;
				for (char_num = 0; char_num < span->len; char_num++)
				{
					ch = &span->text[char_num];
					fz_rect word_box;
					fz_text_char_bbox(&word_box, span, char_num);
					printf("\nword1: %lc\n word1 value: %d",ch->c,ch->c);
					if(ch->c <= 32)
					{
						//printf("before word.rect: char: %s, [%.3f,%.3f,%.3f,%.3f]\n]",word.buf,word.rect.x0,word.rect.y0,word.rect.x1,word.rect.y1);
						pz_union_word(&word,span,char_num,ch->c);

						//printf("after word.rect: char: %s, [%.3f,%.3f,%.3f,%.3f]\n]",word.buf,word.rect.x0,word.rect.y0,word.rect.x1,word.rect.y1);
						pz_print_reset_word(&word,f,&has_data);	
					}else if(ch->c <= 255){
						pz_union_word(&word,span,char_num,ch->c);
					}else{
						pz_print_reset_word(&word,f,&has_data);	
						wchar_t p = ch->c;
						printf("word value:%d\n",ch->c);
						printf("[\"\\u%04x\",%.3f,%.3f,%.3f,%.3f],\n",ch->c,word_box.x0,word_box.y0,word_box.x1,word_box.y1);
						printf("\n char: %lc ,value:%d [%.3f,%.3f,%.3f,%.3f]\n",p,ch->c,word_box.x0,word_box.y0,word_box.x1,word_box.y1);
						fprintf(f,"[\"\\u%04x\",%.3f,%.3f,%.3f,%.3f],\n",ch->c,word_box.x0,word_box.y0,word_box.x1,word_box.y1);
					}
				}
			}
			pz_print_reset_word(&word, f,&has_data);	
		}
	}
	if(has_data>0)
		fseek(f,-2,SEEK_CUR);
	fprintf(f,"]);\n");
}


static int gettime(void)
{
	static struct timeval first;
	static int once = 1;
	struct timeval now;
	if (once)
	{
		gettimeofday(&first, NULL);
		once = 0;
	}
	gettimeofday(&now, NULL);
	return (now.tv_sec - first.tv_sec) * 1000 + (now.tv_usec - first.tv_usec) / 1000;
}

static void drawpage(fz_context *ctx, fz_document *doc, int page_num, FILE *f)
{
	fz_page *page;
	fz_display_list *list = NULL;
	fz_device *dev = NULL;
	int start;
	fz_cookie cookie = { 0 };
	int needshot = 0;

	fz_var(list);
	fz_var(dev);

	if (showtime)
	{
		start = gettime();
	}

	fz_try(ctx)
	{
		page = fz_load_page(doc, page_num - 1);
	}
	fz_catch(ctx)
	{
		fz_throw(ctx, "cannot load page %d ", page_num);
	}

	if (uselist)
	{
		fz_try(ctx)
		{
			list = fz_new_display_list(ctx);
			dev = fz_new_list_device(ctx, list);
			fz_run_page(doc, page, dev, &fz_identity, &cookie);
		}
		fz_always(ctx)
		{
			fz_free_device(dev);
			dev = NULL;
		}
		fz_catch(ctx)
		{
			fz_free_display_list(ctx, list);
			fz_free_page(doc, page);
			fz_throw(ctx, "cannot draw page %d", page_num);
		}
	}

	if (showtext)
	{
		fz_text_page *text = NULL;

		fz_var(text);

		fz_try(ctx)
		{
			fz_rect bounds,bbox;
			fz_matrix transform;
			fz_pre_scale(&transform, 1.0f, 1.0f);
			fz_bound_page(doc, page, &bounds);
			fz_scale(&transform, scale, scale);
			fz_transform_rect(&bounds, &transform);
			
			text = fz_new_text_page(ctx, &bounds);
			dev = fz_new_text_device(ctx, sheet, text);
			if (list)
				fz_run_display_list(list, dev, &fz_identity, &fz_infinite_rect, &cookie);
			else{
				//fz_run_page(doc, page, dev, &fz_identity, &cookie);
				fz_run_page(doc, page, dev, &transform, &cookie);
			}
			fz_free_device(dev);
			dev = NULL;
			
			fz_print_text_page1(ctx, text,page_num,f);
		}
		fz_always(ctx)
		{
			fz_free_device(dev);
			dev = NULL;
			fz_free_text_page(ctx, text);
		}
		fz_catch(ctx)
		{
			fz_free_display_list(ctx, list);
			fz_free_page(doc, page);
			fz_rethrow(ctx);
		}
	}

	if (showmd5 || showtime)
		printf("page %d",  page_num);

	if (output || showmd5 || showtime)
	{
		float zoom;
		fz_matrix ctm;
		fz_rect bounds, tbounds;
		fz_irect ibounds;
		fz_pixmap *pix = NULL;
		int w, h;

		fz_var(pix);

		fz_bound_page(doc, page, &bounds);
		zoom = resolution / 72;
		fz_pre_scale(fz_rotate(&ctm, rotation), zoom, zoom);
		tbounds = bounds;
		fz_round_rect(&ibounds, fz_transform_rect(&tbounds, &ctm));

		/* Make local copies of our width/height */
		w = width;
		h = height;

		/* If a resolution is specified, check to see whether w/h are
		 * exceeded; if not, unset them. */
		if (res_specified)
		{
			int t;
			t = ibounds.x1 - ibounds.x0;
			if (w && t <= w)
				w = 0;
			t = ibounds.y1 - ibounds.y0;
			if (h && t <= h)
				h = 0;
		}

		// Now w or h will be 0 unless they need to be enforced. 
		if (w || h)
		{
			float scalex = w / (tbounds.x1 - tbounds.x0);
			float scaley = h / (tbounds.y1 - tbounds.y0);
			fz_matrix scale_mat;

			if (fit)
			{
				if (w == 0)
					scalex = 1.0f;
				if (h == 0)
					scaley = 1.0f;
			}
			else
			{
				if (w == 0)
					scalex = scaley;
				if (h == 0)
					scaley = scalex;
			}
			if (!fit)
			{
				if (scalex > scaley)
					scalex = scaley;
				else
					scaley = scalex;
			}
			fz_scale(&scale_mat, scalex, scaley);
			fz_concat(&ctm, &ctm, &scale_mat);
			tbounds = bounds;
			fz_transform_rect(&tbounds, &ctm);
		}
		fz_round_rect(&ibounds, &tbounds);
		fz_rect_from_irect(&tbounds, &ibounds);

		// TODO: banded rendering and multi-page ppm 

		fz_try(ctx)
		{
			pix = fz_new_pixmap_with_bbox(ctx, colorspace, &ibounds);

			if (savealpha)
				fz_clear_pixmap(ctx, pix);
			else
				fz_clear_pixmap_with_value(ctx, pix, 255);

			dev = fz_new_draw_device(ctx, pix);
			if (list)
				fz_run_display_list(list, dev, &ctm, &tbounds, &cookie);
			else
				fz_run_page(doc, page, dev, &ctm, &cookie);
			fz_free_device(dev);
			dev = NULL;

			if (invert)
				fz_invert_pixmap(ctx, pix);
			if (gamma_value != 1)
				fz_gamma_pixmap(ctx, pix, gamma_value);

			if (savealpha)
				fz_unmultiply_pixmap(ctx, pix);

			if (output)
			{
				char buf[512];
				sprintf(buf, output, page_num);
				if (strstr(output, ".pgm") || strstr(output, ".ppm") || strstr(output, ".pnm"))
					fz_write_pnm(ctx, pix, buf);
				else if (strstr(output, ".pam"))
					fz_write_pam(ctx, pix, buf, savealpha);
				else if (strstr(output, ".png"))
					fz_write_png(ctx, pix, buf, savealpha);
				else if (strstr(output, ".pbm")) {
					fz_bitmap *bit = fz_halftone_pixmap(ctx, pix, NULL);
					fz_write_pbm(ctx, bit, buf);
					fz_drop_bitmap(ctx, bit);
				}
			}

			if (showmd5)
			{
				unsigned char digest[16];
				int i;

				fz_md5_pixmap(pix, digest);
				printf(" ");
				for (i = 0; i < 16; i++)
					printf("%02x", digest[i]);
			}
		}
		fz_always(ctx)
		{
			fz_free_device(dev);
			dev = NULL;
			fz_drop_pixmap(ctx, pix);
		}
		fz_catch(ctx)
		{
			fz_free_display_list(ctx, list);
			fz_free_page(doc, page);
			fz_rethrow(ctx);
		}
	}

	if (list)
		fz_free_display_list(ctx, list);

	fz_free_page(doc, page);

	if (showtime)
	{
		int end = gettime();
		int diff = end - start;

		if (diff < timing.min)
		{
			timing.min = diff;
			timing.minpage = page_num;
		}
		if (diff > timing.max)
		{
			timing.max = diff;
			timing.maxpage = page_num;
		}
		timing.total += diff;
		timing.count ++;

		printf(" %dms", diff);
	}

	if (showmd5 || showtime)
		printf("\n");

	fz_flush_warnings(ctx);

	if (cookie.errors)
		errored = 1;
}


void
render_txt(fz_document *doc, fz_context *ctx,int page_num, FILE *f)
{
	char *password = "";
	int grayscale = 0;
	int c;

	if (!ctx || !doc)
	{
		fprintf(stderr, "cannot initialise context\n");
		//exit(1);
	}
	fz_set_aa_level(ctx, alphabits);
	colorspace = fz_device_rgb;
	
	sheet = fz_new_text_sheet(ctx);
	fz_try(ctx)
	{
		drawpage(ctx, doc, page_num, f);
	}
	fz_catch(ctx)
	{
	}
	fz_free_text_sheet(ctx, sheet);
}

int
render(const char* path, unsigned char *data, int len, int zoom, int rotation)
{
	int ret = 0;
	// Create a context to hold the exception stack and various caches.

	fz_context *ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);

	// Open the PDF, XPS or CBZ document.
	fz_stream * stream = fz_open_memory(ctx,data,len);
	fz_document *doc = fz_open_document_with_stream(ctx,"application/pdf",stream);

	// Retrieve the number of pages (not used in this example).

	int pagecount = fz_count_pages(doc);
	ret = pagecount;
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
	fz_device *draw_dev;//png dev 
	fz_text_page *text = NULL;
	fz_text_sheet * sheet = NULL;
	fz_rect *hit_bbox = NULL;
	const char    *str;
	int i=1,n= 0;
	const fz_rect fz_empty_rect = { 0, 0, 0, 0 };
	for(; i<=count; i++){
		printf("\n\npage %d :\n",i);
		char name[100] = "";
		sprintf(name,"%s/page_%d.js",path,i);
		FILE *f = fopen(name,"w+");
		fz_var(text);
		fz_try(ctx){
			page = fz_load_page(doc, i-1);
			// Take the page bounds and transform them by the same matrix that
			// we will use to render the page.
		
			fz_bound_page(doc, page, &bounds);
			printf("bounds w:%f,h:%f\n",bounds.x1,bounds.y1);

			//png render
			fz_round_rect(&bbox, &bounds);
			float width = bbox.x1-bbox.x0;
			float height = bbox.y1-bbox.y0;
			printf("before: width:%f,height:%f\n",width,height);
			scale = 720.00f/width;
			fz_scale(&transform, scale, scale);
			fz_round_rect(&bbox, fz_transform_rect(&bounds, &transform));
			width = bbox.x1-bbox.x0;
			height = bbox.y1-bbox.y0;
			printf("after: width:%f,height:%f\n",width,height);

			pix = fz_new_pixmap_with_bbox(ctx, fz_device_rgb, &bbox);
			fz_clear_pixmap_with_value(ctx, pix, 0xff);

			draw_dev = fz_new_draw_device(ctx, pix);
			fz_run_page(doc, page, draw_dev, &transform, NULL);

			// Save the pixmap to a file.
			char out[100];
			sprintf(out,"%s/page_%d.png",path,i);
			fz_write_png(ctx, pix, out, 0);
			fz_free_device(draw_dev);
			// Clean up.
			fz_drop_pixmap(ctx, pix);

			//text render
			render_txt(doc,ctx,i,f);
		}fz_always(ctx){
			fclose(f);
		}fz_catch(ctx){
			ret = 0;
		}
			
	}

	fz_close_document(doc);
	fz_free_context(ctx);

	return ret;
}

//int main(int argc, char **argv)
//{
//	if(!setlocale(LC_CTYPE,"")){
//		fprintf(stderr,"Can't set the specific locale!"
//				"Check LANG,LC_CTYPE,LC_ALL.\n");
//		return 1;
//	}
//
//	char *filename = argv[1];
//	int pagenumber = argc > 2 ? atoi(argv[2]) : 1;
//	int zoom = argc > 3 ? atoi(argv[3]) : 100;
//	int rotation = argc > 4 ? atoi(argv[4]) : 0;
//
//	render(filename, pagenumber, zoom, rotation);
//	return 0;
//}
