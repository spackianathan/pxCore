// pxCore CopyRight 2007-2015 John Robinson
// pxUtil.cpp

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <png.h>

#include "rtLog.h"
#include "pxCore.h"
#include "pxOffscreen.h"
#include "pxUtil.h"


rt_error pxLoadImage(char* imageData, size_t imageDataSize, pxOffscreen& o)
{
  return pxLoadPNGImage(imageData, imageDataSize, o);
}

rt_error pxLoadImage(const char* filename, pxOffscreen& b)
{
  return pxLoadPNGImage(filename, b);
}

rt_error pxStoreImage(const char* filename, pxBuffer& b)
{
  return pxStorePNGImage(filename, b);
}

rt_error pxLoadPNGImage(const char* filename, pxOffscreen& o)
{
  if(!filename)
  {
    rtLogError("FATAL: Invalid argument - filename = NULL");
    return RT_FAIL;
  }

  png_structp png_ptr;
  png_infop info_ptr;
  //  int number_of_passes;
  png_bytep * row_pointers;

  // open file and test for it being a png
  FILE *fp = fopen(filename, "rb");

  if(!fp)
  {
    rtLogError("FATAL: Error opening \"%s\"...", filename);
    return RT_FAIL;
  }

  unsigned char header[8];    // 8 is the maximum size that can be checked

  size_t bytes_read = fread(header, sizeof(unsigned char), 8, fp);

  if(bytes_read != sizeof(header))
  {
    rtLogError("FATAL: Failed to read PNG header from \"%s\" ", filename);
    return RT_FAIL;
  }

  // test PNG header
  if(png_sig_cmp(header, 0, 8) != 0)
  {
    rtLogError("FATAL: Invalid PNG header from \"%s\" ", filename);
    return RT_FAIL;
  }

  // initialize stuff
  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

  if(!png_ptr)
  {
    rtLogError("FATAL: png_create_read_struct() - FAILED");
    return RT_FAIL;
  }

  info_ptr = png_create_info_struct(png_ptr);

  if(!info_ptr)
  {
    rtLogError("FATAL: png_create_info_struct() - FAILED");
    return RT_FAIL;
  }

  if (!setjmp(png_jmpbuf(png_ptr)))
  {
    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, 8);

    png_read_info(png_ptr, info_ptr);

    int width  = png_get_image_width(png_ptr, info_ptr);
    int height = png_get_image_height(png_ptr, info_ptr);

    png_byte color_type = png_get_color_type(png_ptr, info_ptr);
    //png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);

    if (color_type == PNG_COLOR_TYPE_PALETTE)
    {
      png_set_palette_to_rgb(png_ptr);
    }

#if 0
    if (info_ptr->bit_depth < 8)
    {
      png_set_packing(png_ptr);
#endif

#if 0		
      if (color_type == PNG_COLOR_TYPE_GRAY)
        png_set_gray_1_2_4_to_8(png_ptr);
    }
#endif	    
    if (color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    {
      png_set_gray_to_rgb(png_ptr);
    }

    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
    {
      png_set_tRNS_to_alpha(png_ptr);
    }

#if 0
    if (info_ptr->bit_depth == 16)
      png_set_strip_16(png_ptr);
#endif
    png_set_bgr(png_ptr);
    png_set_add_alpha(png_ptr, 0xff, PNG_FILLER_AFTER);

    o.init(width, height);

    //	    number_of_passes = png_set_interlace_handling(png_ptr);
    png_read_update_info(png_ptr, info_ptr);

    // read file
    if (!setjmp(png_jmpbuf(png_ptr)))
    {
      row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);

      if (row_pointers)
      {
#if 0
        for (y=0; y<height; y++)
        {
          row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr,info_ptr));
        }
#endif
#if 1
        for (int y = 0; y < height; y++)
        {
          row_pointers[y] = (png_byte*)o.scanline(y);
        }
#endif

        png_read_image(png_ptr, row_pointers);
        free(row_pointers);
      }
    }

    fclose(fp);
  }

  return RT_OK;
}

