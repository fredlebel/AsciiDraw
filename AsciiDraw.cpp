#define NOMINMAX
#undef MOUSE_MOVED
#include <curses.h>
#undef MOUSE_MOVED

#include <fw/Point.h>
#include <fw/Rect.h>
#include <string>
#include <list>
#include <map>
#include <algorithm>
#include <lua.hpp>

#define FGBG( fg, bg ) ( fg | (bg<<4) )

std::string exePath;

WINDOW* mainWindow = NULL;
short currentFG = 0;
short currentBG = 0;
chtype lastChar = 0;
fw::IPoint pos( 0, 0 );
fw::IPoint screenSize( 80, 50 );

int moveScriptCallback = LUA_REFNIL;

bool documentIsDirty = false;

// Highlight mode
bool hlMode = false;  // Set to true 
bool hlMouse = false; // Set to true when the mouse is highlighting
fw::IPoint hlStart;   // Position where the highlighting began


fw::IRect computeRect( fw::IPoint const& start, fw::IPoint const& end )
{
    fw::IRect r( start, end );

    if ( r.width  == 0 ) r.width  = 1; else r.width++;
    if ( r.height == 0 ) r.height = 1; else r.height++;
    return r;
}

void fillRect( WINDOW* win, fw::IRect const& r, int c )
{
    fw::IPoint oldCurPos( getcurx( win ), getcury( win ) );
    for ( int y = r.y; y < r.height + r.y; ++y )
        for ( int x = r.x; x < r.width + r.x; ++x )
           mvwaddrawch( win, y, x, c );
    wmove( win, oldCurPos.y, oldCurPos.x );
}

int promptForChar()
{
    savetty();
    curs_set( 2 );
    // Create the char prompt window
    WINDOW* promptWin = newwin( 18, 18, 5, 5 );
    keypad( promptWin, true );
    // Outline the window
    box( promptWin, ACS_VLINE, ACS_HLINE );
    // Fill with all ascii characters
    int c = 0;
    for ( int y = 0; y < 16; ++y )
        for ( int x = 0; x < 16; ++x )
            mvwaddrawch( promptWin, y+1, x+1, c++ );
    // Display the window
    wmove( promptWin, 1, 1 );
    wrefresh( promptWin );
    // Loop until a character is chosen or escape is pressed
    bool done = false;
    int ret = -1;
    fw::IRect clientRect( 1, 1, 16, 16 );
    while ( !done )
    {
        fw::IPoint p( getcurx( promptWin ), getcury( promptWin ) );

        c = wgetch( promptWin );
        switch ( c )
        {
        case KEY_UP:    p.offset(  0, -1 ); break;
        case KEY_DOWN:  p.offset(  0,  1 ); break;
        case KEY_LEFT:  p.offset( -1,  0 ); break;
        case KEY_RIGHT: p.offset(  1,  0 ); break;
        case KEY_A1: p.offset( -1, -1 ); break;
        case KEY_A2: p.offset(  0, -1 ); break;
        case KEY_A3: p.offset(  1, -1 ); break;
        case KEY_B1: p.offset( -1,  0 ); break;
        case KEY_B2: p.offset(  0,  0 ); break;
        case KEY_B3: p.offset(  1,  0 ); break;
        case KEY_C1: p.offset( -1,  1 ); break;
        case KEY_C2: p.offset(  0,  1 ); break;
        case KEY_C3: p.offset(  1,  1 ); break;

        case 13: // Enter
            {
                ret = (p.x-1) + (p.y-1) * 16;
                done = true;
            } break;
        case 27: // Escape
            {
                done = true;
            } break;
        case KEY_MOUSE:
            {
                MEVENT mEvent;
                int i = nc_getmouse( &mEvent );
                if ( mEvent.bstate & BUTTON1_CLICKED )
                {
                    fw::IPoint relPos( mEvent.x, mEvent.y );
                    if ( wmouse_trafo( promptWin, &relPos.y, &relPos.x, FALSE ) )
                    {
                        if ( clientRect.contains( relPos ) )
                        {
                            p = relPos;
                            ret = (p.x-1) + (p.y-1) * 16;
                            done = true;
                        }
                    }
                }
            } break;
        }
        p.cap( 1, 1, 16, 16 );
        wmove( promptWin, p.y, p.x );
        wrefresh( promptWin );
    }
    delwin( promptWin );
    resetty();
    return ret;
}

int promptForColor( chtype dispC )
{
    if ( dispC == 0 || dispC == 255 )
        dispC = '@';
    savetty();
    curs_set( 2 );
    // Create the char prompt window
    WINDOW* promptWin = newwin( 18, 18, 5, 5 );
    keypad( promptWin, true );
    // Outline the window
    box( promptWin, ACS_VLINE, ACS_HLINE );
    // Fill with all colours
    int c = 0;
    for ( int y = 0; y < 16; ++y )
        for ( int x = 0; x < 16; ++x )
        {
            wcolor_set( promptWin, c++, NULL );
            mvwaddrawch( promptWin, y+1, x+1, dispC & A_CHARTEXT );
        }
    // Display the window
    wmove( promptWin, 1, 1 );
    wrefresh( promptWin );
    // Loop until a character is chosen or escape is pressed
    bool done = false;
    int ret = -1;
    fw::IRect clientRect( 1, 1, 16, 16 );
    while ( !done )
    {
        fw::IPoint p( getcurx( promptWin ), getcury( promptWin ) );

        c = wgetch( promptWin );
        switch ( c )
        {
        case KEY_UP:    p.offset(  0, -1 ); break;
        case KEY_DOWN:  p.offset(  0,  1 ); break;
        case KEY_LEFT:  p.offset( -1,  0 ); break;
        case KEY_RIGHT: p.offset(  1,  0 ); break;
        case KEY_A1: p.offset( -1, -1 ); break;
        case KEY_A2: p.offset(  0, -1 ); break;
        case KEY_A3: p.offset(  1, -1 ); break;
        case KEY_B1: p.offset( -1,  0 ); break;
        case KEY_B2: p.offset(  0,  0 ); break;
        case KEY_B3: p.offset(  1,  0 ); break;
        case KEY_C1: p.offset( -1,  1 ); break;
        case KEY_C2: p.offset(  0,  1 ); break;
        case KEY_C3: p.offset(  1,  1 ); break;
        case 13: // Enter
            {
                ret = (p.x-1) + (p.y-1) * 16;
                done = true;
            } break;
        case 27: // Escape
            {
                done = true;
            } break;
        case KEY_MOUSE:
            {
                MEVENT mEvent;
                int i = nc_getmouse( &mEvent );
                if ( mEvent.bstate & BUTTON1_CLICKED )
                {
                    fw::IPoint relPos( mEvent.x, mEvent.y );
                    if ( wmouse_trafo( promptWin, &relPos.y, &relPos.x, FALSE ) )
                    {
                        if ( clientRect.contains( relPos ) )
                        {
                            p = relPos;
                            ret = (p.x-1) + (p.y-1) * 16;
                            done = true;
                        }
                    }
                }
            } break;
        }
        p.cap( 1, 1, 16, 16 );
        wmove( promptWin, p.y, p.x );
        wrefresh( promptWin );
    }
    delwin( promptWin );
    resetty();
    return ret;
}

