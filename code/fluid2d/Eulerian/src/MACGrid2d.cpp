#include "MACGrid2d.h"
#include "Configure.h"
#include <math.h>
#include <map>
#include <stdio.h>

namespace FluidSimulation
{
    namespace Eulerian2d
    {

        MACGrid2d::MACGrid2d()
        {
            cellSize = Eulerian2dPara::theCellSize2d;
            dim[0] = Eulerian2dPara::theDim2d[0];
            dim[1] = Eulerian2dPara::theDim2d[1];
            initialize();
        }

        MACGrid2d::MACGrid2d(const MACGrid2d &orig)
        {
            mU = orig.mU;
            mV = orig.mV;
            mD = orig.mD;
            mT = orig.mT;
            mSolid = orig.mSolid;
        }

        MACGrid2d &MACGrid2d::operator=(const MACGrid2d &orig)
        {
            if (&orig == this)
            {
                return *this;
            }
            mU = orig.mU;
            mV = orig.mV;
            mD = orig.mD;
            mT = orig.mT;
            mSolid = orig.mSolid;

            return *this;
        }

        MACGrid2d::~MACGrid2d()
        {
        }

        void MACGrid2d::reset()
        {
            mU.initialize(0.0);
            mV.initialize(0.0);
            mD.initialize(0.0);
            mT.initialize(Eulerian2dPara::ambientTemp);
        }

        void MACGrid2d::createSolids()
        {
            mSolid.initialize();

            /*int j = dim[1] / 2;
            for (int i = dim[0] / 4; i < dim[0] * 3 / 4; i++) {
                mSolid(i, j) = 1;
            }*/
        }

        void MACGrid2d::updateSources()
        {
            int sourcei = (int)dim[0] / 3;

            // 添加温度、密度
            mT(sourcei, 1) = 1.0;
            mD(sourcei, 1) = 1.0;

            // 赋予初始垂直方向速度
            mV(sourcei, 1) = Eulerian2dPara::sourceVelocity;
        }

        void MACGrid2d::initialize()
        {
            reset();
            createSolids();
            assert(checkDivergence());
        }

        // 根据温度差异计算 Boussinesq Force
        double MACGrid2d::getBoussinesqForce(const glm::vec2 &pos)
        {
            // Use Boussinesq approximation
            // f = [0, -alpha*smokeDensity + beta*(T - T_amb), 0]
            double temperature = getTemperature(pos);
            double smokeDensity = getDensity(pos);

            double yforce = -Eulerian2dPara::boussinesqAlpha * smokeDensity +
                            Eulerian2dPara::boussinesqBeta * (temperature - Eulerian2dPara::ambientTemp);

            return yforce;
        }

        glm::vec2 MACGrid2d::getVorticityN(int i, int j)
        {
            glm::vec2 right = getVorticity(i + 1, j);
            glm::vec2 left = getVorticity(i - 1, j);
            glm::vec2 top = getVorticity(i, j + 1);
            glm::vec2 bottom = getVorticity(i, j - 1);

            double scale = 1.0 / (2 * cellSize);
            double x = scale * (glm::length(right) - glm::length(left));
            double y = scale * (glm::length(top) - glm::length(bottom));

            if (x == y && y == 0)
            {
                return glm::vec2(0, 0);
            }
            glm::vec2 N(x, y);
            return glm::normalize(N);
        }

        glm::vec2 MACGrid2d::getVorticity(int i, int j)
        {
            glm::vec2 right = getVelocity(getRightLine(i + 1, j));
            glm::vec2 left = getVelocity(getLeftLine(i, j));
            glm::vec2 top = getVelocity(getTopLine(i, j + 1));
            glm::vec2 bottom = getVelocity(getBottomLine(i, j));

            double scale = 1.0 / (cellSize);
            double x = scale * (top.y - bottom.y);
            double y = scale * (right.x - left.x);

            return glm::vec2(x, y);
        }

        // 计算涡度约束力
        glm::vec2 MACGrid2d::getConfinementForce(int i, int j)
        {
            glm::vec2 N = getVorticityN(i, j); // 涡度法线
            glm::vec2 w = getVorticity(i, j);  // 涡度
            double crossProduct = N.x * w.y - N.y * w.x;
            return Eulerian2dPara::vorticityConst * cellSize * glm::cross(glm::vec3(N, 0), glm::vec3(w, 0));
        }

        double MACGrid2d::checkDivergence(int i, int j)
        {
            double x1 = mU(i + 1, j);
            double x0 = mU(i, j);
            double y1 = mV(i, j + 1);
            double y0 = mV(i, j);
            double xdiv = x1 - x0;
            double ydiv = y1 - y0;
            double div = (xdiv + ydiv) / cellSize;
            return div;
        }

