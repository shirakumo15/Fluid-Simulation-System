#include "MACGrid3d.h"
#include "Configure.h"
#include <math.h>
#include <map>
#include <stdio.h>

namespace FluidSimulation
{
    namespace Eulerian3d
    {

        MACGrid3d::MACGrid3d()
        {
            cellSize = Eulerian3dPara::theCellSize3d;
            dim[0] = Eulerian3dPara::theDim3d[0];
            dim[1] = Eulerian3dPara::theDim3d[1];
            dim[2] = Eulerian3dPara::theDim3d[2];
            initialize();
        }

        MACGrid3d::MACGrid3d(const MACGrid3d &orig)
        {
            mU = orig.mU;
            mV = orig.mV;
            mW = orig.mW;
            mD = orig.mD;
            mT = orig.mT;
            mSolid = orig.mSolid;
        }

        MACGrid3d &MACGrid3d::operator=(const MACGrid3d &orig)
        {
            if (&orig == this)
            {
                return *this;
            }
            mU = orig.mU;
            mV = orig.mV;
            mW = orig.mW;
            mD = orig.mD;
            mT = orig.mT;
            mSolid = orig.mSolid;

            return *this;
        }

        MACGrid3d::~MACGrid3d()
        {
        }

        void MACGrid3d::reset()
        {
            mU.initialize(0.0);
            mV.initialize(0.0);
            mW.initialize(0.0);
            mD.initialize(0.0);
            mT.initialize(Eulerian3dPara::ambientTemp);

            // Set default vel to make things more interesting and avoid degenerate fluid case
            /*int sourcei = (int)dim[0] / 2;
            int sourcek = (int)dim[2] / 2;
            mU(sourcei, 0, sourcek) = cos(0.3);
            mV(sourcei, 0, sourcek) = sin(0.3);*/
        }

        void MACGrid3d::updateSources()
        {

            // source的位置
            int sourcei = (int)dim[0] / 2;
            int sourcej = (int)dim[1] / 2;

            mW(sourcei, sourcej, 0) = Eulerian3dPara::sourceVelocity;
            mT(sourcei, sourcej, 0) = 1.0f;
            mD(sourcei, sourcej, 0) = 1.0f;
        }

        void MACGrid3d::createSolids()
        {
            mSolid.initialize();

            // int j = dim[1] / 2;
            // int quarter1 = dim[0] / 4;
            // int quarter2 = 3 * dim[0] / 4;
            // int size = 4;
            // for (int i = size; i < dim[0] - size; i++)
            //{
            //     if (i > quarter1 - size && i < quarter1 + size) continue; // Create holes
            //     if (i > quarter2 - size && i < quarter2 + size) continue; // Create holes
            //     for (int k = 0; k < dim[2]; k++)
            //     {
            //         mSolid(i, j, k) = 1;
            //     }
            // }
        }

        void MACGrid3d::initialize()
        {
            reset();
            createSolids();
        }

        // 根据温度差异计算 Boussinesq Force
        double MACGrid3d::getBoussinesqForce(const glm::vec3 &pos)
        {
            // Use Boussinesq approximation
            // f = [0, -alpha*smokeDensity + beta*(T - T_amb), 0]
            double temperature = getTemperature(pos);
            double smokeDensity = getDensity(pos);

            double zforce = -Eulerian3dPara::boussinesqAlpha * smokeDensity +
                            Eulerian3dPara::boussinesqBeta * (temperature - Eulerian3dPara::ambientTemp);

            return zforce;
        }

        glm::vec3 MACGrid3d::getVorticityN(int i, int j, int k)
        {
            glm::vec3 right = getVorticity(i, j + 1, k);
            glm::vec3 left = getVorticity(i, j - 1, k);
            glm::vec3 top = getVorticity(i, j, k + 1);
            glm::vec3 bottom = getVorticity(i, j, k - 1);
            glm::vec3 front = getVorticity(i + 1, j, k);
            glm::vec3 back = getVorticity(i - 1, j, k);

            double scale = 1.0 / (2 * cellSize);
            double x = scale * (glm::length(front) - glm::length(back));
            double y = scale * (glm::length(right) - glm::length(left));
            double z = scale * (glm::length(top) - glm::length(bottom));

            if (x == y && y == z && z == 0)
            {
                return glm::vec3(0, 0, 0);
            }
            glm::vec3 N(x, y, z);
            return glm::normalize(N);
        }

