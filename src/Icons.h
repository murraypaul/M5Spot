#pragma once

#include "icons8_prev_64_4bpp.bmp.h"
#include "icons8_first_64_4bpp.bmp.h"
#include "icons8_resume_64_4bpp.bmp.h"
#include "icons8_right_button_64_4bpp.bmp.h"
#include "icons8_automatic_64_4bpp.bmp.h"
#include "icons8_close_window_64_4bpp.bmp.h"
#include "icon_layout_landscape_big_4bpp.bmp.h"
#include "icon_layout_landscape_small_4bpp.bmp.h"
#include "icon_layout_portrait_big_4bpp.bmp.h"
#include "icon_rotate_4bpp.bmp.h"
#include "icon_image_size_small_4bpp.bmp.h"
#include "icon_image_size_medium_4bpp.bmp.h"
#include "icon_image_size_large_4bpp.bmp.h"

// From M5EPD_Canvas.cpp
const uint8_t kGrayScaleMap[] = {
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
    11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

bool drawIcon( M5EPD_Canvas& canvas, const unsigned char* bmpFS, size_t size, uint16_t x, uint16_t y )
{
    if ((x >= canvas.width()) || (y >= canvas.height()))
        return 0;

    if( bmpFS == 0 )
        return 0;

//    log_i("drawIcon, data size = %d", size);

    uint32_t seekOffset;
    uint16_t w, h, row, col;
    uint8_t r, g, b, a = 0;

    uint32_t startTime = millis();

    size_t data_pos = 0;
    auto read16 = [&data_pos](const unsigned char* bmpFS) -> uint16_t
    {
        uint16_t result;
        ((uint8_t *)&result)[0] = bmpFS[data_pos++]; // LSB
        ((uint8_t *)&result)[1] = bmpFS[data_pos++]; // MSB
//        log_i("Read %d",result);
        return result;
    };
    auto read32 = [&data_pos](const unsigned char* bmpFS) -> uint32_t
    {
        uint32_t result;
        ((uint8_t *)&result)[0] = bmpFS[data_pos++]; // LSB
        ((uint8_t *)&result)[1] = bmpFS[data_pos++];
        ((uint8_t *)&result)[2] = bmpFS[data_pos++];
        ((uint8_t *)&result)[3] = bmpFS[data_pos++]; // MSB
//        log_i("Read %d",result);
        return result;
    };
    if (read16(bmpFS) == 0x4D42)
    {
        read32(bmpFS);
        read32(bmpFS);
        seekOffset = read32(bmpFS);
        read32(bmpFS);
        w = read32(bmpFS);
        h = read32(bmpFS);

        uint8_t planes = read16(bmpFS);
        uint8_t bpp = read16(bmpFS);
        uint8_t compression = read32(bmpFS);
        if ( (planes == 1) && ( (bpp == 1 && w%8 == 0) || (bpp == 4 && w%2 == 0) || bpp == 24 || bpp == 32 ) && ( compression == 0 || compression == 3 ) )
        {
            y += h - 1;

            data_pos = seekOffset;

            uint16_t padding = 0;
            switch( bpp )
            {
                case 24:
                    padding = (4 - ((w * 3) & 3)) & 3;
                    break;
                case 1:
                    padding = (w%32==0?0:4-((w%32)/8));
                    break;
                case 4:
                    padding = (w%8==0?0:4-((w%8)/2));
                    break;
            }

            uint8_t* bptr = (uint8_t*)(bmpFS + data_pos); 
            int colskip = 1;
            switch( bpp )
            {
                case 1: colskip = 8; break;
                case 4: colskip = 2; break;
            }
            for (row = 0; row < h; row++)
            {
//                log_i("Row %d, buffer offset = %d",row,bptr-bmpFS);
                // Convert 24 to 16 bit colours
                for (col = 0; col < w; col += colskip)
                {
                    switch( bpp )
                    {
                    case 1:
                    {
                        uint8_t data = *bptr++;
                        for( int i = 0 ; i < 8 ; i++ )
                        {
                            if( data & (1 << (7-i)) )
                                canvas.drawPixel(x + col + i, y, 0);
                            else
                                canvas.drawPixel(x + col + i, y, 15);
                        }
                        break;
                    } 
                    case 4:
                    {
                        uint8_t data = *bptr++;
                        uint8_t col1 = (data & 0xf0) >> 4;
                        uint8_t col2 = (data & 0x0f);
                        canvas.drawPixel(x + col + 0, y, 15 - col1);
                        canvas.drawPixel(x + col + 1, y, 15 - col2);
                        break;
                    }
                    default:
                    {
                        b = *bptr++;
                        g = *bptr++;
                        r = *bptr++;
                        if( bpp == 32 ) a = *bptr++;
                        if( b != 0 || g != 0 || r != 0 || a != 0 )
//                            log_i("%d,%d (%d,%d,%d,%d)", row, col, r, g, b, a);
                        if( bpp == 32 )
                            canvas.drawPixel(x + col, y, kGrayScaleMap[a]);
                        else
                            canvas.drawPixel(x + col, y, kGrayScaleMap[(r * 38 + g * 75 + b * 15) >> 7]);
                        break;
                    }
                    }
                }

                data_pos += padding;

                // Push the pixel row to screen, pushImage will crop the line if needed
                // y is decremented as the BMP image is drawn bottom up
                y--;
            }
//            log_d("Loaded in %lu ms", millis() - startTime);
        }
        else
        {
            log_e("BMP format not recognized.");
            return 0;
        }
            
    }
//    bmpFS.close();
    return 1;
}