        // 计算散度
        double MACGrid2d::getDivergence(int i, int j)
        {

            double x1 = isSolidCell(i + 1, j) ? 0.0 : mU(i + 1, j);
            double x0 = isSolidCell(i - 1, j) ? 0.0 : mU(i, j);

            double y1 = isSolidCell(i, j + 1) ? 0.0 : mV(i, j + 1);
            double y0 = isSolidCell(i, j - 1) ? 0.0 : mV(i, j);

            double xdiv = x1 - x0;
            double ydiv = y1 - y0;
            double div = (xdiv + ydiv) / cellSize;

            return div;
        }

        bool MACGrid2d::checkDivergence()
        {
            FOR_EACH_CELL
            {
                double div = checkDivergence(i, j);
                if (fabs(div) > 0.01)
                {
                    printf("Divergence(%d,%d,%d) = %.2f\n", i, j, div);
                    return false;
                }
            }
            return true;
        }

        glm::vec2 MACGrid2d::traceBack(const glm::vec2 &pt, double dt)
        {
            glm::vec2 vel = getVelocity(pt);
            glm::vec2 pos = pt - vel * (float)dt; // 回溯dt时间之前的位置

            // 确保在容器内
            pos[0] = max(0.0, min((dim[0] - 1) * cellSize, pos[0]));
            pos[1] = max(0.0, min((dim[1] - 1) * cellSize, pos[1]));

            // 确保不在固体内
            int i, j;
            if (inSolid(pt, i, j))
            {
                double t = 0;
                if (intersects(pt, vel, i, j, t))
                {
                    pos = pt - vel * (float)t;
                }
                else
                {
                    std::cout << "WARNING: Shouldn't get here." << std::endl;
                }
            }
            return pos;
        }

        bool MACGrid2d::intersects(const glm::vec2 &wPos, const glm::vec2 &wDir, int i, int j, double &time)
        {
            // Transform pos/dir to local coordinates
            // Cell needs to be translated to origin and scaled by 1/theCellSize
            glm::vec2 pos = getCenter(i, j);

            glm::vec2 rayStart = wPos - pos; // * => transform vector
            glm::vec2 rayDir = wDir;         // ^ => transform vector; the symbol was chosen arbitrarily

            // Calculate ray/box intersection test using slabs method
            double tmin = -9999999999.0;
            double tmax = 9999999999.0;

            // Min/Max is the same in every direction for our cube
            double min = -0.5 * cellSize;
            double max = 0.5 * cellSize;

            // For X,Y,Z planes, find intersection with the minimum/maximum boundary values
            // The clever part: the maximum closest intersection will be our cube intersection
            for (int i = 0; i < 3; i++)
            {
                double e = rayStart[i];
                double f = rayDir[i];
                if (fabs(f) > 0.000000001) // Not in ith plane
                {
                    double t1 = (min - e) / f;
                    double t2 = (max - e) / f;
                    if (t1 > t2)
                        std::swap(t1, t2); // Always make t1 the closest for simplicity
                    if (t1 > tmin)
                        tmin = t1;
                    if (t2 < tmax)
                        tmax = t2;
                    if (tmin > tmax)
                        return false;
                    if (tmax < 0)
                        return false;
                }
                // the ray is parallel: check if we're inside the slabs or outside
                else if (e < min || e > max)
                    return false;
            }

            if (tmin >= 0)
            {
                time = tmin;
                return true;
            }
            else
            {
                time = tmax;
                return true;
            }
            return false;
        }

        int MACGrid2d::getIndex(int i, int j)
        {
            if (i < 0 || i > dim[0] - 1)
                return -1;
            if (j < 0 || j > dim[1] - 1)
                return -1;

            int col = i;
            int row = j * dim[0];
            return col + row;
        }

        void MACGrid2d::getCell(int index, int &i, int &j)
        {
            j = (int)index / dim[0]; // row
            i = index - j * dim[0];  // col
        }

        glm::vec2 MACGrid2d::getCenter(int i, int j)
        {
            double xstart = cellSize / 2.0;
            double ystart = cellSize / 2.0;

            double x = xstart + i * cellSize;
            double y = ystart + j * cellSize;
            return glm::vec2(x, y);
        }

        glm::vec2 MACGrid2d::getLeftLine(int i, int j)
        {
            return getCenter(i, j) - glm::vec2(cellSize * 0.5, 0.0);
        }