        glm::vec3 MACGrid3d::getVorticity(int i, int j, int k)
        {
            glm::vec3 right = getVelocity(getRightFace(i, j + 1, k));
            glm::vec3 left = getVelocity(getLeftFace(i, j, k));
            glm::vec3 top = getVelocity(getTopFace(i, j, k + 1));
            glm::vec3 bottom = getVelocity(getBottomFace(i, j, k));
            glm::vec3 front = getVelocity(getFrontFace(i + 1, j, k));
            glm::vec3 back = getVelocity(getBackFace(i, j, k));

            double scale = 1.0 / (cellSize);
            double x = scale * (right[Z] - left[Z] - (top[Y] - bottom[Y]));
            double y = scale * (top[X] - bottom[X] - (front[Z] - back[Z]));
            double z = scale * (front[Y] - back[Y] - (right[X] - left[X]));

            return glm::vec3(x, y, z);
        }

        // 计算涡度约束力
        glm::vec3 MACGrid3d::getConfinementForce(int i, int j, int k)
        {
            glm::vec3 N = getVorticityN(i, j, k); // 涡度法线
            glm::vec3 w = getVorticity(i, j, k);  // 涡度
            return (float)(Eulerian3dPara::vorticityConst * cellSize) * glm::cross(N, w);
        }

        double MACGrid3d::checkDivergence(int i, int j, int k)
        {
            double x1 = mU(i + 1, j, k);
            double x0 = mU(i, j, k);

            double y1 = mV(i, j + 1, k);
            double y0 = mV(i, j, k);

            double z1 = mW(i, j, k + 1);
            double z0 = mW(i, j, k);

            double xdiv = x1 - x0;
            double ydiv = y1 - y0;
            double zdiv = z1 - z0;
            double div = (xdiv + ydiv + zdiv) / cellSize;
            return div;
        }

        // 计算散度
        double MACGrid3d::getDivergence(int i, int j, int k)
        {

            double x1 = isSolidCell(i + 1, j, k) ? 0.0 : mU(i + 1, j, k);
            double x0 = isSolidCell(i - 1, j, k) ? 0.0 : mU(i, j, k);

            double y1 = isSolidCell(i, j + 1, k) ? 0.0 : mV(i, j + 1, k);
            double y0 = isSolidCell(i, j - 1, k) ? 0.0 : mV(i, j, k);

            double z1 = isSolidCell(i, j, k + 1) ? 0.0 : mW(i, j, k + 1);
            double z0 = isSolidCell(i, j, k - 1) ? 0.0 : mW(i, j, k);

            double xdiv = x1 - x0;
            double ydiv = y1 - y0;
            double zdiv = z1 - z0;
            double div = (xdiv + ydiv + zdiv) / cellSize;

            //   printf("Cell %d %d %d = %.2f\n", i, j, k, div);

            return div;
        }

        bool MACGrid3d::checkDivergence()
        {
            FOR_EACH_CELL
            {
                double div = checkDivergence(i, j, k);
                if (fabs(div) > 0.01)
                {
                    printf("Divergence(%d,%d,%d) = %.2f\n", i, j, k, div);
                    return false;
                }
            }
            return true;
        }

