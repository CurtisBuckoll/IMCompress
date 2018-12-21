#pragma once

#include "stdafx.h"

#include "Util.h"

#include <vector>

//========================================================================
// n x n square matrix
struct Matrix
{
    //--------------------------------------------------------------
    // Init to identity
    Matrix( size_t size );

    //--------------------------------------------------------------
    //
    void scaleBy( double s );

    //--------------------------------------------------------------
    //
    void transpose();

    //--------------------------------------------------------------
    //
    void makeRandom( int low, int hi );

    //--------------------------------------------------------------
    //
    void print();

    //--------------------------------------------------------------
    //
    Matrix operator*( const Matrix& mat );

    //--------------------------------------------------------------
    //
    size_t                           sz_;
    std::vector<std::vector<double>> matrix_;
};