        glm::vec2 MACGrid2d::getRightLine(int i, int j)
        {
            return getCenter(i, j) + glm::vec2(cellSize * 0.5, 0.0);
        }

        glm::vec2 MACGrid2d::getTopLine(int i, int j)
        {
            return getCenter(i, j) + glm::vec2(0.0, cellSize * 0.5);
        }

        glm::vec2 MACGrid2d::getBottomLine(int i, int j)
        {
            return getCenter(i, j) - glm::vec2(0.0, cellSize * 0.5);
        }

        glm::vec2 MACGrid2d::getVelocity(const glm::vec2 &pt)
        {
            if (inSolid(pt))
            {
                // pt.Print("WARNING: Velocity given point in solid");
                return glm::vec2(0, 0);
            }

            glm::vec2 vel;
            vel[0] = getVelocityX(pt);
            vel[1] = getVelocityY(pt);
            return vel;
        }

        double MACGrid2d::getVelocityX(const glm::vec2 &pt)
        {
            return mU.interpolate(pt);
        }

        double MACGrid2d::getVelocityY(const glm::vec2 &pt)
        {
            return mV.interpolate(pt);
        }

        double MACGrid2d::getTemperature(const glm::vec2 &pt)
        {
            return mT.interpolate(pt);
        }

        double MACGrid2d::getDensity(const glm::vec2 &pt)
        {
            return mD.interpolate(pt);
        }

        int MACGrid2d::numSolidCells()
        {
            int numSolid = 0;
            FOR_EACH_CELL { numSolid += mSolid(i, j); }
            return numSolid;
        }

        bool MACGrid2d::inSolid(const glm::vec2 &pt)
        {
            int i, j, k;
            mSolid.getCell(pt, i, j);
            return isSolidCell(i, j) == 1;
        }

        bool MACGrid2d::inSolid(const glm::vec2 &pt, int &i, int &j)
        {
            mSolid.getCell(pt, i, j);
            return isSolidCell(i, j) == 1;
        }

        int MACGrid2d::isSolidCell(int i, int j)
        {
            bool containerBoundary = (i < 0 || i > dim[0] - 1) ||
                                     (j < 0 || j > dim[1] - 1);

            // Check interior boundaries too
            bool objectBoundary = (mSolid(i, j) == 1);

            return containerBoundary || objectBoundary ? 1 : 0;
        }

        int MACGrid2d::isSolidFace(int i, int j, MACGrid2d::Direction d)
        {
            if (d == X && (i == 0 || i == dim[0]))
                return 1;
            else if (d == Y && (j == 0 || j == dim[1]))
                return 1;

            if (d == X && (mSolid(i, j) || mSolid(i - 1, j)))
                return 1;
            if (d == Y && (mSolid(i, j) || mSolid(i, j - 1)))
                return 1;

            return 0;
        }

        bool MACGrid2d::isNeighbor(int i0, int j0, int i1, int j1)
        {
            if (abs(i0 - i1) == 1 && j0 == j1)
                return true;
            if (abs(j0 - j1) == 1 && i0 == i1)
                return true;
            return false;
        }

        double MACGrid2d::getPressureCoeffBetweenCells(
            int i, int j, int pi, int pj)
        {
            // 同一个cell
            if (i == pi && j == pj) // self
            {
                int numSolidNeighbors = (isSolidCell(i + 1, j) +
                                         isSolidCell(i - 1, j) +
                                         isSolidCell(i, j + 1) +
                                         isSolidCell(i, j - 1));
                // Return number of non-solid boundaries around cel ij
                return 4.0 - numSolidNeighbors; // 二维，4
            }
            // 如果两个是邻居且后者为固体，error
            if (isNeighbor(i, j, pi, pj) && !isSolidCell(pi, pj))
                return -1.0;
            // 不是邻居
            return 0.0;
        }

        glm::vec4 MACGrid2d::getRenderColor(int i, int j)
        {
            double value = mD(i, j);
            return glm::vec4(1.0, 1.0, 1.0, value);
        }

        glm::vec4 MACGrid2d::getRenderColor(const glm::vec2 &pt)
        {
            double value = getDensity(pt);
            return glm::vec4(value, value, value, value);
        }

        // 确保在界内
        bool MACGrid2d::isFace(int i, int j, MACGrid2d::Direction d)
        {
            switch (d)
            {
            case X:
                return (i >= 0 && i < dim[X] + 1 &&
                        j >= 0 && j < dim[Y]);
            case Y:
                return (i >= 0 && i < dim[X] &&
                        j >= 0 && j < dim[Y] + 1);
            }
            printf("Error: bad direction passed to isFace\n");
            return false;
        }
    }
}