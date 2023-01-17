#pragma once

#include <vector>
#include "MiniVector.h"

constexpr unsigned long LENBASE[29] = { 3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258 };
constexpr unsigned long LENEXTRA[29] = { 0,0,0,0,0,0,0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4,  4,  5,  5,  5,  5,  0 };
constexpr unsigned long DISTBASE[30] = { 1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577 };
constexpr unsigned long DISTEXTRA[30] = { 0,0,0,0,1,1,2, 2, 3, 3, 4, 4, 5, 5,  6,  6,  7,  7,  8,  8,   9,   9,  10,  10,  11,  11,  12,   12,   13,   13 };
constexpr unsigned long CLCL[19] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 }; //code length code lengths

struct Zlib //nested functions for zlib decompression
{
	static unsigned long readBitFromStream(size_t& bitp, const unsigned char* bits) { unsigned long result = (bits[bitp >> 3] >> (bitp & 0x7)) & 1; bitp++; return result; }
	static unsigned long readBitsFromStream(size_t& bitp, const unsigned char* bits, size_t nbits)
	{
		unsigned long result = 0;
		for( size_t i = 0; i < nbits; i++ ) result += (readBitFromStream(bitp, bits)) << i;
		return result;
	}
	struct HuffmanTree
	{
		int makeFromLengths(const std::vector<unsigned long>& bitlen, unsigned long maxbitlen)
		{ //make tree given the lengths
			unsigned long numcodes = (unsigned long)(bitlen.size()), treepos = 0, nodefilled = 0;
			std::vector<unsigned long> tree1d(numcodes), blcount(maxbitlen + 1, 0), nextcode(maxbitlen + 1, 0);
			for( unsigned long bits = 0; bits < numcodes; bits++ ) blcount[bitlen[bits]]++; //count number of instances of each code length
			for( unsigned long bits = 1; bits <= maxbitlen; bits++ ) nextcode[bits] = (nextcode[bits - 1] + blcount[bits - 1]) << 1;
			for( unsigned long n = 0; n < numcodes; n++ ) if( bitlen[n] != 0 ) tree1d[n] = nextcode[bitlen[n]]++; //generate all the codes
			tree2d.clear(); tree2d.resize(numcodes * 2, 32767); //32767 here means the tree2d isn't filled there yet
			for( unsigned long n = 0; n < numcodes; n++ ) //the codes
				for( unsigned long i = 0; i < bitlen[n]; i++ ) //the bits for this code
				{
					unsigned long bit = (tree1d[n] >> (bitlen[n] - i - 1)) & 1;
					if( treepos > numcodes - 2 ) return 55;
					if( tree2d[2 * treepos + bit] == 32767 ) //not yet filled in
					{
						if( i + 1 == bitlen[n] ) { tree2d[2 * treepos + bit] = n; treepos = 0; } //last bit
						else { tree2d[2 * treepos + bit] = ++nodefilled + numcodes; treepos = nodefilled; } //addresses are encoded as values > numcodes
					}
					else treepos = tree2d[2 * treepos + bit] - numcodes; //subtract numcodes from address to get address value
				}
			return 0;
		}
		int decode(bool& decoded, unsigned long& result, size_t& treepos, unsigned long bit) const
		{ //Decodes a symbol from the tree
			unsigned long numcodes = (unsigned long)tree2d.size() / 2;
			if( treepos >= numcodes ) return 11; //error: you appeared outside the codetree
			result = tree2d[2 * treepos + bit];
			decoded = (result < numcodes);
			treepos = decoded ? 0 : result - numcodes;
			return 0;
		}
		std::vector<unsigned long> tree2d; //2D representation of a huffman tree: The one dimension is "0" or "1", the other contains all nodes and leaves of the tree.
	};
	struct Inflator
	{
		int error;
		void inflate(std::vector<unsigned char>& out, const std::vector<unsigned char>& in, size_t inpos = 0)
		{
			size_t bp = 0, pos = 0; //bit pointer and byte pointer
			error = 0;
			unsigned long BFINAL = 0;
			while( !BFINAL && !error )
			{
				if( bp >> 3 >= in.size() ) { error = 52; return; } //error, bit pointer will jump past memory
				BFINAL = readBitFromStream(bp, &in[inpos]);
				unsigned long BTYPE = readBitFromStream(bp, &in[inpos]); BTYPE += 2 * readBitFromStream(bp, &in[inpos]);
				if( BTYPE == 3 ) { error = 20; return; } //error: invalid BTYPE
				else if( BTYPE == 0 ) inflateNoCompression(out, &in[inpos], bp, pos, in.size());
				else inflateHuffmanBlock(out, &in[inpos], bp, pos, in.size(), BTYPE);
			}
			if( !error ) out.resize(pos); //Only now we know the true size of out, resize it to that
		}
		void generateFixedTrees(HuffmanTree& tree, HuffmanTree& treeD) //get the tree of a deflated block with fixed tree
		{
			std::vector<unsigned long> bitlen(288, 8), bitlenD(32, 5);;
			for( size_t i = 144; i <= 255; i++ ) bitlen[i] = 9;
			for( size_t i = 256; i <= 279; i++ ) bitlen[i] = 7;
			tree.makeFromLengths(bitlen, 15);
			treeD.makeFromLengths(bitlenD, 15);
		}
		HuffmanTree codetree, codetreeD, codelengthcodetree; //the code tree for Huffman codes, dist codes, and code length codes
		unsigned long huffmanDecodeSymbol(const unsigned char* in, size_t& bp, const HuffmanTree& codetree, size_t inlength)
		{ //decode a single symbol from given list of bits with given code tree. return value is the symbol
			bool decoded; unsigned long ct;
			for( size_t treepos = 0;;)
			{
				if( (bp & 0x07) == 0 && (bp >> 3) > inlength ) { error = 10; return 0; } //error: end reached without endcode
				error = codetree.decode(decoded, ct, treepos, readBitFromStream(bp, in)); if( error ) return 0; //stop, an error happened
				if( decoded ) return ct;
			}
		}
		void getTreeInflateDynamic(HuffmanTree& tree, HuffmanTree& treeD, const unsigned char* in, size_t& bp, size_t inlength)
		{ //get the tree of a deflated block with dynamic tree, the tree itself is also Huffman compressed with a known tree
			std::vector<unsigned long> bitlen(288, 0), bitlenD(32, 0);
			if( bp >> 3 >= inlength - 2 ) { error = 49; return; } //the bit pointer is or will go past the memory
			size_t HLIT = readBitsFromStream(bp, in, 5) + 257; //number of literal/length codes + 257
			size_t HDIST = readBitsFromStream(bp, in, 5) + 1; //number of dist codes + 1
			size_t HCLEN = readBitsFromStream(bp, in, 4) + 4; //number of code length codes + 4
			std::vector<unsigned long> codelengthcode(19); //lengths of tree to decode the lengths of the dynamic tree
			for( size_t i = 0; i < 19; i++ ) codelengthcode[CLCL[i]] = (i < HCLEN) ? readBitsFromStream(bp, in, 3) : 0;
			error = codelengthcodetree.makeFromLengths(codelengthcode, 7); if( error ) return;
			size_t i = 0, replength;
			while( i < HLIT + HDIST )
			{
				unsigned long code = huffmanDecodeSymbol(in, bp, codelengthcodetree, inlength); if( error ) return;
				if( code <= 15 ) { if( i < HLIT ) bitlen[i++] = code; else bitlenD[i++ - HLIT] = code; } //a length code
				else if( code == 16 ) //repeat previous
				{
					if( bp >> 3 >= inlength ) { error = 50; return; } //error, bit pointer jumps past memory
					replength = 3 + readBitsFromStream(bp, in, 2);
					unsigned long value; //set value to the previous code
					if( (i - 1) < HLIT ) value = bitlen[i - 1];
					else value = bitlenD[i - HLIT - 1];
					for( size_t n = 0; n < replength; n++ ) //repeat this value in the next lengths
					{
						if( i >= HLIT + HDIST ) { error = 13; return; } //error: i is larger than the amount of codes
						if( i < HLIT ) bitlen[i++] = value; else bitlenD[i++ - HLIT] = value;
					}
				}
				else if( code == 17 ) //repeat "0" 3-10 times
				{
					if( bp >> 3 >= inlength ) { error = 50; return; } //error, bit pointer jumps past memory
					replength = 3 + readBitsFromStream(bp, in, 3);
					for( size_t n = 0; n < replength; n++ ) //repeat this value in the next lengths
					{
						if( i >= HLIT + HDIST ) { error = 14; return; } //error: i is larger than the amount of codes
						if( i < HLIT ) bitlen[i++] = 0; else bitlenD[i++ - HLIT] = 0;
					}
				}
				else if( code == 18 ) //repeat "0" 11-138 times
				{
					if( bp >> 3 >= inlength ) { error = 50; return; } //error, bit pointer jumps past memory
					replength = 11 + readBitsFromStream(bp, in, 7);
					for( size_t n = 0; n < replength; n++ ) //repeat this value in the next lengths
					{
						if( i >= HLIT + HDIST ) { error = 15; return; } //error: i is larger than the amount of codes
						if( i < HLIT ) bitlen[i++] = 0; else bitlenD[i++ - HLIT] = 0;
					}
				}
				else { error = 16; return; } //error: somehow an unexisting code appeared. This can never happen.
			}
			if( bitlen[256] == 0 ) { error = 64; return; } //the length of the end code 256 must be larger than 0
			error = tree.makeFromLengths(bitlen, 15); if( error ) return; //now we've finally got HLIT and HDIST, so generate the code trees, and the function is done
			error = treeD.makeFromLengths(bitlenD, 15); if( error ) return;
		}
		void inflateHuffmanBlock(std::vector<unsigned char>& out, const unsigned char* in, size_t& bp, size_t& pos, size_t inlength, unsigned long btype)
		{
			if( btype == 1 ) { generateFixedTrees(codetree, codetreeD); }
			else if( btype == 2 ) { getTreeInflateDynamic(codetree, codetreeD, in, bp, inlength); if( error ) return; }
			for( ;;)
			{
				unsigned long code = huffmanDecodeSymbol(in, bp, codetree, inlength); if( error ) return;
				if( code == 256 ) return; //end code
				else if( code <= 255 ) //literal symbol
				{
					if( pos >= out.size() ) out.resize((pos + 1) * 2); //reserve more room
					out[pos++] = (unsigned char)(code);
				}
				else if( code >= 257 && code <= 285 ) //length code
				{
					size_t length = LENBASE[code - 257], numextrabits = LENEXTRA[code - 257];
					if( (bp >> 3) >= inlength ) { error = 51; return; } //error, bit pointer will jump past memory
					length += readBitsFromStream(bp, in, numextrabits);
					unsigned long codeD = huffmanDecodeSymbol(in, bp, codetreeD, inlength); if( error ) return;
					if( codeD > 29 ) { error = 18; return; } //error: invalid dist code (30-31 are never used)
					unsigned long dist = DISTBASE[codeD], numextrabitsD = DISTEXTRA[codeD];
					if( (bp >> 3) >= inlength ) { error = 51; return; } //error, bit pointer will jump past memory
					dist += readBitsFromStream(bp, in, numextrabitsD);
					size_t start = pos, back = start - dist; //backwards
					if( pos + length >= out.size() ) out.resize((pos + length) * 2); //reserve more room
					for( size_t i = 0; i < length; i++ ) { out[pos++] = out[back++]; if( back >= start ) back = start - dist; }
				}
			}
		}
		void inflateNoCompression(std::vector<unsigned char>& out, const unsigned char* in, size_t& bp, size_t& pos, size_t inlength)
		{
			while( (bp & 0x7) != 0 ) bp++; //go to first boundary of byte
			size_t p = bp / 8;
			if( p >= inlength - 4 ) { error = 52; return; } //error, bit pointer will jump past memory
			unsigned long LEN = in[p] + 256 * in[p + 1], NLEN = in[p + 2] + 256 * in[p + 3]; p += 4;
			if( LEN + NLEN != 65535 ) { error = 21; return; } //error: NLEN is not one's complement of LEN
			if( pos + LEN >= out.size() ) out.resize(pos + LEN);
			if( p + LEN > inlength ) { error = 23; return; } //error: reading outside of in buffer
			for( unsigned long n = 0; n < LEN; n++ ) out[pos++] = in[p++]; //read LEN bytes of literal data
			bp = p * 8;
		}
	};
	int decompress(std::vector<unsigned char>& out, const std::vector<unsigned char>& in) //returns error value
	{
		Inflator inflator;
		if( in.size() < 2 ) { return 53; } //error, size of zlib data too small
		if( (in[0] * 256 + in[1]) % 31 != 0 ) { return 24; } //error: 256 * in[0] + in[1] must be a multiple of 31, the FCHECK value is supposed to be made that way
		unsigned long CM = in[0] & 15, CINFO = (in[0] >> 4) & 15, FDICT = (in[1] >> 5) & 1;
		if( CM != 8 || CINFO > 7 ) { return 25; } //error: only compression method 8: inflate with sliding window of 32k is supported by the PNG spec
		if( FDICT != 0 ) { return 26; } //error: the specification of PNG says about the zlib stream: "The additional flags shall not specify a preset dictionary."
		inflator.inflate(out, in, 2);
		return inflator.error; //note: adler32 checksum was skipped and ignored
	}
};