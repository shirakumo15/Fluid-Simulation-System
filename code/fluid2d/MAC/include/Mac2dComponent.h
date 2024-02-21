#pragma once
#ifndef __MAC_2D_COMPONENT_H__
#define __MAC_2D_COMPONENT_H__

#include "Renderer.h"
#include "Solver.h"
#include "MACGrid2d.h"

#include "Component.h"
#include "Configure.h"


namespace FluidSimulation {
    namespace MAC2d {
        class Mac2dComponent : public Glb::Component {
        public:
            Renderer* renderer;
            Solver* solver;
            MACGrid2d* grid;

            Mac2dComponent(char* description, int id) {
                this->description = description;
                this->id = id;
                renderer = NULL;
                solver = NULL;
                grid = NULL;
                is3D = false;
            }
            virtual void shutDown();
            virtual void init();
            virtual void simulate();
            virtual GLuint getRenderedTexture();
        };
    }
}


#endif