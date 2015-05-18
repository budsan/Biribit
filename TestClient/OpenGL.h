#ifndef OPENGL_HPP
#define OPENGL_HPP

#include <Biribit/BiribitConfig.h>

////////////////////////////////////////////////////////////
/// This file just includes the OpenGL (GL and GLU) headers,
/// which have actually different paths on each system
////////////////////////////////////////////////////////////
#if defined(SYSTEM_WINDOWS)

    // The Visual C++ version of gl.h uses WINGDIAPI and APIENTRY but doesn't define them
    #ifdef _MSC_VER
        #include <windows.h>
    #endif

    #include <GL/gl.h>
    #include <GL/glu.h>

#elif defined(SYSTEM_LINUX) || defined(SYSTEM_FREEBSD)

    #include <GL/gl.h>
    #include <GL/glu.h>

#elif defined(SYSTEM_MACOS)

    #include <OpenGL/gl.h>
    #include <OpenGL/glu.h>

#endif


#endif // OPENGL_HPP
