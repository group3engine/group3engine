    #ifndef GROUP3ENGINE_GLFW_HPP
    #define GROUP3ENGINE_GLFW_HPP

    #include <GLFW/glfw3.h>

    #include "InputData.hpp"

    #include "SDL.hpp"

    extern InputData gInputData;
    extern SDL_INPUT::InputData gSDLInputData;

    void PollInputEvents();

    class Platform {
      private:
        Platform() = default;
        ~Platform() = default;

      public:
        Platform(const Platform &) = delete;
        Platform &operator=(const Platform &) = delete;

        static Platform &get() {
            static Platform instance;
            return instance;
        }

        void StartUp(int windowWidth, int windowHeight);
        void ShutDown();

      public:
        GLFWwindow *window;
    };
    #endif // GROUP3ENGINE_GLFW_HPP
