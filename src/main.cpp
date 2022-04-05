#include <stdio.h>
#include <stdlib.h>
#include <GLFW/glfw3.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include "video_reader.hpp"


int main(int argc, const char** argv) {
    GLFWwindow* window;

    if (!glfwInit()) {
        printf("Couldn't init GLFW\n");
        return 1;
    }

    window = glfwCreateWindow(800, 480, "Hello World", NULL, NULL);
    if (!window) {
        printf("Couldn't open window\n");
        return 1;
    }

    ShutterAndroidJNI::LibavVideoDecoder decoder = ShutterAndroidJNI::LibavVideoDecoder();

    std::vector<std::string> src = {
            "/Users/anshulsaraf/Downloads/40_sss_loop.mov", // 4k file - 0
            "/Users/anshulsaraf/Downloads/colour_particals.mov", // 1
            "/Users/anshulsaraf/Downloads/2a3d50ef_1647944187358_sc.webm", // 2 // looping solved
            "/Users/anshulsaraf/Downloads/file_example_AVI_640_800kB.avi", // 3
            "/Users/anshulsaraf/Downloads/sample_640x360.hevc", // not looping // 4 // av_format_ctx-> start_time and duration == (int64 overflow)
            "/Users/anshulsaraf/Downloads/sample_640x360.flv", // 5
            "/Users/anshulsaraf/Downloads/sample-5s.mp4", // 6
            "/Users/anshulsaraf/Downloads/sample_640x360.mkv" // 7
    };

    decoder.InitDecoder(src[2], false); // loop false for 60 fps // first loop

    glfwMakeContextCurrent(window);

    // Generate texture
    GLuint tex_handle;
    glGenTextures(1, &tex_handle);
    glBindTexture(GL_TEXTURE_2D, tex_handle);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    // Allocate frame buffer
    constexpr int ALIGNMENT = 2048;
    const int frame_width = decoder.width;
    const int frame_height = decoder.height;
    uint8_t* frame_data;
    if (posix_memalign((void**)&frame_data, ALIGNMENT, decoder.getBufferSize()) != 0) {
        printf("Couldn't allocate frame buffer\n");
        return 1;
    }

    int frames = 0;
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Set up orphographic projection
        int window_width, window_height;
        glfwGetFramebufferSize(window, &window_width, &window_height);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, window_width, window_height, 0, -1, 1);
        glMatrixMode(GL_MODELVIEW);

        // Read a new frame and load it into texture
        double pt_in_seconds = -1; /* = pts * (double)vr_state.time_base.num / (double)vr_state.time_base.den;*/

        if (!decoder.Decode(frames++, frame_data, pt_in_seconds)) {
            printf("Couldn't load video frame\n");
            break;
        }

        static bool first_frame = true;
        if (first_frame) {
            glfwSetTime(0.0);
            first_frame = false;
        }
//        printf("pts : %f", pt_in_seconds);
        while (pt_in_seconds != -1 && pt_in_seconds > glfwGetTime()) {
            glfwWaitEventsTimeout(pt_in_seconds - glfwGetTime());
        }

        glBindTexture(GL_TEXTURE_2D, tex_handle);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, frame_width, frame_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, frame_data);

        // Render whatever you want
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, tex_handle);
        glBegin(GL_QUADS);
            glTexCoord2d(0,0); glVertex2i(200, 200);
            glTexCoord2d(1,0); glVertex2i(200 + frame_width, 200);
            glTexCoord2d(1,1); glVertex2i(200 + frame_width, 200 + frame_height);
            glTexCoord2d(0,1); glVertex2i(200, 200 + frame_height);
        glEnd();
        glDisable(GL_TEXTURE_2D);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    decoder.ReleaseDecoder();
    free(frame_data);

    return 0;
}

