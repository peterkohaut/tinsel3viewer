#pragma once

#include <string>
#include <iostream>

#include "base.hpp"

using namespace std;

static void skip(istream &stream, size_t len)
{
	stream.seekg(len, ios_base::cur);
}

static u8 read_u8(istream &stream)
{
	u8 val;
	stream.read((char*)&val, sizeof(u8));
	return val;
}

static u16 read_u16(istream &stream)
{
	u16 val;
	stream.read((char*)&val, sizeof(u16));
	return val;
}

static u32 read_u32(istream &stream)
{
	u32 val;
	stream.read((char*)&val, sizeof(u32));
	return val;
}

static i32 read_i32(istream &stream)
{
	i32 val;
	stream.read((char*)&val, sizeof(i32));
	return val;
}

static string read_string(istream &stream, size_t len)
{
	char *buf = new char[len + 1];
	stream.read(buf, len);
	buf[len] = 0;
	string val(buf);
	delete[] buf;
	return val;
}
