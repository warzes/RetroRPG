#pragma once


#include "Core.h"

class Bitmap;

// Load and store bitmaps in png format 
class StbFile
{
public:
	StbFile(const char* filename);
	~StbFile();

	Bitmap* Load();

private:
	char* m_path = nullptr;
};