std::string promptForFilename( std::string const& original )
{
    savetty();
    curs_set( 2 );
    echo();
    // Create the char prompt window
    WINDOW* promptWin = newwin( 3, 50, 5, 5 );
    keypad( promptWin, true );
    // Outline the window
    box( promptWin, ACS_VLINE, ACS_HLINE );
    // Display the window
    wmove( promptWin, 1, 1 );
    wrefresh( promptWin );

    char buff[80];
    memset( buff, 0, sizeof(buff) );
    mvwgetnstr( promptWin, 1, 1, buff, sizeof(buff) );

    delwin( promptWin );
    resetty();
    return buff;
}

void displayScriptingError( std::string const& str )
{
    savetty();
    curs_set( 0 );
    WINDOW* w = newwin( 3, COLS, 3, 0 );
    keypad( w, true );
    // Outline the window
    box( w, ACS_VLINE, ACS_HLINE );
    // Write the title
    wmove( w, 0, 2 );
    wprintw( w, "Scripting error" );
    // Write the error string
    wmove( w, 1, 1 );
    wprintw( w, "%s", str.c_str() );

    wrefresh( w );

    flushinp();
    getch();
    delwin( w );
    resetty();
}

void displayHelp()
{
    savetty();
    curs_set( 0 );
    WINDOW* helpWin = newwin( LINES-3, COLS, 3, 0 );
    keypad( helpWin, true );
    // Outline the window
    box( helpWin, ACS_VLINE, ACS_HLINE );
    int line = 1;
    // Display the window
    wmove( helpWin, line++, 1 ); wprintw( helpWin, "-- HELP - ASCIIDRAW [build #8] -------" );
    wmove( helpWin, line++, 1 ); 
    wmove( helpWin, line++, 1 ); wprintw( helpWin, "F1  : This help screen" );
    wmove( helpWin, line++, 1 ); wprintw( helpWin, "F2  : Pick ascii character from table (INS to draw)" );
    wmove( helpWin, line++, 1 ); wprintw( helpWin, "F3  : Pick color from table" );
    wmove( helpWin, line++, 1 ); wprintw( helpWin, "F4  : Apply current color" );
    wmove( helpWin, line++, 1 ); wprintw( helpWin, "F5  : Run a scripted command" );
    wmove( helpWin, line++, 1 ); wprintw( helpWin, "F6  : Reload the script file" );
    wmove( helpWin, line++, 1 ); wprintw( helpWin, "F7  : " );
    wmove( helpWin, line++, 1 ); wprintw( helpWin, "F8  : Show/hide status bar" );
    wmove( helpWin, line++, 1 ); wprintw( helpWin, "F9  : Copy as HTML to clipboard" );
    wmove( helpWin, line++, 1 ); wprintw( helpWin, "F10 : " );
    wmove( helpWin, line++, 1 ); wprintw( helpWin, "F11 : " );
    wmove( helpWin, line++, 1 ); wprintw( helpWin, "F12 : Exit" );
    wmove( helpWin, line++, 1 ); wprintw( helpWin, "--------------------------------------" );
    wmove( helpWin, line++, 1 ); wprintw( helpWin, "CTRL-INS : Select current character/copied block" );
    wmove( helpWin, line++, 1 ); wprintw( helpWin, "INS      : Insert current character or copied block" );
    wmove( helpWin, line++, 1 ); wprintw( helpWin, "           Fills a selection with the current character" );
    wmove( helpWin, line++, 1 ); wprintw( helpWin, "RIGHT-MOUSE: Same as INS" );
    wmove( helpWin, line++, 1 ); wprintw( helpWin, "ALT-INS : Paste from system clipboard" );
    wmove( helpWin, line++, 1 ); wprintw( helpWin, "--------------------------------------" );
    wmove( helpWin, line++, 1 ); wprintw( helpWin, "SHIFT-DIR  : Selection mode" );
    wmove( helpWin, line++, 1 ); wprintw( helpWin, "MOUSE-DRAG : Selection mode" );
    wmove( helpWin, line++, 1 ); wprintw( helpWin, "SHIFT-MOUSE-DRAG : Cut and drag selection" );
    wmove( helpWin, line++, 1 ); wprintw( helpWin, "CTRL-MOUSE-DRAG : Copy and drag selection" );
    wmove( helpWin, line++, 1 ); wprintw( helpWin, "CTRL-A : Select all" );
    wmove( helpWin, line++, 1 ); wprintw( helpWin, "CTRL-N : Start new image.  Doesn't clear the screen." );
    wmove( helpWin, line++, 1 ); wprintw( helpWin, "CTRL-Z : Undo" );
    wmove( helpWin, line++, 1 ); wprintw( helpWin, "CTRL-S : Save to file" );
    wmove( helpWin, line++, 1 ); wprintw( helpWin, "--------------------------------------" );
    wmove( helpWin, line++, 1 ); wprintw( helpWin, "CTRL-A, DEL : Clear the screen" );


    wrefresh( helpWin );

    flushinp();
    getch();
    delwin( helpWin );
    resetty();
}

fw::IRect getWindowRect( WINDOW* w )
{
    fw::IRect ret;
    ret.set( 0, 0, getmaxx( w ), getmaxy( w ) );
    ret.moveTo( getbegx( w ), getbegy( w ) );
    return ret;
}

#define MAX_UNDO_COUNT  200

typedef std::pair<WINDOW*, fw::IPoint> UndoState;
typedef std::list<UndoState> UndoList;
UndoList undoList;

void pushUndoState()
{
    documentIsDirty = true;
    WINDOW* w = newwin( LINES, COLS, 0, 0 );
    copywin( mainWindow, w, 0, 0, 0, 0, LINES-1, COLS-1, false );
    undoList.push_back( std::make_pair( w, pos ) );

    if ( undoList.size() > MAX_UNDO_COUNT )
    {
        WINDOW* w = undoList.front().first;
        undoList.pop_front();
        delwin( w );
    }
}

void popUndoState()
{
    if ( undoList.empty() )
        return;
    UndoState s = undoList.back();
    undoList.pop_back();
    copywin( s.first, mainWindow, 0, 0, 0, 0, LINES-1, COLS-1, false );
    pos = s.second;
    wmove( mainWindow, pos.y, pos.x );
    delwin( s.first );
    hlMode = false;
}

// To deal with scripted commands
typedef std::map<std::string, int> CommandMap;
CommandMap gCommands;

