/*========================================================================

    Name:     OpenFileDialog.h

    Date:     June 2017

    Author:   Curtis Buckoll

    Overview:

        Provides an interface for a user to open and load a file by means
    of an open file dialog.

========================================================================*/

#pragma once

//--------------------------------------------------------------

#include <windows.h>
#include <string>
#include <vector>

#include "Errors.h"
#include "Util.h"

//========================================================================
// OpenFileDialog
class OpenFileDialog
{
public:

    //--------------------------------------------------------------
    //
    OpenFileDialog( const std::string& dialog_name );

    //--------------------------------------------------------------
    // Runs the dialog and populates the supplied byte vector with
    // the raw data.
    MsgNum invoke( std::vector<Byte>& data );

    //--------------------------------------------------------------
    // Returns a constant reference to the file path string. The
    // instance of this class must persist for the duration that 
    // it is used externally.
    const std::string& getFilePath() const { return file_path_; };


private:

    //--------------------------------------------------------------
    // Hide default constructor so it cannot be called.
    OpenFileDialog() = delete;

    //--------------------------------------------------------------
    // 
    const char* runDialog();

    //--------------------------------------------------------------
    // Converts a 16-bit wide char string to an 8-bit ASCII 
    // representation. All code points are expected to be less than
    // 256.
    const char* toASCII( const wchar_t* str ) const;

    //--------------------------------------------------------------
    // Converts an ASCII string to a UTF-16 wide char string.
    const wchar_t* toWideChar( const char* str ) const;

    //--------------------------------------------------------------
    //
    OPENFILENAME            ofname_;
    std::string             dialog_name_;
    std::string             file_path_;
};

