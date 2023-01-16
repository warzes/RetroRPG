#pragma once

#define  _CRT_SECURE_NO_WARNINGS // TODO: delete
#include "Bitmap.h"
#include <stdio.h>

// Load and store bitmaps in uncompressed MS Windows bitmap format 
class BmpFile
{
public:
	BmpFile(const char* filename);
	~BmpFile();

	Bitmap* Load();
	void Save(const Bitmap* bitmap);

private:
	int readBitmap(FILE* file, Bitmap* bitmap);
	int writeBitmap(FILE* file, const Bitmap* bitmap);

	char* m_path = nullptr;
};