std::string promptForScriptName()
{
    if ( gCommands.empty() )
    {
        savetty();
        curs_set( 0 );
        WINDOW* w = newwin( 3, COLS, 3, 0 );
        keypad( w, true );
        // Outline the window
        box( w, ACS_VLINE, ACS_HLINE );
        // Write the title
        wmove( w, 0, 2 );
        wprintw( w, "No scripting command found" );

        wrefresh( w );

        flushinp();
        getch();
        delwin( w );
        resetty();
        return "";
    }
    else
    {
        savetty();
        // Maximum of 40 commands
        int windowHeight = std::min( (int)gCommands.size(), 42 );
        // Create the prompt window
        WINDOW* w = newwin( windowHeight+2, 50, 4, 5 );
        // Outline the window
        unsigned int selectedLine = 1;
        std::string selectedCommand;
        bool done = false;
        while ( !done )
        {
            wcolor_set( w, FGBG( 0, 0 ), NULL );
            wclear( w );
            box( w, ACS_VLINE, ACS_HLINE );
            wmove( w, 0, 2 );
            wprintw( w, "Choose script command" );
            // Display the options
            int line = 1;
            CommandMap::iterator it = gCommands.begin();
            for ( ; it != gCommands.end(); ++it, ++line )
            {
                if ( line == selectedLine )
                    wcolor_set( w, FGBG( 0, 7 ), NULL );
                else
                    wcolor_set( w, FGBG( 0, 0 ), NULL );
                wmove( w, line, 1 );
                wprintw( w, "%s", it->first.c_str() );
            }
            wmove( w, 1, 1 );
            wrefresh( w );

            int c = getch();
            switch ( c )
            {
            // Movement keys
            case KEY_A2:
            case KEY_UP:
                selectedLine -= 1;
                break;
            case KEY_DOWN:
            case KEY_C2:
                selectedLine += 1;
                break;
            case '\r':
            case '\n':
                {
                    CommandMap::iterator it = gCommands.begin();
                    std::advance( it, selectedLine - 1 );
                    selectedCommand = it->first;
                    done = true;
                } break;
            case 27: // ESC
                done = true;
                break;
            }

            if ( selectedLine < 1 )
                selectedLine = 1;
            else if ( selectedLine > gCommands.size() )
                selectedLine = (unsigned int)gCommands.size();
        }

        delwin( w );
        resetty();
        return selectedCommand;
    }
}

void runCommand( lua_State* L, std::string const& str )
{
    CommandMap::const_iterator it = gCommands.find( str );
    if ( it != gCommands.end() )
    {
        // Execute the command.
        lua_rawgeti( L, LUA_REGISTRYINDEX, it->second );
        int ret = lua_pcall( L, 0, 0, 0 );
        if ( ret == LUA_ERRRUN )
        {
            const char* errorStr = lua_tolstring( L, -1, NULL );
            displayScriptingError( errorStr );
        }
    }
}

int l_DeclareCommand( lua_State* L )
{
    luaL_checktype( L, -1, LUA_TTABLE );
    // Get the attribute name
    lua_getfield( L, -1, "name" );
    luaL_checktype( L, -1, LUA_TSTRING );
    const char* commandName = lua_tolstring( L, -1, NULL );
    lua_pop( L, 1 );
    // Get the function
    lua_getfield( L, -1, "op" );
    luaL_checktype( L, -1, LUA_TFUNCTION );
    int key = luaL_ref( L, LUA_REGISTRYINDEX );

    luaL_checktype( L, -1, LUA_TTABLE );
    gCommands.insert( std::make_pair( commandName, key ) );
    return 0;
}

int l_SetFG( lua_State* L )
{
    luaL_checktype( L, 1, LUA_TNUMBER );
    currentFG = (int)lua_tointeger( L, 1 ) & 0x0000000F;
    wcolor_set( mainWindow, FGBG( currentFG, currentBG ), NULL );
    return 0;
}

int l_SetBG( lua_State* L )
{
    luaL_checktype( L, 1, LUA_TNUMBER );
    currentBG = (int)lua_tointeger( L, 1 ) & 0x0000000F;
    wcolor_set( mainWindow, FGBG( currentFG, currentBG ), NULL );
    return 0;
}

int l_SetChar( lua_State* L )
{
    int t = lua_type( L, 1 );
    switch ( t )
    {
    case LUA_TSTRING:
        {
            const char* str = lua_tolstring( L, 1, NULL );
            lastChar = str[0];
        } break;
    case LUA_TNUMBER:
        {
            lastChar = (int)lua_tointeger( L, 1 ) & 0x000000FF;
        } break;
    default:
        return 0;
    }
    return 0;
}

int l_GetChar( lua_State* L )
{
    lua_pushinteger( L, lastChar & 0xff );
    lua_pushinteger( L, currentFG );
    lua_pushinteger( L, currentBG );
    return 3;
}

int l_Select( lua_State* L )
{
    luaL_checktype( L, 1, LUA_TNUMBER );
    luaL_checktype( L, 2, LUA_TNUMBER );
    luaL_checktype( L, 3, LUA_TNUMBER );
    luaL_checktype( L, 4, LUA_TNUMBER );
    hlMode = true;
    hlStart.set( (int)lua_tointeger( L, 1 ), (int)lua_tointeger( L, 2 ) );
    pos.set( (int)lua_tointeger( L, 3 ), (int)lua_tointeger( L, 4 ) );
    wmove( mainWindow, pos.y, pos.x );
    return 0;
}

int l_Unselect( lua_State* L )
{
    hlMode = false;
    return 0;
}

int l_HasSelection( lua_State* L )
{
    lua_pushboolean( L, hlMode );
    return 1;
}

int l_GetSelection( lua_State* L )
{
    fw::IRect hlRect = computeRect( hlStart, pos );
    lua_pushinteger( L, hlRect.x );
    lua_pushinteger( L, hlRect.y );
    lua_pushinteger( L, hlRect.x + hlRect.width - 1 );
    lua_pushinteger( L, hlRect.y + hlRect.height - 1 );
    return 4;
}

int l_Move( lua_State* L )
{
    if ( hlMode )
        return 0; // Can't move in highlighting mode
    luaL_checktype( L, 1, LUA_TNUMBER );
    luaL_checktype( L, 2, LUA_TNUMBER );
    pos.set( (int)lua_tointeger( L, 1 ), (int)lua_tointeger( L, 2 ) );
    wmove( mainWindow, pos.y, pos.x );
    return 0;
}

int l_GetPos( lua_State* L )
{
    lua_pushinteger( L, pos.x );
    lua_pushinteger( L, pos.y );
    return 2;
}

int l_Set( lua_State* L )
{
    if ( hlMode )
    {
        // Fill the selection with the character
        fw::IRect hlRect = computeRect( hlStart, pos );
        fillRect( mainWindow, hlRect, lastChar );
    }
    else
    {
        waddrawch( mainWindow, lastChar );
        wmove( mainWindow, pos.y, pos.x );
    }

    return 0;
}

