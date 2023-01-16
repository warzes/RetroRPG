#include "Core.h"
#include <stdlib.h>
#include <string.h>
//-----------------------------------------------------------------------------
void Global::toLower(char* txt)
{
	int len = strlen(txt);
	for( int i = 0; i < len; i++ )
	{
		char c = txt[i];
		if( c >= 'A' && c <= 'Z' ) c += 'a' - 'A';
		txt[i] = c;
	}
}
//-----------------------------------------------------------------------------
void Global::toUpper(char* txt)
{
	int len = strlen(txt);
	for( int i = 0; i < len; i++ )
	{
		char c = txt[i];
		if( c >= 'a' && c <= 'z' ) c += 'A' - 'a';
		txt[i] = c;
	}
}
//-----------------------------------------------------------------------------
void Global::getFileExtention(char* ext, const int extSize, const char* path)
{
	ext[0] = '\0';
	int len = strlen(path);
	while( len )
	{
		if( path[--len] == '.' )
		{
			strncpy(ext, &path[len + 1], extSize - 1);
			ext[extSize - 1] = '\0';
			toLower(ext);
			return;
		}
	}
}
//-----------------------------------------------------------------------------
void Global::getFileName(char* name, const int nameSize, const char* path)
{
	name[0] = '\0';
	int len = strlen(path);
	while( len )
	{
		char c = path[--len];
		if( c == '\\' || c == '/' )
		{
			strncpy(name, &path[len + 1], nameSize - 1);
			name[nameSize - 1] = '\0';
			return;
		}
	}
	strncpy(name, path, nameSize - 1);
}
//-----------------------------------------------------------------------------
void Global::getFileDirectory(char* dir, int dirSize, const char* path)
{
	dir[0] = '\0';
	int len = strlen(path);
	while( len )
	{
		char c = path[--len];
		if( c == '/' || c == '\\' )
		{
			if( len > dirSize - 2 ) return;
			strncpy(dir, path, len + 1);
			dir[len + 1] = 0;
			return;
		}
	}
}
//-----------------------------------------------------------------------------
int Global::log2i32(int n)
{
	int r = 0;
	if( n >= 0x10000 ) { n >>= 16; r |= 0x10; }
	if( n >= 0x100 ) { n >>= 8; r |= 0x8; }
	if( n >= 0x10 ) { n >>= 4; r |= 0x4; }
	if( n >= 0x4 ) { n >>= 2;	r |= 0x2; }
	if( n >= 0x2 ) { r |= 0x1; }
	return r;
}
//-----------------------------------------------------------------------------
#ifdef _MSC_VER
extern "C" int __builtin_ffs(int x)
{
	unsigned long index;
	_BitScanForward(&index, x);
	return (int)index + 1;
}
#endif
//-----------------------------------------------------------------------------