#pragma once

#include "Core.h"
#include <stdio.h>

class Bitmap;

// Load and store bitmaps in uncompressed MS Windows bitmap format 
class BmpFile
{
public:
	BmpFile(const char* filename);
	~BmpFile();

	Bitmap* Load();
	void Save(const Bitmap* bitmap);

private:
	int read(FILE* file, Bitmap* bitmap);
	int writeBitmap(FILE* file, const Bitmap* bitmap);

	char* m_path = nullptr;
};