rt_error pxStorePNGImage(const char* filename, pxBuffer& b, bool /*grayscale*/, bool /*alpha*/)
{
  png_structp png_ptr;
  png_infop info_ptr;
  png_bytep * row_pointers;

  // create file
  FILE *fp = fopen(filename, "wb");

  if (fp)
  {
    // initialize stuff
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if(!png_ptr)
    {
      rtLogError("FATAL: png_create_write_struct() - FAILED");
      return RT_FAIL;
    }

    info_ptr = png_create_info_struct(png_ptr);

    if(!info_ptr)
    {
      rtLogError("FATAL: png_create_info_struct() - FAILED");
      return RT_FAIL;
    }

    if (!setjmp(png_jmpbuf(png_ptr)))
    {
      png_init_io(png_ptr, fp);

      // write header
      if (!setjmp(png_jmpbuf(png_ptr)))
      {
#if 0
        png_byte color_type = (grayscale?PNG_COLOR_MASK_ALPHA:0) |
            (alpha?PNG_COLOR_MASK_ALPHA:0);
#endif
        png_set_bgr(png_ptr);

        png_set_IHDR(png_ptr, info_ptr, b.width(), b.height(),
                     8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
                     PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

        png_write_info(png_ptr, info_ptr);


        // setjmp() ... needed for 'libpng' error handling...

        // write bytes
        if (!setjmp(png_jmpbuf(png_ptr)))
        {
          row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * b.height());

          if (row_pointers)
          {
            for (int y = 0; y < b.height(); y++)
            {
              row_pointers[y] = (png_byte*)b.scanline(y);
            }

            png_write_image(png_ptr, row_pointers);

            // end write
            if (!setjmp(png_jmpbuf(png_ptr)))
            {
              png_write_end(png_ptr, NULL);
            }

            free(row_pointers);

          }//ENDIF - row_pointers
        }
      }
    }


#if 0
    // cleanup heap allocation
    for (y=0; y<height; y++)
    {
      free(row_pointers[y]);
    }
#endif
    //		free(row_pointers);

    fclose(fp);
  }
  return RT_OK;
}

rt_error pxLoadJPGImage(char* /*filename*/, pxOffscreen& /*o*/)
{
  return RT_FAIL;
}

rt_error pxStoreJPGImage(char* /*filename*/, pxBuffer& /*b*/)
{
  return RT_FAIL;
}

struct PngStruct
{
  PngStruct(char* data, size_t dataSize)
    : imageData(data), imageDataSize(dataSize), readPosition(0)
  {
  }

  char* imageData;
  size_t imageDataSize;
  int readPosition;
};


void readPngData(png_structp pngPtr, png_bytep data, png_size_t length)
{
  png_voidp          a = png_get_io_ptr(pngPtr);
  PngStruct* pngStruct = (PngStruct*)a;

  memcpy ( (char*)data, pngStruct->imageData+pngStruct->readPosition, length );
  pngStruct->readPosition += length;
}


rt_error pxLoadPNGImage(char* imageData, size_t imageDataSize, pxOffscreen& o)
{
  rt_error e = RT_FAIL;

  png_structp png_ptr;
  png_infop info_ptr;
  //  int number_of_passes;
  png_bytep * row_pointers;
  PngStruct pngStruct(imageData, imageDataSize);

  if(!imageData)
  {
    rtLogError("FATAL: Invalid arguments - imageData = NULL");
    return e;
  }

  if(imageDataSize < 8)
  {
    rtLogError("FATAL: Invalid arguments - imageDataSize < 8");
    return e;
  }

  unsigned char header[8];    // 8 is the maximum size that can be checked

  // open file and test for it being a png
  memcpy(header, imageData, 8);
  pngStruct.readPosition += 8;

  // test PNG header
  if(png_sig_cmp(header, 0, 8) != 0)
  {
    rtLogError("FATAL: Invalid PNG header");
    return RT_FAIL;
  }

  // initialize stuff
  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

  if(!png_ptr)
  {
    rtLogError("FATAL: png_create_read_struct() - failed !");
    return e;
  }

  info_ptr = png_create_info_struct(png_ptr);

  if(!info_ptr)
  {
    rtLogError("FATAL: png_create_info_struct() - failed !");
    return e;
  }

  if (!setjmp(png_jmpbuf(png_ptr)))
  {
    png_set_read_fn(png_ptr,(png_voidp)&pngStruct, readPngData);
    png_set_sig_bytes(png_ptr, 8);

    png_read_info(png_ptr, info_ptr);

    int width  = png_get_image_width(png_ptr, info_ptr);
    int height = png_get_image_height(png_ptr, info_ptr);

    png_byte color_type = png_get_color_type(png_ptr, info_ptr);
    //png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);

    if (color_type == PNG_COLOR_TYPE_PALETTE)
    {
      png_set_palette_to_rgb(png_ptr);
    }

    if (color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    {
      png_set_gray_to_rgb(png_ptr);
    }

    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
    {
      png_set_tRNS_to_alpha(png_ptr);
    }

    png_set_bgr(png_ptr);
    png_set_add_alpha(png_ptr, 0xff, PNG_FILLER_AFTER);

    o.init(width, height);

    //	    number_of_passes = png_set_interlace_handling(png_ptr);
    png_read_update_info(png_ptr, info_ptr);

    // read file
    if (!setjmp(png_jmpbuf(png_ptr)))
    {
      row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);

      if (row_pointers)
      {
        for (int y = 0; y < height; y++)
        {
          row_pointers[y] = (png_byte*)o.scanline(y);
        }

        png_read_image(png_ptr, row_pointers);
        free(row_pointers);
      }
      e = RT_OK;
    }
  }

  return e;
}
