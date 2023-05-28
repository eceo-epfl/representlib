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
* Mathematical helper functions
*
* @author Bjoern Barz <bjoern.barz@uni-jena.de>
*/

#ifndef MAXDIV_MATH_UTILS_H
#define MAXDIV_MATH_UTILS_H

#define _USE_MATH_DEFINES

#include <cmath>
#include <Eigen/Core>
#include <Eigen/Cholesky>
#include "DataTensor.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace MaxDiv
{

/**
* @brief Regularized Cholesky Decomposition with computation of log-determinant
*
* This function uses Eigen to perform the Cholesky decomposition on a given matrix. If the decomposition fails,
* a small regularizer will be added to the main diagonal of the matrix for the next try. If the decomposition
* still fails, the regularizer will be increased.
*
* This function can also use the Cholesky decomposition to compute the natural logarithm of the determinant of
* a symmetric matrix. This computation will be performed in log space to prevent overflows.
*
* @param[in] mat A symmetric, positive-definite matrix.
*
* @param[out] llt A pointer to the address where the decomposition will be stored. May be `NULL`.
*
* @param[out] logdet A pointer to a scalar vlaue where the natural logarithm of the determinant of `mat` will be
* stored. May be `NULL`.
*/
template<typename Derived>
void cholesky(const Eigen::MatrixBase<Derived> & mat, Eigen::LLT<typename Derived::PlainObject> * llt, typename Derived::Scalar * logdet = NULL)
{
    typedef typename Derived::Scalar Scalar;
    typedef typename Derived::PlainObject Matrix;
    
    if (llt == NULL && logdet == NULL)
        return;
    
    // Try Cholesky decomposition of original matrix
    Eigen::LLT<Matrix> * chol = (llt != NULL) ? llt : new Eigen::LLT<Matrix>();
    chol->compute(mat);
    
    // Increase regularizer on failure
    Scalar regularizer = 0;
    while (chol->info() != Eigen::Success)
    {
        regularizer += 1e-4;
        chol->compute(mat + (Matrix::Identity(mat.rows(), mat.cols()) * regularizer));
    }
    
    // Compute log-determinant if requested
    if (logdet != NULL)
    {
        *logdet = 0;
        auto & U = chol->matrixL();
        for (typename Matrix::Index i = 0; i < U.rows(); ++i)
            *logdet += std::log(U(i, i));
        *logdet *= 2;
    }
    
    // Clean up
    if (llt == NULL)
        delete chol;
}

/**
* Computes a Gaussian kernel: \f$k(x,y) = \left( 2 \pi \sigma^2 \right)^{-D/2} \cdot \exp \left( - \frac{\left \| x-y \right \|^2}{2 \sigma^2} \right)\f$
*
* @param[in] X N-by-D-Matrix with N samples of dimensionality D.
*
* @param[in] kernel_sigma_sq The value of `sigma^2`.
*
* @param[in] normed Specifies whether to normalize the kernel by dividing all values by `(2 * pi * sigma^2)^(D/2)`.
*
* @return Returns the Gaussian kernel.
*/
template<typename Derived>
typename Derived::PlainObject gauss_kernel(const Eigen::MatrixBase<Derived> & X, typename Derived::Scalar kernel_sigma_sq = 1, bool normed = true)
{
    // Compute distance matrix
    typename Derived::PlainObject kernel(X.rows(), X.rows());
    typename Derived::Index x, y;
    #pragma omp parallel for private(x,y) schedule(static,1)
    for (x = 0; x < X.rows(); ++x)
    {
        kernel(x, x) = 0;
        for (y = x + 1; y < X.rows(); ++y)
        {
            kernel(x, y) = (X.row(x) - X.row(y)).squaredNorm();
            kernel(y, x) = kernel(x, y);
        }
    }
    
    // Compute kernel
    kernel /= -2 * kernel_sigma_sq;
    kernel = kernel.array().exp();
    if (normed)
        kernel /= std::pow(2 * M_PI * kernel_sigma_sq, X.cols() / 2.0);
    return kernel;
}

/**
* Computes a Gaussian kernel: \f$k(x,y) = \left( 2 \pi \sigma^2 \right)^{-D/2} \cdot \exp \left( - \frac{\left \| x-y \right \|^2}{2 \sigma^2} \right)\f$
*
* @param[in] data DataTensor with the samples to compute the Gaussian kernel for.  
* If the data contain missing samples, they must have been masked by calling `DataTensor::mask()`. Kernel values
* involving a missing sample are defined to be 0.
*
* @param[in] kernel_sigma_sq The value of `sigma^2`.
*
* @param[in] normed Specifies whether to normalize the kernel by dividing all values by `(2 * pi * sigma^2)^(D/2)`.
*
* @return Returns the Gaussian kernel.
*/
ScalarMatrix gauss_kernel(const DataTensor & data, Scalar kernel_sigma_sq = 1, bool normed = true);


/**
* @brief An implicit representation of a Gaussian kernel
*
* Given data samples x and y, the Gaussian kernel is defined as:
*
* \f[
*   k(x,y) = \left( 2 \pi \sigma^2 \right)^{-D/2} \cdot \exp \left( - \frac{\left \| x-y \right \|^2}{2 \sigma^2} \right)
* \f]
*
* As opposed to the `gauss_kernel` function, this class does not materialize the entire kernel at once,
* but computes its values on demand. This can be used to save memory when dealing with a lot of samples.
*/
class GaussKernel
{
public:

    /**
    * @param[in] data The DataTensor with the samples to compute the Gaussian kernel for.
    * Only a reference to this object will be stored, so its lifetime should be as least as long
    * as the lifetime of this GaussKernel.  
    * If the data contain missing samples, they must have been masked by calling `DataTensor::mask()`. Kernel values
    * involving a missing sample are defined to be 0.
    *
    * @param[in] kernel_sigma_sq The value of `sigma^2`.
    *
    * @param[in] normed Specifies whether to normalize the kernel by dividing all values by `(2 * pi * sigma^2)^(D/2)`.
    */
    GaussKernel(const DataTensor & data, Scalar kernel_sigma_sq = 1, bool normed = true);
    
    /**
    * @return Returns the value of the Gaussian kernel at `k(x, y)`.
    */
    Scalar operator()(DataTensor::Index x, DataTensor::Index y) const;
    
    /**
    * @return Explicitely computes and returns the full Gaussian kernel matrix.
    */
    ScalarMatrix materialize() const;
    
    /**
    * Explicitely computes a single column of the Gaussian kernel matrix.
    *
    * This is slightly faster than computing the values of the column one by one, but not as memory-efficient
    * if the values are only needed independently.
    *
    * @param[in] col The index of the column to compute.
    *
    * @return Returns a materialized vector with all values of the requested column.
    */
    Sample column(DataTensor::Index col) const;
    
    /**
    * Computes the sum of each row of the Gaussian kernel without having to materialize the entire kernel matrix.
    *
    * For a small number of samples a materialized representation of the kernel matrix will be used in order to
    * leverage vectorization, while for a large number of samples the sum will be computed without storing the
    * entire kernel matrix in order to save memory.
    *
    * @return Returns a vector with the sums of each row.
    */
    Sample rowwiseSum() const;
    
    /**
    * Computes the mean of each row of the Gaussian kernel without having to materialize the entire kernel matrix.
    *
    * For a small number of samples a materialized representation of the kernel matrix will be used in order to
    * leverage vectorization, while for a large number of samples the mean will be computed without storing the
    * entire kernel matrix in order to save memory.
    *
    * @return Returns a vector with the means of each row.
    */
    Sample rowwiseMean() const;


protected:

    const DataTensor & m_data;
    Scalar m_sigma_sq;
    Scalar m_norm;
    bool m_normed;

};

}

#endif