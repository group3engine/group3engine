//
// Created by thomas on 30/01/25.
//
#include <iostream>

#include "Renderer.hpp"

// main function
int main() {
    // create the renderer
    {
        GraphicsThings::Renderer renderer;
        GraphicsThings::Light::create_sample_lights(
            renderer.mShadowLightManager);

        // start timer
        auto start = Clock_::now();
        int counter = 0;
        // main loop
        while (renderer.Render()) {
            if (counter++ > 60) {
                counter = 0;
                // get the time taken
                auto stop = Clock_::now();
                // get the time taken
                auto duration =
                    std::chrono::duration_cast<Secondsf_>(stop - start);
                // print the time taken
                std::cout << "Frame took " << duration.count() << " seconds"
                          << std::endl;
                // print the fps
                std::cout << "FPS: " << 60.f / duration.count() << std::endl;
                // reset the timer
                start = Clock_::now();
            }
        }
    }

    return 0;
}