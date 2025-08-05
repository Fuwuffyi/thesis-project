#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <cstdlib>

int main() {
   if (!glfwInit()) {
      return EXIT_FAILURE;
   }
   glfwTerminate();
   return EXIT_SUCCESS;
}
