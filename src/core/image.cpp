#include "stdafx.h"
#include "image.h"

#ifdef __unix
#include <png.h>
#include <stdlib.h>
#include <stdarg.h>
#endif

#ifdef _WIN32

#include <windows.h>
#include <process.h>
#include <gdiplus.h>

	

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	using namespace Gdiplus;

	UINT  num = 0;          // number of Image encoders
	UINT  size = 0;         // size of the Image encoder array in bytes

	ImageCodecInfo* pImageCodecInfo = NULL;

	GetImageEncodersSize(&num, &size);
	if(size == 0)
		return -1;  // Failure

	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if(pImageCodecInfo == NULL)
		return -1;  // Failure

	GetImageEncoders(num, size, pImageCodecInfo);

	for(UINT j = 0; j < num; ++j)
	{
		if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;  // Success
		}    
	}

	free(pImageCodecInfo);
	return -1;  // Failure
}

#undef min
#undef max

#endif


#ifdef __unix
void abort_(const char * s, ...)
{
  va_list args;
  va_start(args, s);
  vfprintf(stderr, s, args);
  fprintf(stderr, "\n");
  va_end(args);
  abort();
}
#endif

void Image::writePNG(std::string _fileName)
{

#ifdef _WIN32
	using namespace Gdiplus;

	// Initialize GDI+.
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	CLSID   encoderClsid;
	Status  stat;

	Bitmap *Image = new Bitmap(m_width, m_height, PixelFormat24bppRGB);
	BitmapData data;
	Rect r(0, 0, m_width, m_height);
	Image->LockBits(&r, ImageLockModeWrite, PixelFormat24bppRGB, &data);

	std::vector<float4>::const_iterator it = m_bits.begin();
	for(uint y = 0; y < m_height; y++)
	{
		byte* ptr = (byte*)data.Scan0 + data.Stride * y;
		for(uint x = 0; x < m_width; x++)
		{
			float4 v = *it++;
			v = float4::max(float4::min(v, float4::rep(1)), float4::rep(0));
			*ptr++ = (byte)(v.z * 255);
			*ptr++ = (byte)(v.y * 255);
			*ptr++ = (byte)(v.x * 255);
		}
	}

	Image->UnlockBits(&data);

	// Get the CLSID of the PNG encoder.
	GetEncoderClsid(L"image/png", &encoderClsid);

	std::wstring name(_fileName.begin(), _fileName.end());
	stat = Image->Save(name.c_str(), &encoderClsid, NULL);

	delete Image;
	GdiplusShutdown(gdiplusToken);
#endif

#ifdef __unix

	std::vector<png_byte> byteData (m_bits.size() * 3);
	std::vector<png_byte>::iterator ptr = byteData.begin();
	for(std::vector<float4>::const_iterator it = m_bits.begin(); it != m_bits.end(); it++)
	{
		float4 v = *it;
		v = float4::max(float4::min(v, float4::rep(1)), float4::rep(0));
		*ptr++ = (byte)(v.x * 255);
		*ptr++ = (byte)(v.y * 255);
		*ptr++ = (byte)(v.z * 255);
	}

	std::vector<png_byte*> rowData(m_height);
	for(int i = 0; i < m_height; i++)
		rowData[i] = i * m_width * 3 + &byteData.front();

	/* create file */
	FILE *fp = fopen(_fileName.c_str(), "wb");
	if (!fp)
	  abort_("[write_png_file] File %s could not be opened for writing", _fileName.c_str());


	/* initialize stuff */
	png_structp png_ptr;
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (!png_ptr)
		abort_("[write_png_file] png_create_write_struct failed");

	png_infop info_ptr;
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
		abort_("[write_png_file] png_create_info_struct failed");

	if (setjmp(png_jmpbuf(png_ptr)))
		abort_("[write_png_file] Error during init_io");


	png_init_io(png_ptr, fp);

	/* write header */
	if (setjmp(png_jmpbuf(png_ptr)))
		abort_("[write_png_file] Error during writing header");

	png_set_IHDR(png_ptr, info_ptr, m_width, m_height,
		8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	png_write_info(png_ptr, info_ptr);

	/* write bytes */
	if (setjmp(png_jmpbuf(png_ptr)))
		abort_("[write_png_file] Error during writing bytes");

	png_write_image(png_ptr, (png_byte**)&rowData.front());


	/* end write */
	if (setjmp(png_jmpbuf(png_ptr)))
		abort_("[write_png_file] Error during end of write");

	png_write_end(png_ptr, NULL);

	fclose(fp);
#endif
}


void Image::readPNG(std::string _fileName)
{
#ifdef _WIN32
	using namespace Gdiplus;

	// Initialize GDI+.
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	std::wstring name(_fileName.begin(), _fileName.end());
	Bitmap *bmp = new Bitmap(name.c_str(), FALSE);
	m_width = bmp->GetWidth();
	m_height = bmp->GetHeight();
	m_bits.resize(m_width * m_height);

	BitmapData data;
	Rect r(0, 0, m_width, m_height);
	bmp->LockBits(&r, ImageLockModeWrite, PixelFormat24bppRGB, &data);

	std::vector<float4>::iterator it = m_bits.begin();
	for(uint y = 0; y < m_height; y++)
	{
		byte* ptr = (byte*)data.Scan0 + data.Stride * y;
		for(uint x = 0; x < m_width; x++)
		{
			float4 &v = *it++;
			v.z = (float)(*ptr++) / 255.f;
			v.y = (float)(*ptr++) / 255.f;
			v.x = (float)(*ptr++) / 255.f;
			v.w = 0.f;
		}
	}

	bmp->UnlockBits(&data);
	delete bmp;

	GdiplusShutdown(gdiplusToken);
#endif

#ifdef __unix
	png_byte header[8];	// 8 is the maximum size that can be checked

	/* open file and test for it being a png */
	FILE *fp = fopen(_fileName.c_str(), "rb");
	if (!fp)
		abort_("[read_png_file] File %s could not be opened for reading", _fileName.c_str());
	fread(header, 1, 8, fp);
	if (png_sig_cmp(header, 0, 8))
		abort_("[read_png_file] File %s is not recognized as a PNG file", _fileName.c_str());


	/* initialize stuff */
	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (!png_ptr)
		abort_("[read_png_file] png_create_read_struct failed");

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
		abort_("[read_png_file] png_create_info_struct failed");

	if (setjmp(png_jmpbuf(png_ptr)))
		abort_("[read_png_file] Error during init_io");

	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, 8);

	png_read_info(png_ptr, info_ptr);

	m_width = info_ptr->width;
	m_height = info_ptr->height;
	/*color_type = info_ptr->color_type;
	bit_depth = info_ptr->bit_depth;*/

	int number_of_passes = png_set_interlace_handling(png_ptr);
	png_read_update_info(png_ptr, info_ptr);

	std::vector<png_byte> byteData (info_ptr->rowbytes * m_height);
	std::vector<png_byte*> rowData(m_height);
	for(int i = 0; i < m_height; i++)
		rowData[i] = i * info_ptr->rowbytes + &byteData.front();

	/* read file */
	if (setjmp(png_jmpbuf(png_ptr)))
		abort_("[read_png_file] Error during read_image");

	png_read_image(png_ptr, &rowData.front());

	fclose(fp);

	m_bits.resize(m_width * m_height);
	std::vector<float4>::iterator it = m_bits.begin();
	for(size_t y = 0; y < m_height; y++)
	{
		png_byte *b = rowData[y];
		for(size_t x = 0; x < m_width; x++)
		{
			float4 &v = *it++;
			v.z = (float)(*b++) / 255.f;
			v.y = (float)(*b++) / 255.f;
			v.x = (float)(*b++) / 255.f;
			if(info_ptr->channels == 4)
				b++;
			v.w = 0.f;
		}
	}
#endif

}
