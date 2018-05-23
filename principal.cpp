#include <stdio.h>
#include <setjmp.h>
#include <stdint.h>  //uint8_t
#include <algorithm>    //min y max
#include <cmath>     //abs
#include "LibJpeg/jpeglib.h"

struct ImageInfo
			{
				uint32_t nWidth;
				uint32_t nHeight;
				uint8_t nNumComponent;
				uint8_t* pData;
				ImageInfo(int ancho, int alto, int ncomponentes) {
				    nWidth = ancho;
                    nHeight = alto;
                    nNumComponent = ncomponentes;
                    pData = new uint8_t[nWidth*nHeight*nNumComponent];
				}
				~ImageInfo() {
				    delete[] pData;
				    }
			};

struct ErrorManager
			{
				jpeg_error_mgr defaultErrorManager;
				jmp_buf jumpBuffer;
				~ErrorManager() {}//delete defaultErrorManager;
			};

ImageInfo* m_pImageInfo;

uint8_t* pImagenSalida;
uint8_t* pImagenPaso1;

void escribirImagenArchivo(ErrorManager* errorManager1, uint8_t * pImagenSalida1, FILE* parchivoSalida1){
    jpeg_compress_struct cinfo;
    cinfo.err = jpeg_std_error(&(errorManager1->defaultErrorManager));
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, parchivoSalida1);
	cinfo.image_width      = 640;
    cinfo.image_height     = 200;
    cinfo.input_components = 1;
    cinfo.in_color_space   = JCS_GRAYSCALE;
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality (&cinfo, 100, true);
    jpeg_start_compress(&cinfo, true);
    JSAMPROW row_pointer[1];          /* pointer to a single row */

    while (cinfo.next_scanline < cinfo.image_height) {
        row_pointer[0] = (JSAMPROW) &pImagenSalida1[cinfo.next_scanline*cinfo.image_width];
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    fclose(parchivoSalida1);
}

void convolucion(uint8_t * datosImagen, uint8_t * imagenConvolucionada, int ancho, int alto, float* filtro){
    //int limiteArrayImagen = 640*200;
    int x1=0;
    for (int y = 0; y < alto; y++) {

        for (int x = 0; x < ancho; x++) {
            double pixelNuevo = 0.0;

            for (int j = 0; j < 3; j++) {
                if (y + j < 1
                    || y + j - 1 >= alto)
                    continue;

                for (int i = 0; i < 3; i++) {
                    if (x + i < 1
                        || x + i - 1 >= ancho)
                        continue;

                    float k = filtro[i + j * 3];
                    int indice = ((y + j - 1)*ancho) + x + i - 1;
                    double pixel = (double)datosImagen[indice];
                    pixelNuevo += k * pixel;
                }
            }
            if(pixelNuevo<0)pixelNuevo=0;
            if(pixelNuevo>255)pixelNuevo=255;
            imagenConvolucionada[x1] = (uint8_t)pixelNuevo;
            x1++;
        }
    }
}

int main(int argc, char *argv[])
{
    float filtroAfilador[9] = { 0.0f, -1.0f, 0.0f,
                                -1.0f,  5.0f, -1.0f,
                                0.0f, -1.0f, 0.0f};
    float filtroBordeador[9] = { -1.0f,-1.0f,-1.0f,
                                -1.0f,8.0f,-1.0f,
                                -1.0f,-1.0f,-1.0f};

    pImagenPaso1 = new uint8_t[640*200*1];
    pImagenSalida = new uint8_t[640*200*1];

    jpeg_decompress_struct dcinfo;
	ErrorManager errorManager;

    FILE* pArchivoLectura = fopen("redim.jpg", "rb");
    if(!pArchivoLectura)
		return -1;

    dcinfo.err = jpeg_std_error(&errorManager.defaultErrorManager);

    //LEER ARCHIVO ORIGEN
    jpeg_create_decompress(&dcinfo);
	jpeg_stdio_src(&dcinfo, pArchivoLectura);
	jpeg_read_header(&dcinfo, TRUE);
	jpeg_start_decompress(&dcinfo);

	m_pImageInfo = new ImageInfo(dcinfo.image_width,dcinfo.image_height,dcinfo.num_components);
    uint8_t* p;
	while(dcinfo.output_scanline < dcinfo.image_height)
	{
		p = m_pImageInfo->pData + dcinfo.output_scanline*dcinfo.image_width*dcinfo.num_components;
		jpeg_read_scanlines(&dcinfo, &p, 1);
	}

	jpeg_finish_decompress(&dcinfo);
	jpeg_destroy_decompress(&dcinfo);
	fclose(pArchivoLectura);
    free(p);

	//CONVOLUCIONAR

    convolucion(m_pImageInfo->pData, pImagenPaso1, 640, 200, filtroAfilador);
    convolucion(pImagenPaso1, pImagenSalida, 640, 200, filtroBordeador);

	//ESCRIBIR ARCHIVOs DESTINO
	FILE* parchivoPaso1 = fopen("paso1.jpg", "wb");
	FILE* parchivoPaso2 = fopen("paso2.jpg", "wb");
	escribirImagenArchivo(&errorManager, pImagenPaso1, parchivoPaso1);
    escribirImagenArchivo(&errorManager, pImagenSalida, parchivoPaso2);
    delete[] pImagenPaso1;
    delete[] pImagenSalida;
    delete m_pImageInfo;
    free(parchivoPaso1);
    free(parchivoPaso2);
    free(pArchivoLectura);
    return 0;
}
