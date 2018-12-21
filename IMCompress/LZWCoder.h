#pragma once

#include "Util.h"
#include <vector>
#include <unordered_map>

class LZWCoder
{
public:
    LZWCoder();
    ~LZWCoder();

    //--------------------------------------------------------------
    //
    MsgNum encode( const std::vector<UByte>& inData, std::vector<UByte>& outData );

private:

    //--------------------------------------------------------------
    //
    void insert12Bits( uint16_t bits, std::vector<UByte>& arr );

    //--------------------------------------------------------------
    //
    bool do_shift_;
};