int l_Get( lua_State* L )
{
    chtype c = winch( mainWindow );
    short fg, bg;
    pair_content( (short)PAIR_NUMBER( c ), &fg, &bg );
    c &= A_CHARTEXT;

    lua_pushinteger( L, c );
    lua_pushinteger( L, fg );
    lua_pushinteger( L, bg );
    return 3;
}

int l_PushUndoState( lua_State* L )
{
    pushUndoState();
    return 0;
}

int l_PopUndoState( lua_State* L )
{
    popUndoState();
    return 0;
}

int l_Random( lua_State* L )
{
    lua_pushinteger( L, rand() );
    return 1;
}

int l_SetMoveCB( lua_State* L )
{
    luaL_checktype( L, 1, LUA_TFUNCTION );
    if ( moveScriptCallback != LUA_REFNIL )
        luaL_unref( L, LUA_REGISTRYINDEX, moveScriptCallback );
    moveScriptCallback = luaL_ref( L, LUA_REGISTRYINDEX );
    return 0;
}

lua_State* initializeScripting()
{
    gCommands.clear(); // In case something is left inside

    lua_State* L = lua_open();

    luaopen_base( L );
    luaopen_table( L );
    //luaopen_io( L );
    //luaopen_os( L );
    luaopen_string( L );
    luaopen_math( L );
    //luaopen_debug( L );
    //luaopen_package( L );

    lua_pushinteger( L,  0 ); lua_setglobal( L, "BLACK" );
    lua_pushinteger( L,  1 ); lua_setglobal( L, "DARKBLUE" );
    lua_pushinteger( L,  2 ); lua_setglobal( L, "DARKGREEN" );
    lua_pushinteger( L,  3 ); lua_setglobal( L, "DARKCYAN" );
    lua_pushinteger( L,  4 ); lua_setglobal( L, "DARKRED" );
    lua_pushinteger( L,  5 ); lua_setglobal( L, "DARKPURPLE" );
    lua_pushinteger( L,  6 ); lua_setglobal( L, "BROWN" );
    lua_pushinteger( L,  7 ); lua_setglobal( L, "LIGHTGREY" );
    lua_pushinteger( L,  8 ); lua_setglobal( L, "DARKGREY" );
    lua_pushinteger( L,  9 ); lua_setglobal( L, "LIGHTBLUE" );
    lua_pushinteger( L, 10 ); lua_setglobal( L, "LIGHTGREEN" );
    lua_pushinteger( L, 11 ); lua_setglobal( L, "LIGHTCYAN" );
    lua_pushinteger( L, 12 ); lua_setglobal( L, "LIGHTRED" );
    lua_pushinteger( L, 13 ); lua_setglobal( L, "LIGHTPURPLE" );
    lua_pushinteger( L, 14 ); lua_setglobal( L, "YELLOW" );
    lua_pushinteger( L, 15 ); lua_setglobal( L, "WHITE" );
    lua_pushcfunction( L, l_DeclareCommand );       lua_setglobal( L, "DeclareCommand" );
    lua_pushcfunction( L, l_SetFG );                lua_setglobal( L, "SetFG" );
    lua_pushcfunction( L, l_SetBG );                lua_setglobal( L, "SetBG" );
    lua_pushcfunction( L, l_SetChar );              lua_setglobal( L, "SetChar" );
    lua_pushcfunction( L, l_GetChar );              lua_setglobal( L, "GetChar" );
    lua_pushcfunction( L, l_Move );                 lua_setglobal( L, "Move" );
    lua_pushcfunction( L, l_GetPos );               lua_setglobal( L, "GetPos" );
    lua_pushcfunction( L, l_Set );                  lua_setglobal( L, "Set" );
    lua_pushcfunction( L, l_Get );                  lua_setglobal( L, "Get" );
    lua_pushcfunction( L, l_PushUndoState );        lua_setglobal( L, "PushUndoState" );
    lua_pushcfunction( L, l_PopUndoState );         lua_setglobal( L, "PopUndoState" );

    lua_pushcfunction( L, l_Select );               lua_setglobal( L, "Select" );
    lua_pushcfunction( L, l_Unselect );             lua_setglobal( L, "Unselect" );
    lua_pushcfunction( L, l_GetSelection );         lua_setglobal( L, "GetSelection" );
    lua_pushcfunction( L, l_HasSelection );         lua_setglobal( L, "HasSelection" );

    lua_pushcfunction( L, l_Random );               lua_setglobal( L, "Random" );
    lua_pushcfunction( L, l_SetMoveCB );            lua_setglobal( L, "SetMoveCB" );

#ifdef _DEBUG
    switch ( luaL_loadfile( L, "asciidraw.lua" ) )
#else
    switch ( luaL_loadfile( L, (exePath + "asciidraw.lua").c_str() ) )
#endif
    {
    case 0: lua_pcall( L, 0, 0, 0 ); break;
    case LUA_ERRFILE:
    case LUA_ERRSYNTAX:
        {
            displayScriptingError( lua_tolstring( L, -1, NULL ) );
        } break;
    }

    return L;
}

void cursorMoveHelper( int dx, int dy, lua_State* L )
{
    pos.offset( dx, dy );
    wmove( mainWindow, pos.y, pos.x );
    hlMode = false;
    if ( moveScriptCallback != LUA_REFNIL )
    {
        // Execute the command.
        lua_rawgeti( L, LUA_REGISTRYINDEX, moveScriptCallback );
        int ret = lua_pcall( L, 0, 0, 0 );
        if ( ret == LUA_ERRRUN )
        {
            const char* errorStr = lua_tolstring( L, -1, NULL );
            displayScriptingError( errorStr );
        }
    }
}

