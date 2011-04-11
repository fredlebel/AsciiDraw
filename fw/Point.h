#ifndef _fw_Point_h_included_
#define _fw_Point_h_included_

#include <cmath>
#include <fw/StringUtils.h>

namespace fw {

template <class T>
class Point_t
{
public:
    Point_t() 
        : x( 0 )
        , y( 0 )
    {
    }

    Point_t( T tx, T ty ) 
        : x( tx )
        , y( ty )
    {
    }

    bool operator==( Point_t const& p ) const
    {
        return (x == p.x && y == p.y);
    }

    bool operator!=( Point_t const& p ) const
    {
        return (x != p.x || y != p.y);
    }

    Point_t& operator=( Point_t const& p )
    {
        x = p.x;
        y = p.y;
        return *this;
    }

    bool operator<( Point_t const& p ) const
    {
        // Y coordinate has priority
        if ( y < p.y )
            return true;
        else if ( y > p.y )
            return false;
        else if ( x < p.x )
            return true;
        else if ( x > p.x )
            return false;
        else
            return false;
    }

    void set( T tx, T ty )
    {
        x = tx;
        y = ty;
    }

    void offset( T dx, T dy )
    {
        x += dx;
        y += dy;
    }

    Point_t& operator+=( Point_t const& p )
    {
        x += p.x;
        y += p.y;
        return *this;
    }

    Point_t& operator-=( Point_t const& p )
    {
        x -= p.x;
        y -= p.y;
        return *this;
    }

    Point_t& operator*=( Point_t const& p )
    {
        x *= p.x;
        y *= p.y;
        return *this;
    }

    Point_t& operator/=( Point_t const& p )
    {
        x /= p.x;
        y /= p.y;
        return *this;
    }

    const Point_t operator+( Point_t const& p ) const
    {
        return Point_t( *this ) += p;
    }

    const Point_t operator-( Point_t const& p ) const
    {
        return Point_t( *this ) -= p;
    }

    const Point_t operator*( Point_t const& p ) const
    {
        return Point_t( *this ) *= p;
    }

    const Point_t operator/( Point_t const& p ) const
    {
        return Point_t( *this ) /= p;
    }

    template <class Unit>
    const Point_t operator*( Unit i ) const
    {
        return Point_t( *this ) *= Point_t( i, i );
    }

    template <class Unit>
    const Point_t operator*=( Unit i )
    {
        return (*this *= Point_t( i, i ) );
    }

    template <class Unit>
    const Point_t operator/( Unit i ) const
    {
        return Point_t( *this ) /= Point_t( i, i );
    }

    T length() const
    {
        return (T)std::sqrt( (double)x*x + y*y );
    }

    std::string str() const
    {
        return "(" + toString( x ) + "," + toString( y ) + ")";
    }

    void cap( int minX, int minY, int maxX, int maxY )
    {
        x = std::min( maxX, std::max( minX, x ) );
        y = std::min( maxY, std::max( minY, y ) );
    }

    T x;
    T y;
};

typedef Point_t<int> IPoint;
typedef Point_t<double> DPoint;

} // namespace fw

#endif // _fw_Point_h_included_