        glm::vec3 MACGrid3d::traceBack(const glm::vec3 &pt, double dt)
        {
            glm::vec3 vel = getVelocity(pt);
            glm::vec3 pos = pt - vel * (float)dt;

            // 1. Clamp pos to insides of our container
            pos[0] = max(0.0, min((dim[0] - 1) * cellSize, pos[0]));
            pos[1] = max(0.0, min((dim[1] - 1) * cellSize, pos[1]));
            pos[2] = max(0.0, min((dim[2] - 1) * cellSize, pos[2]));

            // 2. Push point outside of our interior solids
            int i, j, k;
            if (inSolid(pt, i, j, k))
            {
                double t = 0;
                if (intersects(pt, vel, i, j, k, t))
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

        bool MACGrid3d::intersects(const glm::vec3 &wPos, const glm::vec3 &wDir, int i, int j, int k, double &time)
        {
            // Transform pos/dir to local coordinates
            // Cell needs to be translated to origin and scaled by 1/theCellSize
            glm::vec3 pos = getCenter(i, j, k);

            glm::vec3 rayStart = wPos - pos; // * => transform vector
            glm::vec3 rayDir = wDir;         // ^ => transform vector; the symbol was chosen arbitrarily

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

        int MACGrid3d::getIndex(int i, int j, int k)
        {
            if (i < 0 || i > dim[0] - 1)
                return -1;
            if (j < 0 || j > dim[1] - 1)
                return -1;
            if (k < 0 || k > dim[2] - 1)
                return -1;

            int col = i;
            int row = k * dim[0];
            int stack = j * dim[0] * dim[2];
            return col + row + stack;
        }

        void MACGrid3d::getCell(int index, int &i, int &j, int &k)
        {
            j = (int)index / (dim[0] * dim[2]);              // stack
            k = (int)(index - j * dim[0] * dim[2]) / dim[0]; // row
            i = index - j * dim[0] * dim[2] - k * dim[0];    // col
        }

        glm::vec3 MACGrid3d::getCenter(int i, int j, int k)
        {
            double xstart = cellSize / 2.0;
            double ystart = cellSize / 2.0;
            double zstart = cellSize / 2.0;

            double x = xstart + i * cellSize;
            double y = ystart + j * cellSize;
            double z = zstart + k * cellSize;
            return glm::vec3(x, y, z);
        }

        glm::vec3 MACGrid3d::getLeftFace(int i, int j, int k)
        {
            return getCenter(i, j, k) - glm::vec3(0.0, cellSize * 0.5, 0.0);
        }

        glm::vec3 MACGrid3d::getRightFace(int i, int j, int k)
        {
            return getCenter(i, j, k) + glm::vec3(0.0, cellSize * 0.5, 0.0);
        }

        glm::vec3 MACGrid3d::getTopFace(int i, int j, int k)
        {
            return getCenter(i, j, k) + glm::vec3(0.0, 0.0, cellSize * 0.5);
        }

        glm::vec3 MACGrid3d::getBottomFace(int i, int j, int k)
        {
            return getCenter(i, j, k) - glm::vec3(0.0, 0.0, cellSize * 0.5);
        }

        glm::vec3 MACGrid3d::getFrontFace(int i, int j, int k)
        {
            return getCenter(i, j, k) + glm::vec3(cellSize * 0.5, 0.0, 0.0);
        }

        glm::vec3 MACGrid3d::getBackFace(int i, int j, int k)
        {
            return getCenter(i, j, k) - glm::vec3(cellSize * 0.5, 0.0, 0.0);
        }

        glm::vec3 MACGrid3d::getVelocity(const glm::vec3 &pt)
        {
            if (inSolid(pt))
            {
                // pt.Print("WARNING: Velocity given point in solid");
                return glm::vec3(0);
            }

            glm::vec3 vel;
            vel[0] = getVelocityX(pt);
            vel[1] = getVelocityY(pt);
            vel[2] = getVelocityZ(pt);
            return vel;
        }

        double MACGrid3d::getVelocityX(const glm::vec3 &pt)
        {
            return mU.interpolate(pt);
        }

        double MACGrid3d::getVelocityY(const glm::vec3 &pt)
        {
            return mV.interpolate(pt);
        }

        double MACGrid3d::getVelocityZ(const glm::vec3 &pt)
        {
            return mW.interpolate(pt);
        }

        double MACGrid3d::getTemperature(const glm::vec3 &pt)
        {
            return mT.interpolate(pt);
        }

        double MACGrid3d::getDensity(const glm::vec3 &pt)
        {
            return mD.interpolate(pt);
        }

        int MACGrid3d::numSolidCells()
        {
            int numSolid = 0;
            FOR_EACH_CELL { numSolid += mSolid(i, j, k); }
            return numSolid;
        }

        bool MACGrid3d::inSolid(const glm::vec3 &pt)
        {
            int i, j, k;
            mSolid.getCell(pt, i, j, k);
            return isSolidCell(i, j, k) == 1;
        }

        bool MACGrid3d::inSolid(const glm::vec3 &pt, int &i, int &j, int &k)
        {
            mSolid.getCell(pt, i, j, k);
            return isSolidCell(i, j, k) == 1;
        }

        int MACGrid3d::isSolidCell(int i, int j, int k)
        {
            bool containerBoundary = (i < 0 || i > dim[0] - 1) ||
                                     (j < 0 || j > dim[1] - 1) ||
                                     (k < 0 || k > dim[2] - 1);

            // Check interior boundaries too
            bool objectBoundary = (mSolid(i, j, k) == 1);

            return containerBoundary || objectBoundary ? 1 : 0;
        }

        int MACGrid3d::isSolidFace(int i, int j, int k, MACGrid3d::Direction d)
        {
            if (d == X && (i == 0 || i == dim[0]))
                return 1;
            else if (d == Y && (j == 0 || j == dim[1]))
                return 1;
            else if (d == Z && (k == 0 || k == dim[2]))
                return 1;

            if (d == X && (mSolid(i, j, k) || mSolid(i - 1, j, k)))
                return 1;
            if (d == Y && (mSolid(i, j, k) || mSolid(i, j - 1, k)))
                return 1;
            if (d == Z && (mSolid(i, j, k) || mSolid(i, j, k - 1)))
                return 1;

            return 0;
        }

        bool MACGrid3d::isNeighbor(int i0, int j0, int k0, int i1, int j1, int k1)
        {
            if (abs(i0 - i1) == 1 && j0 == j1 && k0 == k1)
                return true;
            if (abs(j0 - j1) == 1 && i0 == i1 && k0 == k1)
                return true;
            if (abs(k0 - k1) == 1 && j0 == j1 && i0 == i1)
                return true;
            return false;
        }

        double MACGrid3d::getPressureCoeffBetweenCells(
            int i, int j, int k, int pi, int pj, int pk)
        {
            if (i == pi && j == pj && k == pk) // self
            {
                int numSolidNeighbors = (isSolidCell(i + 1, j, k) +
                                         isSolidCell(i - 1, j, k) +
                                         isSolidCell(i, j + 1, k) +
                                         isSolidCell(i, j - 1, k) +
                                         isSolidCell(i, j, k + 1) +
                                         isSolidCell(i, j, k - 1));
                // Return number of non-solid boundaries around cel ijk
                return 6.0 - numSolidNeighbors;
            }
            if (isNeighbor(i, j, k, pi, pj, pk) && !isSolidCell(pi, pj, pk))
                return -1.0;
            return 0.0;
        }

        glm::vec4 MACGrid3d::getRenderColor(int i, int j, int k)
        {
            double value = mD(i, j, k);
            return glm::vec4(1.0, 1.0, 1.0, value);
        }

        glm::vec4 MACGrid3d::getRenderColor(const glm::vec3 &pt)
        {
            double value = getDensity(pt);
            return glm::vec4(value, value, value, value);
        }

        // 确保在界内
        bool MACGrid3d::isFace(int i, int j, int k, MACGrid3d::Direction d)
        {
            switch (d)
            {
            case X:
                return (i >= 0 && i < dim[X] + 1 &&
                        j >= 0 && j < dim[Y] &&
                        k >= 0 && k < dim[Z]);
            case Y:
                return (i >= 0 && i < dim[X] &&
                        j >= 0 && j < dim[Y] + 1 &&
                        k >= 0 && k < dim[Z]);
            case Z:
                return (i >= 0 && i < dim[X] &&
                        j >= 0 && j < dim[Y] &&
                        k >= 0 && k < dim[Z] + 1);
            }
            printf("Error: bad direction passed to isFace\n");
            return false;
        }
    }
}