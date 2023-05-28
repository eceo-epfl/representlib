//  Copyright (C) 2016 Bjoern Barz (University of Jena)
//
//  This file is part of libmaxdiv.
//
//  libmaxdiv is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  libmaxdiv is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with libmaxdiv. If not, see <http://www.gnu.org/licenses/>.

/**
* @file
* Helper functions
*
* @author Bjoern Barz <bjoern.barz@uni-jena.de>
*/

#ifndef MAXDIV_UTILS_H
#define MAXDIV_UTILS_H

#include <string>
#include <vector>
#include "DataTensor.h"

namespace MaxDiv
{

/**
* Removes white-space and newline characters from the beginning and the end of a string.
*
* @param[in] str The string to be trimmed.
*
* @return The trimmed string.
*/
std::string trim(std::string str);

/**
* Turns a string into lower-case.
*
* @param[in] str The input string.
*
* @return Lower-case variant of `str`.
*/
std::string strtolower(const std::string & str);

/**
* Turns a string into upper-case.
*
* @param[in] str The input string.
*
* @return Upper-case variant of `str`.
*/
std::string strtoupper(const std::string & str);

/**
* Splits a string up into tokens by given delimiters.
*
* @param[in] str The string to be split.
*
* @param[in] delimiters Each character in this string is a delimiter. Delimiters won't be contained in the tokens.
*
* @param[out] tokens Each token is appended to this vector.
*
* @return Number of tokens.
*/
int splitString(const std::string & str, const char * delimiters, std::vector<std::string> & tokens);

/**
* Reads a non-spatial time series from a CSV file, whose rows correspond to time steps and
* whose columns correspond to attributes. Missing values may be encoded as "nan".
* 
* @param[in] filename The path of the file.
*
* @param[in] delimiter The delimiter separating the fields in the CSV file.
* 
* @param[in] firstRow The index of the first row to be read. Set this to 1 for skipping a header line.
* 
* @param[in] firstCol The index of the first column to be read. Set this to 1 for skipping an index column.
*
* @param[in] lastCol The index of the last column to be read.
*
* @return Returns a DataTensor containing the data read from the CSV file. The tensor will be empty if
* the file could not be read or parsed.
*/
DataTensor readDataFromCSV(const std::string & filename, char delimiter = ',',
                           DataTensor::Index firstRow = 0, DataTensor::Index firstCol = 0,
                           DataTensor::Index lastCol = -1);

}

#endif