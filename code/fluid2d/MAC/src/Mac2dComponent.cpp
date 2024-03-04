#include "Mac2dComponent.h"

namespace FluidSimulation {
    namespace MAC2d {

        void Mac2dComponent::shutDown() {
            delete renderer, solver, grid;
            renderer = NULL;
            solver = NULL;
            grid = NULL;
        }

        void Mac2dComponent::init() {

            if (renderer != NULL || solver != NULL || grid != NULL) {
                delete renderer, solver, grid;
                renderer = NULL;
                solver = NULL;
                grid = NULL;
            }

            Glb::Timer::getInstance().clear();

            grid = new MACGrid2d();
            renderer = new Renderer();
            solver = new Solver(*grid);

        }

        void Mac2dComponent::simulate() {
            grid->updateSources();
            solver->solve();

        }

        GLuint Mac2dComponent::getRenderedTexture()
        {
            Glb::Timer::getInstance().start();
            renderer->draw(*grid);
            Glb::Timer::getInstance().recordTime("rendering");
            return renderer->getTextureID();
        }
    }
}