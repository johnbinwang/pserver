/*
 * @author Johnbin Wang
 * @date 2013/1/14
 *
 */

#ifndef PdfConverterHandler_H
#define PdfConverterHandler_H

#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <cstring>
#include <ctime>
#include <string>
#include <limits>
#include <iostream>

#include "PdfConverter_types.h"
#include "PdfConverter.h"


class PdfConverterHandler : virtual public PdfConverterIf {

public:
	void convert(PdfJobResult& _return, const PdfJob& job);
};

#endif
