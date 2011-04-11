#ifndef _fw_StringUtils_h_included_
#define _fw_StringUtils_h_included_
#pragma once

#include <string>
#include <vector>
#include <list>

std::string toHex( void* p );

std::string toHex( int value, int width );
std::wstring toHexW( int value, int width );

std::string toString( int value );

std::string toAscii( std::wstring const& str );

typedef std::list<std::string> StringList;
StringList tokenize( std::string const& str, std::string const& delimiter );

/// Splits the given string at the first delimiter and returns a list of two strings
StringList splitString( std::string const& str, std::string const& delimiter );

bool parseString( std::string const& str, int& outVal );

std::string trim( std::string const& str );
void trimOp( std::string& str );

StringList wordWrap( std::string const& str, unsigned int width );
#endif // _fw_StringUtils_h_included_

