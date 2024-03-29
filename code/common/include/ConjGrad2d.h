#pragma once
#ifndef __CONJGRAD_2D_H__
#define __CONJGRAD_2D_H__

#pragma warning(disable : 4244 4267 4996)

#include "boost/numeric/ublas/vector.hpp"
#include "boost/numeric/ublas/matrix.hpp"
#include <boost/numeric/ublas/io.hpp>

#include <utility>

#include "Configure.h"

namespace Glb
{

    /*
    Conjugate Gradient Method
    使用共轭梯度法求解对称正定的线性方程组。
    这是一种迭代方法，试图收敛于一个解。
    当迭代次数达到 max_iter，或者残差的二范数低于 tol 时，该方法将退出。
    @param A 要解的线性方程组。这必须是对称正定的。
    @param b 系统的常数项。
    @param x 存储输出的位置。这必须是适当大小的向量。该方法会覆盖这个向量。
    @param max_iter 求解器将进行的最大迭代次数。
    @param tol 最大容忍误差，低于此误差时，求解器将认为解已经收敛。
    */

    // 共轭梯度法
    template <typename Matrix, typename Vector>
    bool cg_solve2d(const Matrix &A, const Vector &b, Vector &x, int max_iter, double tol)
    {
        std::fill(x.begin(), x.end(), 0);

        Vector r = b;
        Vector p = b;
        Vector r_old;
        Vector temp;

        // CG loop
        for (int niter = 0; niter < max_iter; niter++)
        {
            temp = prod(A, p);
            double alpha = inner_prod(r, r) / inner_prod(p, temp);
            x += (p * alpha);
            r_old = r;
            r -= (temp * alpha);
            double residn = norm_2(r);
            if (residn < tol)
            {
                std::cout << "numiters: " << niter << std::endl;
                return true;
            }
            double beta = inner_prod(r, r) / inner_prod(r_old, r_old);
            p = r + p * beta;
        }

        std::cout << "WARNING: cg_solve did not converge" << std::endl;
        return false;
    }

    int getIndex(int i, int j)
    {
        if (i < 0 || i > Eulerian2dPara::theDim2d[0] - 1)
            return -1;
        if (j < 0 || j > Eulerian2dPara::theDim2d[1] - 1)
            return -1;

        int col = i;
        int row = j * Eulerian2dPara::theDim2d[0];
        return col + row;
    }

    void getCell(int index, int &i, int &j)
    {
        j = (int)index / Eulerian2dPara::theDim2d[0]; // row
        i = index - j * Eulerian2dPara::theDim2d[0];  // col
    }

    template <typename Matrix, typename Vector>
    void applyPreconditioner2d(const Matrix &A, const Vector &precon, const Vector &r, Vector &z)
    {
        unsigned int numCells = r.size();
        Vector q(numCells, 0.0); // Use Vector type directly

        for (unsigned int index = 0; index < numCells; index++)
        {
            int i, j;
            getCell(index, i, j);

            int neighbori = getIndex(i - 1, j);
            int neighborj = getIndex(i, j - 1);
            double termi = (neighbori != -1) ? A(index, neighbori) * precon(neighbori) * q(neighbori) : 0.0;
            double termj = (neighborj != -1) ? A(index, neighborj) * precon(neighborj) * q(neighborj) : 0.0;

            double t = r(index) - termi - termj;
            q(index) = t * precon(index);
        }

        for (int index = numCells - 1; index >= 0; index--)
        {
            int i, j;
            getCell(index, i, j);

            int neighbori = getIndex(i + 1, j);
            int neighborj = getIndex(i, j + 1);
            double termi = (neighbori != -1) ? A(index, neighbori) * precon(index) * z(neighbori) : 0.0;
            double termj = (neighborj != -1) ? A(index, neighborj) * precon(index) * z(neighborj) : 0.0;

            double t = q(index) - termi - termj;
            z(index) = t * precon(index);
        }
    }

