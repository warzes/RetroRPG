#pragma once

#include "Core.h"
#include <stdio.h>
#include <vector>

class Bitmap;

// Load and store bitmaps in png format 
class PngFile
{
public:
	PngFile(const char* filename);
	~PngFile();

	Bitmap* Load();

private:
	bool loadFile(std::vector<unsigned char>& buffer, const char* fileName);

	char* m_path = nullptr;
};