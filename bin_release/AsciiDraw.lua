function SetRandomCharacter( glyphs, colours )
    SetFG( colours[Random() % #colours + 1] )
    SetChar( glyphs[Random() % #glyphs + 1] )
    Set()
end

-- Floods an area of matching characters calling the setter function
function FloodFill( setter )
    local x, y = GetPos()
    local c, fg, bg = Get()
    -- Make sure not floodfilling over same char
    do
        local c_, fg_, bg_ = GetChar()
        if c == c_ and fg == fg_ and bg == bg_ then
            return
        end
    end
    setter()
    local adjacent = { {x+1, y}, {x-1, y}, {x, y+1}, {x, y-1} }
    for i = 1, 4 do
        Move( unpack( adjacent[i] ) )
        local c_, fg_, bg_ = Get()
        if c == c_ and fg == fg_ and bg == bg_ then
            FloodFill( setter )
        end
    end
end

-- Calls FloodFill with a simple setter function
DeclareCommand{
    name="ALT-F", 
    op = function()
        -- Save x,y to restore them after the call
        local x, y = GetPos()
        PushUndoState()
        FloodFill( function() Set() end )
        Move( x, y )
    end
}

-- Replace all occurences of the character at a given position
-- with the current character.
DeclareCommand{
    name="ALT-R", 
    op = function()
        -- Save x,y to restore them after the call
        local oldx, oldy = GetPos()
        local c, fg, bg = Get()
        PushUndoState()
        for y = 0, 49 do
            for x = 0, 79 do
                Move( x, y )
                local c_, fg_, bg_ = Get()
                if c == c_ and fg == fg_ and bg == bg_ then Set() end
            end
        end
        Move( oldx, oldy )
    end
}

function FillAreaWithRandomCharacters( glyphs, colours )
    SetBG( BLACK )
    if not HasSelection() then
        Select( 0, 0, 79, 49 )
    end

    -- Save the selection to restore it after the call
    local x1, y1, x2, y2 = GetSelection()
    Unselect()
    for y = y1, y2 do
        for x = x1, x2 do
            Move( x, y )
            SetRandomCharacter( glyphs, colours )
        end
    end
    Select( x1, y1, x2, y2 )
end

-- Fill area with grass, fills selected area or floods
DeclareCommand{
    name="ALT-G", 
    op = function()
        -- Save x,y to restore them after the call
        local oldx, oldy = GetPos()
        PushUndoState()
        local glyphs = { '.', ',', '`', ';', "'", 250, '"', ':' }
        local colors = { 2, 10, 2, 10, 2, 10, 2, 10, 2, 10, 3, 11 }
        if not HasSelection() then
            FloodFill( function() SetRandomCharacter( glyphs, colors ) end )
        else
            FillAreaWithRandomCharacters( glyphs, colors )
        end
        Move( oldx, oldy )
    end
}

-- Fill area with stone floor, fills selected area or floods
DeclareCommand{
    name="ALT-S", 
    op = function()
        -- Save x,y to restore them after the call
        local oldx, oldy = GetPos()
        PushUndoState()
        local glyphs = { '.', '.', 249, 249, 46, 46, 250, 250, 7 }
        local colors = { 7, 8 }
        if not HasSelection() then
            FloodFill( function() SetRandomCharacter( glyphs, colors ) end )
        else
            FillAreaWithRandomCharacters( glyphs, colors )
        end
        Move( oldx, oldy )
    end
}

-- Draws a box in the selection
function DrawBox( h, v, tl, tr, bl, br )
    if not HasSelection() then
        return
    end

    PushUndoState()
    local x1, y1, x2, y2 = GetSelection()
    Unselect()
    SetChar( h )
    for x = x1, x2 do
        Move( x, y1 ) Set()
        Move( x, y2 ) Set()
    end
    SetChar( v )
    for y = y1, y2 do
        Move( x1, y ) Set()
        Move( x2, y ) Set()
    end
    SetChar( tl ) Move( x1, y1 ) Set()
    SetChar( tr ) Move( x2, y1 ) Set()
    SetChar( bl ) Move( x1, y2 ) Set()
    SetChar( br ) Move( x2, y2 ) Set()
    Select( x1, y1, x2, y2 )
end

DeclareCommand{
    name="ThickWall", 
    op = function()
        DrawBox( 205, 186, 201, 187, 200, 188 )
    end
}

DeclareCommand{
    name="ThinWall", 
    op = function()
        DrawBox( 196, 179, 218, 191, 192, 217 )
    end
}

DeclareCommand{
    name="Invert", 
    op = function()
        if not HasSelection() then
            Select( 0, 0, 79, 49 )
        end

        PushUndoState()
        -- Save the selection to restore it after the call
        local x1, y1, x2, y2 = GetSelection()
        Unselect()
        for y = y1, y2 do
            for x = x1, x2 do
                Move( x, y )
                c, fg, bg = Get()
                SetFG( bg )
                SetBG( fg )
                SetChar( c )
                Set()
            end
        end
        Select( x1, y1, x2, y2 )
    end
}

function drawDoor( charHorizontal, charVertical )

    if not HasSelection() then return end

    local x1, y1, x2, y2 = GetSelection()
    local width, height = x2-x1+1, y2-y1+1
    Unselect()
    if width == 3 and height == 1 then
        Move( x1, y1 )
        local lch = Get()
        Move( x1+2, y1 )
        local rch = Get()
        if lch == rch and rch == 196 then
            PushUndoState()
            Move( x1, y1 )   SetChar( 180 )            Set()
            Move( x1+1, y1 ) SetChar( charHorizontal ) Set()
            Move( x1+2, y1 ) SetChar( 195 )            Set()
        elseif lch == rch and rch == 205 then
            PushUndoState()
            Move( x1, y1 )   SetChar( 181 )            Set()
            Move( x1+1, y1 ) SetChar( charHorizontal ) Set()
            Move( x1+2, y1 ) SetChar( 198 )            Set()
        end
    elseif width == 1 and height == 3 then
        Move( x1, y1 )
        local lch = Get()
        Move( x1, y1+2 )
        local rch = Get()

        if lch == rch and rch == 179 then
            PushUndoState()
            Move( x1, y1   ) SetChar( 193 )          Set()
            Move( x1, y1+1 ) SetChar( charVertical ) Set()
            Move( x1, y1+2 ) SetChar( 194 )          Set()
        elseif lch == rch and rch == 186 then
            PushUndoState()
            Move( x1, y1   ) SetChar( 208 )          Set()
            Move( x1, y1+1 ) SetChar( charVertical ) Set()
            Move( x1, y1+2 ) SetChar( 210 )          Set()
        end
    end
    Select( x1, y1, x2, y2 )
end

DeclareCommand{
    name="ThinDoor", 
    op = function()
        drawDoor( 196, 179 )
    end
}

DeclareCommand{
    name="ThickDoor", 
    op = function()
        drawDoor( 205, 186 )
    end
}

-- Move callback to call Set() everytime the cursor is moved
DeclareCommand{
    name="cbDraw", 
    op = function()
        SetMoveCB(
            function()
                PushUndoState()
                Set()
            end
        )
    end
}

DeclareCommand{
    name="cbDrawDirtWall", 
    op = function()
        local glyphs = { 176, 177, 178 }
        SetMoveCB(
            function()
                PushUndoState()
                SetChar( glyphs[Random() % #glyphs + 1] )
                Set()
            end
        )
    end
}

local thinWallLookup = {
    u  = 179, d  = 179, l  = 196, r  = 196,
    uu = 179, ud = 179, ul = 191, ur = 218,
    du = 179, dd = 179, dl = 217, dr = 192,
    lu = 192, ld = 218, ll = 196, lr = 196,
    ru = 217, rd = 191, rl = 196, rr = 196,
}

local thickWallLookup = {
    u  = 186, d  = 186, l  = 205, r  = 205,
    uu = 186, ud = 186, ul = 187, ur = 201,
    du = 186, dd = 186, dl = 188, dr = 200,
    lu = 200, ld = 201, ll = 205, lr = 205,
    ru = 188, rd = 187, rl = 205, rr = 205,
}

function cbDrawLinkedWall( wallLookup )
    local olddir
    local oldx, oldy = GetPos()
    SetMoveCB(
        function()
            PushUndoState()
            local x, y = GetPos()
            local dx = x - oldx
            local dy = y - oldy
            local newdir
            if dx == 0 and dy == 0 then
                -- No movement
                return
            elseif dx ~= 0 and dy ~= 0 then
                -- Diagonal movement, unsupported
                oldx, oldy = x, y
                return
            elseif dx > 0 then newdir = 'r'
            elseif dx < 0 then newdir = 'l'
            elseif dy > 0 then newdir = 'd'
            elseif dy < 0 then newdir = 'u'
            end
            if newdir and olddir then
                SetChar( wallLookup[olddir .. newdir] )
            else
                SetChar( wallLookup[newdir] )
            end
            Move( oldx, oldy )
            Set()
            Move( x, y )
            -- Print current direction
            SetChar( wallLookup[newdir] )
            Set()
            olddir = newdir
            oldx, oldy = x, y
        end
    )
end

DeclareCommand{
    name="cbDrawThinWall", 
    op = function()
        cbDrawLinkedWall( thinWallLookup )
    end
}

DeclareCommand{
    name="cbDrawThickWall", 
    op = function()
        cbDrawLinkedWall( thickWallLookup )
    end
}


