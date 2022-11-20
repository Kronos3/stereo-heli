#ifndef CMPE460_OLED_H
#define CMPE460_OLED_H

#include <Fw/Types/BasicTypes.hpp>
#include <Fw/Types/Assert.hpp>

namespace Heli
{
    class Oled
    {
    public:
        Oled();

        enum {
            WIDTH = 128,
            HEIGHT = 64,
            LINE_LENGTH = 16,      // Maximum number of printable characters in one line
            LINE_MAX_N = 4,        // Maximum number of displayable lines
            CHARACTER_WIDTH = 8,   // Width of character in bytes
            CHARACTER_HEIGHT = 2,  // Height of character in bytes
            LINE_SIZE = WIDTH * CHARACTER_HEIGHT,       // Number of bytes per line
            SCREEN = (WIDTH * HEIGHT) / 8,

            I2C_ADDRESS = 0x3C,
        };

        // Make sure our parameters make sense
        static_assert(CHARACTER_WIDTH * LINE_LENGTH == WIDTH, "Invalid oled width");
        static_assert(LINE_SIZE == CHARACTER_HEIGHT * WIDTH, "Invalid line height");
        static_assert(LINE_SIZE == LINE_LENGTH * CHARACTER_WIDTH * CHARACTER_HEIGHT, "Invalid Line length");
        static_assert(LINE_MAX_N == SCREEN / LINE_SIZE, "Invalid number of lines");

        /**
         * Initialize the OLED I2C device
         */
        void init();

        /**
         * Clear the canvas of all enabled pixels
         */
        void clear();

        /**
         * Convert a camera array canvas into an OLED canvas
         * Draw camera data on the OLED display
         * @param camera_in_array camera data canvas
         * @param max scale of the maximum pix in the array
         */
        void draw_integer(const U16* camera_in_array, U16 max = 0x3FFF);
        void draw_floating(const F64* array, U32 n, F64 max = 1.0, F64 offset = 0.0);

        /**
         * Draw an OLED canvas to the device via I2C
         */
        void draw();

        /**
         * Place an ASCII character into an OLED canvas at a certain coordinate
         * @param row row in text display lattice
         * @param col column in text display lattice
         * @param ascii character to place
         */
        void ascii(U32 row, U32 col, char ascii);

        /**
         * Draw an entire line to the screen at a certain coordinate
         * supports a 4 line display
         * @param row row to write line on
         * @param col column to start line on
         * @param ascii_str line to display
         */
        void line(U32 row, U32 col, const char* ascii_str);

        /**
         * Turn the entire display ON
         */
        void display_on();

        /**
         * Turn the entire display OFF
         */
        void display_off();

    protected:
        virtual void i2c_write(U8* data, U32 len) = 0;

    private:
        void send_command(U8 cmd);
        U8 canvas[SCREEN];
    };
}

#endif //CMPE460_OLED_H
