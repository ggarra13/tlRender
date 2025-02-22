
#include <tlDevice/OutputData.h>

#include <tlGL/GL.h>

namespace tl
{
    namespace device
    {
        GLenum getPackPixelsFormat(PixelType value)
        {
            const std::array<GLenum, static_cast<size_t>(PixelType::Count)> data =
            {
                GL_NONE,
                GL_BGRA,
                GL_BGRA,
                GL_RGB,
                GL_RGB,
                GL_RGB,
                //GL_RGBA,   
                GL_RGB,
                GL_RGB,
                GL_RGBA,
                GL_RGB,
                GL_RGBA,
                GL_RGB,
                GL_RGBA,
                GL_RGBA,
                GL_RGBA,
            };
            return data[static_cast<size_t>(value)];
        }

        GLenum getPackPixelsType(PixelType value)
        {
            const std::array<GLenum, static_cast<size_t>(PixelType::Count)> data =
            {
                GL_NONE,
                GL_UNSIGNED_BYTE,
                GL_UNSIGNED_BYTE,
                GL_UNSIGNED_SHORT,
                GL_UNSIGNED_SHORT,
                GL_UNSIGNED_SHORT,
                //GL_UNSIGNED_INT_10_10_10_2,
                GL_UNSIGNED_SHORT,
                GL_UNSIGNED_SHORT,
                GL_UNSIGNED_BYTE,
                GL_UNSIGNED_SHORT,
                GL_UNSIGNED_SHORT,
                GL_UNSIGNED_BYTE,
                GL_UNSIGNED_BYTE,
                GL_UNSIGNED_BYTE,
                GL_UNSIGNED_BYTE,
            };
            return data[static_cast<size_t>(value)];
        }

        GLint getPackPixelsAlign(PixelType value)
        {
            const std::array<GLint, static_cast<size_t>(PixelType::Count)> data =
            {
                0,
                4,
                4,
                1,
                1,
                1,
                //! \bug OpenGL only allows alignment values of 1, 2, 4, and 8.
                //8, // 128,
                1,
                1,
                // These are NDI formats
                1,
                1,
                1,
                1,
                4,
                4,
                4,
            };
            return data[static_cast<size_t>(value)];
        }

        GLint getPackPixelsSwap(PixelType value)
        {
            const std::array<GLint, static_cast<size_t>(PixelType::Count)> data =
            {
                GL_FALSE,
                GL_FALSE,
                GL_FALSE,
                GL_FALSE,
                GL_FALSE,
                GL_FALSE,
                //GL_FALSE,
                GL_FALSE,
                GL_FALSE,
                // These are NDI formats
                GL_FALSE,
                GL_FALSE,
                GL_FALSE,
                GL_FALSE,
                GL_FALSE,
                GL_FALSE,
                GL_FALSE,
            };
            return data[static_cast<size_t>(value)];
        }
    }
}
