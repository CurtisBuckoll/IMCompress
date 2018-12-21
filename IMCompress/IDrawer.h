/*========================================================================

    Name:     IDrawer.h

    Date:     June 2017

    Author:   Curtis Buckoll

    Overview:

        Interface to a "drawer" object. Writes pixel data to the frame
    buffer contained in the IWindow instance.

========================================================================*/

#pragma once

//--------------------------------------------------------------
// Forward declarations

class IWindow;

//========================================================================
// WavDrawer
class IDrawer
{
public:

    //--------------------------------------------------------------
    //
    virtual ~IDrawer() {};

    //--------------------------------------------------------------
    //
    virtual void writeToWindowBuffer( IWindow& frame ) = 0;

    //--------------------------------------------------------------
    // Advances current output to possibly generate a new pixel
    // buffer to be returned
    virtual void advance() = 0;

protected:

    //--------------------------------------------------------------
    //
    IDrawer() {};
};

