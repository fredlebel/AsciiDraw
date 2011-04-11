#ifndef _fw_Rect_h_included_
#define _fw_Rect_h_included_

//#include <cmath>

namespace fw {

template <class T>
class Rect_t
{
public:
    Rect_t() 
        : x( 0 )
        , y( 0 )
        , width( 0 )
        , height( 0 )
    {
    }

    Rect_t( T tx, T ty, T twidth, T theight ) 
        : x( tx )
        , y( ty )
        , width( twidth )
        , height( theight )
    {
    }

    template <class Point>
    Rect_t( Point const& p1, Point const& p2 )
    {
        x = std::min( p1.x, p2.x );
        y = std::min( p1.y, p2.y );
        width = std::abs( p1.x - p2.x );
        height = std::abs( p1.y - p2.y );
    }

    void set( T tx, T ty, T twidth, T theight )
    {
        x = tx;
        y = ty;
        width = twidth;
        height = theight;
    }

    void offset( T dx, T dy )
    {
        x += dx;
        y += dy;
    }

    template <class Point>
    void offset( Point const& p )
    {
        x += p.x;
        y += p.y;
    }

    void moveTo( T tx, T ty )
    {
        x = tx;
        y = ty;
    }

    template <class Point>
    void moveTo( Point const& p )
    {
        x = p.x;
        y = p.y;
    }

    bool operator>( Rect_t const& r ) const
    {
        int myArea = width * height;
        int itsArea = r.width * r.height;
        return ( myArea > itsArea );
    }

    bool operator<( Rect_t const& r ) const
    {
        int myArea = width * height;
        int itsArea = r.width * r.height;
        return ( myArea < itsArea );
    }

    template <class Point>
    bool contains( Point const& p ) const
    {
        return ( (p.x >= x) && (p.x < x+width) &&
                 (p.y >= y) && (p.y < y+height) );
    }

    bool contains( Rect_t const& r ) const
    {
        return ( (r.x >= x) && (r.x+r.width  < x+width) &&
                 (r.y >= y) && (r.y+r.height < y+height) );
    }

    bool intersects( Rect_t const& r ) const
    {
        if ( y+height < r.y )   return false;
        if ( y > r.y+r.height ) return false;
        if ( x+width < r.x )    return false;
        if ( x > r.x+r.width )  return false;

        return true;
    }

    Rect_t getIntersection( Rect_t const& r ) const
    {
        if ( intersects( r ) )
        {
            // TODO:
            //return Rect_t(max(left, other.left), max(top, other.top),
            //    min(right, other.right), min(bottom, other.bottom));
        }

        return Rect_t();
    }

    T x;
    T y;
    T width;
    T height;
};

typedef Rect_t<int> IRect;
typedef Rect_t<double> DRect;

} // namespace fw

#endif // _fw_Rect_h_included_