    template <typename Matrix, typename Vector>
    bool cg_psolve2d(const Matrix &A, const Vector &precon, const Vector &b, Vector &p, int max_iter, double tol)
    {
        std::fill(p.begin(), p.end(), 0);
        Vector r = b;
        Vector z = b;
        Vector s = b;
        applyPreconditioner2d(A, precon, r, s);

        double sigma = inner_prod(s, r);

        for (int niter = 0; niter < max_iter; niter++)
        {
            z = prod(A, s);
            double alpha = sigma / inner_prod(s, z);
            p += alpha * s;
            r -= alpha * z;
            double resign = norm_2(r);
            if (resign < tol)
            {
                return true;
            }

            applyPreconditioner2d(A, precon, r, z);
            double sigma_new = inner_prod(z, r);
            double beta = sigma_new / sigma;
            s = z + beta * s;
            sigma = sigma_new;
        }

        std::cout << "WARNING: cg_psolve did not converge: " << norm_2(r) << std::endl;
        return false;
    }

    // template<typename Matrix, typename Vector>
    // void applyPreconditioner2d(const Matrix& A, const Vector& precon, const Vector& r, Vector& z)
    //{
    //     unsigned int numCells = r.size();
    //     ublas::vector<double> q(numCells);
    //     std::fill(q.begin(), q.end(), 0);

    //    // First solve Lq = r
    //    for (unsigned int index = 0; index < numCells; index++)
    //    {
    //        //std::cout << "Initializing row " << row << "/" << numCells << std::endl;
    //        int i, j, k; getCell(index, i, j); // Each row corresponds to a cell

    //        int neighbori = getIndex(i - 1, j);
    //        int neighborj = getIndex(i, j - 1);
    //        double termi = neighbori != -1 ? A(index, neighbori) * precon(neighbori) * q(neighbori) : 0;
    //        double termj = neighborj != -1 ? A(index, neighborj) * precon(neighborj) * q(neighborj) : 0;
    //
    //        double t = r(index) - termi - termj;
    //        q(index) = t * precon(index);
    //    }

    //    //std::cout << "q: " << q << std::endl;

    //    // Next solve transpose(L)z = q
    //    for (int index = numCells - 1; index >= 0; index--)
    //    {
    //        //std::cout << "Initializing row " << row << "/" << numCells << std::endl;
    //        int i, j; getCell(index, i, j); // Each row corresponds to a cell

    //        int neighbori = getIndex(i + 1, j);
    //        int neighborj = getIndex(i, j + 1);
    //        double termi = neighbori != -1 ? A(index, neighbori) * precon(index) * z(neighbori) : 0;
    //        double termj = neighborj != -1 ? A(index, neighborj) * precon(index) * z(neighborj) : 0;
    //
    //        double t = q(index) - termi - termj;
    //        z(index) = t * precon(index);
    //    }
    //    //std::cout << "z: " << z << std::endl;
    //}

    // template<typename Matrix, typename Vector>
    // bool cg_psolve2d(const Matrix& A, const Vector& precon, const Vector& b, Vector& p, int max_iter, double tol)
    //{
    //     std::fill(p.begin(), p.end(), 0);
    //     Vector r = b;
    //     Vector z = b;
    //     Vector s = b;
    //     //std::cout << "r: " << r << std::endl;
    //     applyPreconditioner2d(A, precon, r, s);

    //    double resign;
    //    double sigma = inner_prod(s, r);

    //    for (int niter = 0; niter < max_iter; niter++)
    //    {
    //        z = prod(A, s);
    //        double alpha = sigma / inner_prod(s, z);
    //        p += alpha * s;
    //        r -= alpha * z;
    //        resign = norm_2(r);
    //        if (resign < tol)
    //        {
    //            //std::cout << "numiters: "<< niter <<std::endl;
    //            return true;
    //        }

    //        applyPreconditioner2d(A, precon, r, z);
    //        double sigma_new = inner_prod(z, r);
    //        double beta = sigma_new / sigma;
    //        s = z + beta * s;
    //        sigma = sigma_new;
    //    }

    //    std::cout << "WARNING: cg_psolve did not converge: " << resign << std::endl;
    //    return false;
    //}
}

#endif