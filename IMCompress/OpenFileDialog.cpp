/*========================================================================

    Name:     OpenFileDialog.cpp

    Date:     June 2017

    Author:   Curtis Buckoll

    Overview:

========================================================================*/

#include "stdafx.h"
#include "OpenFileDialog.h"
#include <memory>
#include <iostream>

#include <fstream>

//========================================================================
//
OpenFileDialog::OpenFileDialog( const std::string& dialog_name )
    : dialog_name_( dialog_name )
{

}

//========================================================================
//
const char* OpenFileDialog::runDialog()
{
    static const int kBUFFERSIZE = 2048;
    wchar_t buffer[kBUFFERSIZE] = { 0 };

    // Construct the dialog object.
    OPENFILENAME ofname = { 0 };
    ofname.lStructSize = sizeof( ofname );
    ofname.lpstrFile = buffer;
    ofname.nMaxFile = kBUFFERSIZE;
    ofname.lpstrTitle = toWideChar(dialog_name_.c_str());

    // Pass in and populate with file path.
    if( GetOpenFileName( &ofname ) )
    {
        return toASCII( buffer );
    }

    return nullptr;
}

//========================================================================
//
MsgNum OpenFileDialog::invoke( std::vector<Byte>& data )
{
    std::unique_ptr<const char> file_path( runDialog() );

    if( file_path == nullptr )
    {
        return INVALID_FILE_PATH;
    }

    // The pointer is okay, so store the file path in case we need to 
    // ask for it later.
    file_path_ = std::string( file_path.get() );

    std::ifstream FILE( file_path.get(), std::ifstream::binary );

    if( FILE.is_open() )
    {
        std::cout << "Loading file: " << file_path.get() << std::endl;

        // Get the length of the file and make some room
        FILE.seekg( 0, FILE.end );
        unsigned long long len = FILE.tellg();
        FILE.seekg( 0, FILE.beg );

        data = std::vector<Byte>( static_cast<size_t>(len) );

        FILE.read( &data[0], len );

        FILE.close();
    }
    else
    {
        return printMsg( FAILURE_READING_FILE );
    }
    
    return STATUS_OKAY;
}

//========================================================================
//
const char* OpenFileDialog::toASCII( const wchar_t* str ) const
{
    static const Uint kMAX_SIZE = 1024;

    char* result = new char[kMAX_SIZE];

    Byte bytes[2];
    bytes[0] = str[0] & 0xff;
    bytes[1] = ( str[0] >> 8 ) & 0xff;

    // Based on the location of the first non-null
    // byte in the string, this should tell us which
    // byte of the two to use. Ideally, this should
    // make the function insensitive to the endianess 
    // of the machine.
    Uint byteIndex = bytes[1] == '\0' ? 0 : 1;

    Uint i = 0;

    while( ( bytes[0] || bytes[1] ) && i < kMAX_SIZE - 1 )
    {
        bytes[0] = str[i] & 0xff;
        bytes[1] = ( str[i] >> 8 ) & 0xff;

        result[i] = bytes[byteIndex];

        ++i;
    }

    result[i] = '\0';

    return result;
}

//========================================================================
//
const wchar_t* OpenFileDialog::toWideChar( const char* str ) const
{
    static const Uint kMAX_SIZE = 1024;

    wchar_t* result = new wchar_t[kMAX_SIZE];

    Uint i = 0;

    // Assume little endian encoding.
    while( str[i] != '\0' && i < kMAX_SIZE - 1 )
    {
        result[i] = str[i] & 0x00ff;
        ++i;
    }

    result[i] = '\0';

    return result;
}