int main( int argc, char** argv )
{
    exePath = argv[0];
    exePath.erase( exePath.find_last_of( "/\\" ) + 1 );
    mainWindow = initscr();

    WINDOW* hlWindow = NULL;

    bool showBar = true;
    WINDOW* infoBar = newwin( 3, COLS, 0, 0 );
    WINDOW* copyBuffer = NULL;

    bool isDragging = false; // Set to true when dragging a selection
    WINDOW* dragWindow = NULL;
    fw::IPoint dragOffset;

    resize_term( screenSize.y, screenSize.x );

    {
        start_color();
        for ( short pair = 0; pair < COLOR_PAIRS; ++pair )
            init_pair( pair, pair%COLORS, pair/COLORS );

#if 0
        init_color( 0x0,    0,    0,    0 );
        init_color( 0x1,    0,    0,  750 );
        init_color( 0x2,    0,  750,    0 );
        init_color( 0x3,    0,  750,  750 );
        init_color( 0x4,  750,    0,    0 );
        init_color( 0x5,  750,    0,  750 );
        init_color( 0x6,  500,  500,    0 );
        init_color( 0x7,  666,  666,  666 );

        init_color( 0x8,  333,  333,  333 );
        init_color( 0x9,  333,  333, 1000 );
        init_color( 0xa,  333, 1000,  333 );
        init_color( 0xb,  333, 1000, 1000 );
        init_color( 0xc, 1000,  333,  333 );
        init_color( 0xd, 1000,  333, 1000 );
        init_color( 0xe, 1000, 1000,    0 );
        init_color( 0xf, 1000, 1000, 1000 );
#endif
    }

    raw();
    noecho(); // Don't echo keyboard chars
    cbreak(); // Get keyboard events immediately, don't wait for cr
    //nodelay( mainWindow_, TRUE ); // Polling key mode
    nodelay( mainWindow, FALSE );
    keypad( mainWindow, true );
    //PDC_return_key_modifiers( true );
    wmove( mainWindow, LINES/2, COLS/2 );

    wcolor_set( mainWindow, FGBG( currentFG, currentBG ), NULL );
    curs_set( 2 );

    mousemask( ALL_MOUSE_EVENTS, NULL );

    lua_State *L = initializeScripting();

    // Read the filename from the command line
    std::string filename;
    if ( argc == 2 )
    {
        filename = argv[1];
        scr_restore( filename.c_str() );
        refresh();
        copywin( curscr, mainWindow, 0, 0, 0, 0, LINES-1, COLS-1, false );
    }

    bool mustExit = false;
    while ( !mustExit )
    {
        pos.set( getcurx( mainWindow ), getcury( mainWindow ) );

        // Update the information in the info bar.
        if ( showBar )
        {
            wclear( infoBar );
            box( infoBar, 0, 0 );
            wmove( infoBar, 1, 1 );
            wcolor_set( infoBar, 0, NULL );

            if ( moveScriptCallback != LUA_REFNIL )
            {
                wprintw( infoBar, "{moveCB}" );
            }

            if ( copyBuffer != NULL )
            {
                wprintw( infoBar, "[block copied]" );
            }
            else
            {
                wprintw( infoBar, "[" );
                wcolor_set( infoBar, FGBG( currentFG, currentBG ), NULL );
                waddrawch( infoBar, lastChar );
                wcolor_set( infoBar, 0, NULL );
                wprintw( infoBar, "] -> %i", lastChar & 0xff );
            }
            wprintw( infoBar, "   FILENAME:%s", filename.c_str() );
            if ( documentIsDirty )
            {
                wprintw( infoBar, " *" );
            }
        }

        // Create the highlight window
        if ( hlMode && hlWindow == NULL )
        {
            hlWindow = newwin( 1, 1, 1, 1 );
            curs_set( 0 );
        }
        
        if ( isDragging )
        {
            curs_set( 0 );
        }
        else if ( dragWindow != NULL )
        {
            delwin( dragWindow );
            dragWindow = NULL;
            // TODO: Copy dragWindow to mainWindow
        }

        if ( !hlMode && hlWindow != NULL )
        {
            delwin( hlWindow );
            hlWindow = NULL;
        }

        if ( !hlMode && !isDragging )
        {
            curs_set( 2 );
        }

        if ( hlWindow != NULL ) // Resize and refresh hlWindow's content
        {
            fw::IRect hlRect = computeRect( hlStart, pos );

            wresize( hlWindow, hlRect.height, hlRect.width );
            mvwin( hlWindow, hlRect.y, hlRect.x );
            wresize( hlWindow, hlRect.height, hlRect.width );
            // Copy from mainWindow to hlWindow
            wclear( hlWindow );
            copywin( mainWindow, hlWindow, hlRect.y, hlRect.x, 0, 0, hlRect.height-1, hlRect.width-1, false );
            for ( int y = 0; y < hlRect.height; ++y )
                for ( int x = 0; x < hlRect.width; ++x )
                {
                    int charVal = mvwinch( hlWindow, y, x );
                    charVal |= A_REVERSE;
                    mvwaddrawch( hlWindow, y, x, charVal );
                }
            //box( hlWindow, ACS_VLINE, ACS_HLINE );
        }

        touchwin( mainWindow );
        touchwin( infoBar );
        if ( hlWindow != NULL )
            touchwin( hlWindow );
        wnoutrefresh( mainWindow );
        if ( hlMode )
            wnoutrefresh( hlWindow );
        if ( showBar )
            wnoutrefresh( infoBar );
        if ( dragWindow != NULL )
        {
            touchwin( dragWindow );
            wnoutrefresh( dragWindow );
        }
        doupdate();


        wcolor_set( mainWindow, FGBG( currentFG, currentBG ), NULL );
        flushinp();

        // Get the current mouse position
        // We need to catch when the mouse moves
        fw::IPoint oldMousePos;
        {
            request_mouse_pos();
            oldMousePos.set( MOUSE_X_POS, MOUSE_Y_POS );
        }
        int c;
        if ( hlMouse || isDragging )
            nodelay( mainWindow, TRUE );

        while ( (c = getch()) == ERR )
        {
            // When the mouse is dragging, we check if it moves
            if ( hlMouse || isDragging )
            {
                fw::IPoint newMousePos;
                request_mouse_pos();
                newMousePos.set( MOUSE_X_POS, MOUSE_Y_POS );
                // Positions differ, the mouse moved.
                if ( newMousePos != oldMousePos )
                {
                    if ( isDragging )
                    {
                        fw::IRect dragRect = getWindowRect( dragWindow );
                        dragRect.moveTo( newMousePos.x - dragOffset.x, newMousePos.y - dragOffset.y );
                        mvwin( dragWindow, dragRect.y, dragRect.x );
                        wmove( mainWindow, dragRect.y + dragRect.height, dragRect.x + dragRect.width );
                    }
                    else
                    {
                        wmove( mainWindow, newMousePos.y, newMousePos.x );
                    }
                    break;
                }
            }
        }
        nodelay( mainWindow, FALSE );

        if ( c >= 0 && c <= 255 )
        {
            switch ( c )
            {
            // Special cases that we want the control switch to handle
            case KEY_DC:
            case 1: // CTRL-A
            case 8: // Backspace
            case 14: // CTRL-N
            case 19: // CTRL-S
            case 26: // CTRL-Z
            case 27: // ESC
            case '\r':
            case '\n':
//            case 8/*bkspc*/:
                break;
            default:
                pushUndoState();
                if ( hlMode )
                {
                    // Fill the selection with the character
                    fw::IRect hlRect = computeRect( hlStart, pos );
                    fillRect( mainWindow, hlRect, c );
                }
                else
                {
                    waddrawch( mainWindow, c );
                }
                lastChar = c;
                continue;
            }
        }

        // Control switch for non character
        switch ( c )
        {
        //case SDLK_ESCAPE: k = CursesUI::Key_Esc; break;
        // Movement keys
        case KEY_UP:    cursorMoveHelper(  0, -1, L ); break;
        case KEY_DOWN:  cursorMoveHelper(  0,  1, L ); break;
        case KEY_LEFT:  cursorMoveHelper( -1,  0, L ); break;
        case KEY_RIGHT: cursorMoveHelper(  1,  0, L ); break;

        case KEY_A1:    cursorMoveHelper( -1, -1, L ); break;
        case KEY_A2:    cursorMoveHelper(  0, -1, L ); break;
        case KEY_A3:    cursorMoveHelper(  1, -1, L ); break;
        case KEY_B1:    cursorMoveHelper( -1,  0, L ); break;
        case KEY_B2:    cursorMoveHelper(  0,  0, L ); break;
        case KEY_B3:    cursorMoveHelper(  1,  0, L ); break;
        case KEY_C1:    cursorMoveHelper( -1,  1, L ); break;
        case KEY_C2:    cursorMoveHelper(  0,  1, L ); break;
        case KEY_C3:    cursorMoveHelper(  1,  1, L ); break;

        case CTL_LEFT:  currentFG += COLORS - 1; currentFG %= COLORS; break;
        case CTL_RIGHT: currentFG += COLORS + 1; currentFG %= COLORS; break;
        case CTL_UP:    currentBG += COLORS + 1; currentBG %= COLORS; break;
        case CTL_DOWN:  currentBG += COLORS - 1; currentBG %= COLORS; break;

        case KEY_DC: // Delete
            {
                pushUndoState();
                wcolor_set( mainWindow, 0, NULL );
                if ( hlMode )
                {
                    fw::IRect hlRect = computeRect( hlStart, pos );
                    fillRect( mainWindow, hlRect, ' ' );
                }
                else
                {
                    waddrawch( mainWindow, ' ' );
                }
            } break;
        case 8: // Backspace
            {
                pushUndoState();
                wcolor_set( mainWindow, 0, NULL );
                if ( hlMode )
                {
                    fw::IRect hlRect = computeRect( hlStart, pos );
                    fillRect( mainWindow, hlRect, ' ' );
                }
                else
                {
                    wmove( mainWindow, pos.y, pos.x - 1 );
                    waddrawch( mainWindow, ' ' );
                    wmove( mainWindow, pos.y, pos.x - 1 );
                }
            } break;

        case KEY_IC: // Insert
        //case KEY_SIC: // shift-insert
            {
                if ( copyBuffer != NULL )
                {
                    if ( !hlMode ) // Don't paste copied block in highlight mode.  Not intuitive
                    {
                        pushUndoState();
                        fw::IPoint brCorner( getmaxx(copyBuffer)+pos.x, getmaxy(copyBuffer)+pos.y );
                        brCorner.cap( 0, 0, COLS-1, LINES-1 );
                        // There's a copy buffer, copy it in the main window
                        copywin( copyBuffer, mainWindow, 0, 0, pos.y, pos.x, brCorner.y, brCorner.x, false );
                    }
                }
                else if ( hlMode )
                {
                    pushUndoState();
                    // Fill the selection with the character
                    fw::IRect hlRect = computeRect( hlStart, pos );
                    fillRect( mainWindow, hlRect, lastChar );
                }
                else
                {
                    pushUndoState();
                    waddrawch( mainWindow, lastChar );
                }
            } break;

//        case 8/*bkspc*/: wcolor_set( mainWindow, 0, NULL ); mvwaddrawch( mainWindow, pos.y, pos.x-1, 0 ); wmove( mainWindow, pos.y, pos.x-1 ); break;

        // COPY
        case CTL_INS:
            {
                // First free the current copy buffer
                if ( copyBuffer != NULL )
                {
                    delwin( copyBuffer );
                    copyBuffer = NULL;
                }

                if ( hlMode )
                {
                    fw::IRect hlRect = computeRect( hlStart, pos );
                    copyBuffer = newwin( hlRect.height, hlRect.width, 0, 0 );
                    copywin( mainWindow, copyBuffer, hlRect.y, hlRect.x, 0, 0, hlRect.height-1, hlRect.width-1, false );

                    // Copy to the system clipboard
                    {
                        std::string clipBuffer;
                        clipBuffer += "ASCIIDRAWCLIP,";
                        for ( int y = 0; y < hlRect.height; ++y )
                        {
                            for ( int x = 0; x < hlRect.width; ++x )
                            {
                                int ch = mvwinch( mainWindow, hlRect.y+y, hlRect.x+x );
                                clipBuffer += toString( ch );
                                clipBuffer += ",";
                            }
                            clipBuffer += "\n,";
                        }
                        PDC_setclipboard( clipBuffer.c_str(), (long)clipBuffer.size() );
                    }
                }
                else
                {
                    lastChar = winch( mainWindow );
                    pair_content( (short)PAIR_NUMBER( lastChar ), &currentFG, &currentBG );
                    lastChar &= A_CHARTEXT;
                }
                wmove( mainWindow, pos.y, pos.x );
            } break;

        case ALT_INS:
            {
                pushUndoState();
                long cbLength;
                char* cbContent;
                PDC_getclipboard( &cbContent, &cbLength );
                StringList tokens = tokenize( cbContent, "," );
                // Paste between AsciiDraw instance
                if ( tokens.front() == "ASCIIDRAWCLIP" )
                {
                    wcolor_set( mainWindow,0, NULL );
                    StringList::iterator it    = tokens.begin();
                    StringList::iterator itEnd = tokens.end();
                    for ( ; it != itEnd; ++it )
                    {
                        if ( *it == "\n" )
                        {
                            pos.offset( 0, 1 );
                            if ( pos.y >= LINES )
                                break;
                            wmove( mainWindow, pos.y, pos.x );
                        }
                        else if ( ! it->empty() )
                        {
                            int ch = atoi( it->c_str() );
                            waddrawch( mainWindow, ch );
                        }
                    }
                }
                else
                {
                    for ( int i = 0; i < cbLength; ++i )
                    {
                        if ( cbContent[i] == 0x0d )
                        {
                            pos.offset( 0, 1 );
                            wmove( mainWindow, pos.y, pos.x );
                        }
                        else if ( cbContent[i] != 0x0a )
                        {
                            waddrawch( mainWindow, cbContent[i] & 0xff );
                        }
                    }
                }
                wmove( mainWindow, pos.y, pos.x );
                PDC_freeclipboard( cbContent );
            } break;

        // Shift-direction, highlighting
        case KEY_SUP:
        case KEY_SDOWN:
        case KEY_SLEFT:
        case KEY_SRIGHT:
            {
                if ( !hlMode )
                {
                    hlStart = pos;
                    hlMode = true;
                }
                switch ( c )
                {
                case KEY_SUP:    pos.offset(  0, -1 ); break;
                case KEY_SDOWN:  pos.offset(  0,  1 ); break;
                case KEY_SLEFT:  pos.offset( -1,  0 ); break;
                case KEY_SRIGHT: pos.offset(  1,  0 ); break;
                }
                wmove( mainWindow, pos.y, pos.x );
            } break;

        case 1: // CTRL-A
            {
                // Select everything
                hlMode = true;
                hlStart.set( 0, 0 );
                wmove( mainWindow, LINES-1, COLS-1 );

            } break;

        case KEY_HOME: pos.x = 0;                         wmove( mainWindow, pos.y, pos.x ); break;
        case KEY_END:  pos.x = COLS - 1;                  wmove( mainWindow, pos.y, pos.x ); break;
        case CTL_HOME: pos.set( 0, 0 );                   wmove( mainWindow, pos.y, pos.x ); break;
        case CTL_END:  pos.set( COLS - 1, LINES - 1 );    wmove( mainWindow, pos.y, pos.x ); break;

        /*case SDLK_PLUS:     k = CursesUI::Key_Plus; break;
        case SDLK_KP_PLUS:  k = CursesUI::Key_Plus; break;
        case SDLK_MINUS:    k = CursesUI::Key_Minus; break;
        case SDLK_KP_MINUS: k = CursesUI::Key_Minus; break;

        case SDLK_RIGHTBRACKET: k = CursesUI::Key_RightBracket; break;
        case SDLK_LEFTBRACKET:  k = CursesUI::Key_LeftBracket;  break;*/

        case 14: // CTRL-N
            {
                filename.clear();
            } break;

        case 19: // CTRL-S
            {
                if ( filename.empty() )
                {
                    std::string str = promptForFilename( filename );
                    if ( str.empty() )
                        break;
                    filename = str;
                }

                touchwin( mainWindow );
                wrefresh( mainWindow );
                scr_dump( filename.c_str() );
                documentIsDirty = false;
            } break;

        case 26: // CTRL-Z
            {
                popUndoState();
            } break;

        case 27: // ESC
            {
                if ( moveScriptCallback != LUA_REFNIL )
                {
                    luaL_unref( L, LUA_REGISTRYINDEX, moveScriptCallback );
                    moveScriptCallback = LUA_REFNIL;
                }
            } break;

        case KEY_F(1):
            {
                displayHelp();
            } break;
        case KEY_F(2):
            {
                // Prompt the user for key value;
                int charVal = promptForChar();
                if ( charVal < 0 )
                    break;

                /*if ( hlMode )
                {
                    // Fill the selection with the character
                    fw::IRect hlRect = computeRect( hlStart, pos );
                    fillRect( mainWindow, hlRect, charVal );
                }
                else
                {
                    waddrawch( mainWindow, charVal );
                }*/
                lastChar = charVal;
                if ( copyBuffer != NULL )
                {
                    delwin( copyBuffer );
                    copyBuffer = NULL;
                }
            } break;
        case KEY_F(3):
            {
                int color = promptForColor( lastChar );
                if ( color >= 0 )
                {
                    currentFG = color & 0x0f;
                    currentBG = (color & 0xf0) >> 4;
                }
                if ( copyBuffer != NULL )
                {
                    delwin( copyBuffer );
                    copyBuffer = NULL;
                }
            } break;
        case KEY_F(4): // Apply color to selection
            {
                if ( hlMode )
                {
                    pushUndoState();
                    fw::IRect hlRect = computeRect( hlStart, pos );
                    for ( int y = hlRect.y; y < hlRect.y+hlRect.height; ++y )
                        for ( int x = hlRect.x; x < hlRect.x+hlRect.width; ++x )
                        {
                            int charVal = mvwinch( mainWindow, y, x );
                            charVal &= 0xff;
                            wcolor_set( mainWindow, FGBG( currentFG, currentBG ), NULL );
                            mvwaddrawch( mainWindow, y, x, charVal );
                            wmove( mainWindow, pos.y, pos.x );
                        }
                }
                else
                {
                    pushUndoState();
                    int charVal = winch( mainWindow );
                    charVal &= 0xff;
                    wcolor_set( mainWindow, FGBG( currentFG, currentBG ), NULL );
                    waddrawch( mainWindow, charVal );
                    wmove( mainWindow, pos.y, pos.x );
                }
            } break;
        case KEY_F(5):
            {
                std::string scriptFunctionName = promptForScriptName();
                if ( ! scriptFunctionName.empty() )
                    runCommand( L, scriptFunctionName );
            } break;
        case KEY_F(6):
            {
                if ( moveScriptCallback != LUA_REFNIL )
                {
                    luaL_unref( L, LUA_REGISTRYINDEX, moveScriptCallback );
                    moveScriptCallback = LUA_REFNIL;
                }
                lua_close( L );
                L = initializeScripting();
            } break;
        case KEY_F(7):  break;
        case KEY_F(8): showBar = !showBar; break;
        case KEY_F(9):
            {
                int unicodeChars[] = 
                {
                    32,9786,9787,9829,9830,9827,9824,8226,9688,9675,9689,9794,9792,9834,9835,
                    9788,9658,9668,8597,8252,182,167,9644,8616,8593,8595,8594,8592,8735,8596,
                    9650,9660,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,
                    53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,
                    77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,
                    101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,
                    119,120,121,122,123,124,125,126,8962,199,252,233,226,228,224,229,231,234,
                    235,232,239,238,236,196,197,201,230,198,244,246,242,251,249,255,214,220,
                    162,163,165,8359,402,225,237,243,250,241,209,170,186,191,8976,172,189,188,
                    161,171,187,9617,9618,9619,9474,9508,9569,9570,9558,9557,9571,9553,9559,
                    9565,9564,9563,9488,9492,9524,9516,9500,9472,9532,9566,9567,9562,9556,9577,
                    9574,9568,9552,9580,9575,9576,9572,9573,9561,9560,9554,9555,9579,9578,9496,
                    9484,9608,9604,9612,9616,9600,945,223,915,960,931,963,181,964,934,920,937,
                    948,8734,966,949,8745,8801,177,8805,8804,8992,8993,247,8776,176,8729,183,
                    8730,8319,178,9632,32
                };
                // html screenshot
                std::string html;
                html += "<pre><span style=\"font-family: Lucida Console, monospace; font-size:12px; line-height:1\">";
                short old_fg = -1, old_bg = -1;
                for ( int y = 0; y < LINES; ++y )
                {
                    for ( int x = 0; x < COLS; ++x )
                    {
                        int ch = mvwinch( mainWindow, y, x );
                        short fg, bg;
                        pair_content( (short)PAIR_NUMBER( ch ), &fg, &bg );
                        if ( old_fg != fg || old_bg != bg )
                        {
                            if ( old_fg != -1 || old_bg != -1 )
                                html += "</span>";
                            html += "<span style=\"background-color:#";
                            {
                                short r, g, b;
                                color_content( bg, &r, &g, &b );
                                html += toHex( (unsigned char)((double)r/1000*0xff), 2 );
                                html += toHex( (unsigned char)((double)g/1000*0xff), 2 );
                                html += toHex( (unsigned char)((double)b/1000*0xff), 2 );
                            }
                            html += "; color:#";
                            {
                                short r, g, b;
                                color_content( fg, &r, &g, &b );
                                html += toHex( (unsigned char)((double)r/1000*0xff), 2 );
                                html += toHex( (unsigned char)((double)g/1000*0xff), 2 );
                                html += toHex( (unsigned char)((double)b/1000*0xff), 2 );
                            }
                            html += "\">";
                        }
                        unsigned char c = ch & 0xff;
                        int uni_ch = unicodeChars[c];
                        if ( uni_ch < 127 && uni_ch != 32 && c != '<' && c != '>' )
                            html += (char)(unicodeChars[c]);
                        else if ( uni_ch != 32 )
                            html += "&#x" + toHex( uni_ch, 4 ) + ";";
                        else
                            html += "&nbsp;";
                        old_fg = fg;
                        old_bg = bg;
                    }
                    html += "\n";
                }
                
                html += "</span></span></pre>";
                PDC_setclipboard( html.c_str(), (long)html.size() );
            } break;
        case KEY_F(10): break;
        case KEY_F(11): break;
        case KEY_F(12): mustExit = true; break;

        case ALT_0: runCommand( L, "ALT-0" ); break;
        case ALT_1: runCommand( L, "ALT-1" ); break;
        case ALT_2: runCommand( L, "ALT-2" ); break;
        case ALT_3: runCommand( L, "ALT-3" ); break;
        case ALT_4: runCommand( L, "ALT-4" ); break;
        case ALT_5: runCommand( L, "ALT-5" ); break;
        case ALT_6: runCommand( L, "ALT-6" ); break;
        case ALT_7: runCommand( L, "ALT-7" ); break;
        case ALT_8: runCommand( L, "ALT-8" ); break;
        case ALT_9: runCommand( L, "ALT-9" ); break;
        case ALT_A: runCommand( L, "ALT-A" ); break;
        case ALT_B: runCommand( L, "ALT-B" ); break;
        case ALT_C: runCommand( L, "ALT-C" ); break;
        case ALT_D: runCommand( L, "ALT-D" ); break;
        case ALT_E: runCommand( L, "ALT-E" ); break;
        case ALT_F: runCommand( L, "ALT-F" ); break;
        case ALT_G: runCommand( L, "ALT-G" ); break;
        case ALT_H: runCommand( L, "ALT-H" ); break;
        case ALT_I: runCommand( L, "ALT-I" ); break;
        case ALT_J: runCommand( L, "ALT-J" ); break;
        case ALT_K: runCommand( L, "ALT-K" ); break;
        case ALT_L: runCommand( L, "ALT-L" ); break;
        case ALT_M: runCommand( L, "ALT-M" ); break;
        case ALT_N: runCommand( L, "ALT-N" ); break;
        case ALT_O: runCommand( L, "ALT-O" ); break;
        case ALT_P: runCommand( L, "ALT-P" ); break;
        case ALT_Q: runCommand( L, "ALT-Q" ); break;
        case ALT_R: runCommand( L, "ALT-R" ); break;
        case ALT_S: runCommand( L, "ALT-S" ); break;
        case ALT_T: runCommand( L, "ALT-T" ); break;
        case ALT_U: runCommand( L, "ALT-U" ); break;
        case ALT_V: runCommand( L, "ALT-V" ); break;
        case ALT_W: runCommand( L, "ALT-W" ); break;
        case ALT_X: runCommand( L, "ALT-X" ); break;
        case ALT_Y: runCommand( L, "ALT-Y" ); break;
        case ALT_Z: runCommand( L, "ALT-Z" ); break;
        case KEY_MOUSE:
            {
                MEVENT mEvent;
                int i = nc_getmouse( &mEvent );
                if ( mEvent.bstate & BUTTON1_CLICKED )
                {
                    wmove( mainWindow, mEvent.y, mEvent.x );
                    hlMode = false;
                    hlMouse = false;
                }
                else if ( hlMode && (mEvent.bstate & BUTTON1_PRESSED) &&
                    (mEvent.bstate & BUTTON_MODIFIER_SHIFT || mEvent.bstate & BUTTON_MODIFIER_CONTROL) )
                {
                    fw::IRect hlRect = computeRect( hlStart, pos );
                    if ( hlRect.contains( fw::IPoint( mEvent.x, mEvent.y ) ) )
                    {
                        pushUndoState();
                        isDragging = true;
                        dragOffset.set( mEvent.x - hlRect.x, mEvent.y - hlRect.y );
                        if ( mEvent.bstate & BUTTON_MODIFIER_SHIFT )
                        {
                            wcolor_set( mainWindow,0, NULL );
                            fillRect( mainWindow, hlRect, ' ' );
                        }

                        dragWindow = newwin( 1, 1, 1, 1 );
                        wresize( dragWindow, hlRect.height, hlRect.width );
                        mvwin( dragWindow, hlRect.y, hlRect.x );
                        copywin( hlWindow, dragWindow, 0, 0, 0, 0, hlRect.height-1, hlRect.width-1, false );

                        hlMode = false;
                    }
                }
                else if ( mEvent.bstate & BUTTON1_PRESSED )
                {
                    hlMode = true;
                    hlMouse = true;
                    isDragging = false;
                    hlStart.set( mEvent.x, mEvent.y );
                    wmove( mainWindow, mEvent.y, mEvent.x );
                }
                else if ( mEvent.bstate & BUTTON1_RELEASED )
                {
                    if ( isDragging )
                    {
                        // Copy the dragWindow's content back to the mainWindow
                        isDragging = false;
                        fw::IRect dragRect = getWindowRect( dragWindow );
                        // Remove the reverse flag on the chars
                        for ( int y = 0; y < dragRect.height; ++y )
                            for ( int x = 0; x < dragRect.width; ++x )
                            {
                                int charVal = mvwinch( dragWindow, y, x );
                                charVal &= ~A_REVERSE;
                                mvwaddrawch( dragWindow, y, x, charVal );
                            }

                        copywin( dragWindow, mainWindow, 0, 0, dragRect.y, dragRect.x, dragRect.y+dragRect.height-1, dragRect.x+dragRect.width-1, false );
                        delwin( dragWindow );
                        dragWindow = NULL;
                    }
                    else
                    {
                        hlMouse = false;
                        wmove( mainWindow, mEvent.y, mEvent.x );
                        // Sometime a click is not registered we only get up/down
                        fw::IRect hlRect = computeRect( hlStart, pos );
                        if ( hlRect.width < 2 && hlRect.height < 2 )
                        {
                            hlMode = false;
                        }
                    }
                }
            } break;
        }
    }

    endwin();

    lua_close( L );
    return 0;
}

