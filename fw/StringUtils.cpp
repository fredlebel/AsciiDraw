#include <fw/StringUtils.h>
#include <sstream>
#include <iomanip>

std::string toHex( void* p )
{
    std::ostringstream oss;
    oss << std::uppercase 
        << "0x"
        << std::setfill( '0' ) 
        << std::setw( sizeof( p ) ) 
        << std::hex
        << p;
    return oss.str();
}

std::string toHex( int value, int width )
{
    std::ostringstream oss;
    oss << std::uppercase 
        << std::setfill( '0' ) 
        << std::setw( width ) 
        << std::hex
        << value;
    return oss.str();
}

std::wstring toHexW( int value, int width )
{
    std::wostringstream oss;
    oss << std::uppercase 
        << std::setfill( L'0' ) 
        << std::setw( width ) 
        << std::hex
        << value;
    return oss.str();
}

std::string toString( int value )
{
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

std::string toAscii( std::wstring const& str )
{
    std::string ret;
    ret.append( str.begin(), str.end() );
    return ret;
}

StringList tokenize( std::string const& str, std::string const& delimiter )
{
    using namespace std;

    StringList ret;
    if ( str.empty() )
        return ret;

    string::size_type tokenBegin = 0;
    string::size_type tokenEnd = 0;

    do
    {
        tokenEnd = str.find( delimiter, tokenBegin );
        ret.push_back( str.substr( tokenBegin, tokenEnd - tokenBegin ) );
        tokenBegin = tokenEnd + 1;
    } while ( tokenEnd != string::npos );

    return ret;
}

StringList splitString( std::string const& str, std::string const& delimiter )
{
    using namespace std;

    StringList ret;
    if ( str.empty() )
        return ret;

    string::size_type t = str.find( delimiter );
    if ( t != std::string::npos )
    {
        ret.push_back( str.substr( 0, t ) );
        ret.push_back( str.substr( t + 1 ) );
    }
    else
    {
        ret.push_back( str );
    }
    return ret;
}

bool parseString( std::string const& str, int& outVal )
{
    char* stopC;
    errno = 0;
    outVal = strtol( str.c_str(), &stopC, 10 );
    if ( *stopC != 0/* || errno == ERANGE */)
        return false;
    return true;
}

std::string trim( std::string const& str )
{
    std::string ret = str;
    std::string::size_type first = str.find_first_not_of( ' ' );
    std::string::size_type last = str.find_last_not_of( ' ' );
    if ( first != 0 || last != str.size() - 1 )
    {
        ret = str.substr( first, last - first + 1 );
    }
    return ret;
}

void trimOp( std::string& str )
{
    std::string::size_type first = str.find_first_not_of( ' ' );
    std::string::size_type last = str.find_last_not_of( ' ' );
    if ( first != 0 || last != str.size() - 1 )
        str = str.substr( first, last - first + 1 );
}

StringList wordWrap( std::string const& str, unsigned int width )
{
    StringList ret;
    StringList strs = tokenize( str, " " );

    std::string current;
    StringList::iterator it    = strs.begin();
    StringList::iterator itEnd = strs.end();
    for ( ; it != itEnd; ++it )
    {
        if ( current.size() + it->size() < width )
        {
            current += *it;
            if ( current.size() < width - 1 )
                current += " ";
        }
        else
        {
            ret.push_back( current );
            current = *it + " ";
        }
    }
    if ( !current.empty() )
        ret.push_back( current );
    return ret;
}

