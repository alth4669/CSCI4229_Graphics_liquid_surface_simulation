/*
 *  Load texture from BMP file
 */
#include "texLoad.h"

/*
 *  Reverse n bytes
 */
static void Reverse(void* x,const int n)
{
   int k;
   char* ch = (char*)x;
   for (k=0;k<n/2;k++)
   {
      char tmp = ch[k];
      ch[k] = ch[n-1-k];
      ch[n-1-k] = tmp;
   }
}

/*
 *  Load texture from BMP file
 */
unsigned int LoadCubeTexBMP(const char* file[])
{
   unsigned int   texture;    // Texture name
   FILE*          f;          // File pointer
   unsigned short magic;      // Image magic
   unsigned int   dx,dy,size; // Image dimensions
   unsigned short nbp,bpp;    // Planes and bits per pixel
   unsigned char* image;      // Image data
   unsigned int   off;        // Image offset
   unsigned int   k;          // Counter
   int            max;        // Maximum texture dimensions

   //  Generate Cube texture
   glGenTextures(1,&texture);
   glBindTexture(GL_TEXTURE_CUBE_MAP,texture);

   //  Generate images for the various sides of the cube map
   int i;
   for (i=0; i<6; i++) {
      //  Open file
      f = fopen(file[i],"rb");
      if (!f) Fatal("Cannot open file %s\n",file[i]);
      //  Check image magic
      if (fread(&magic,2,1,f)!=1) Fatal("Cannot read magic from %s\n",file[i]);
      if (magic!=0x4D42 && magic!=0x424D) Fatal("Image magic not BMP in %s\n",file[i]);
      //  Read header
      if (fseek(f,8,SEEK_CUR) || fread(&off,4,1,f)!=1 ||
         fseek(f,4,SEEK_CUR) || fread(&dx,4,1,f)!=1 || fread(&dy,4,1,f)!=1 ||
         fread(&nbp,2,1,f)!=1 || fread(&bpp,2,1,f)!=1 || fread(&k,4,1,f)!=1)
         Fatal("Cannot read header from %s\n",file[i]);
      //  Reverse bytes on big endian hardware (detected by backwards magic)
      if (magic==0x424D)
      {
         Reverse(&off,4);
         Reverse(&dx,4);
         Reverse(&dy,4);
         Reverse(&nbp,2);
         Reverse(&bpp,2);
         Reverse(&k,4);
      }
      //  Check image parameters
      glGetIntegerv(GL_MAX_TEXTURE_SIZE,&max);
      if (dx<1 || dx>max) Fatal("%s image width %d out of range 1-%d\n",file[i],dx,max);
      if (dy<1 || dy>max) Fatal("%s image height %d out of range 1-%d\n",file[i],dy,max);
      if (nbp!=1)  Fatal("%s bit planes is not 1: %d\n",file[i],nbp);
      if (bpp!=24) Fatal("%s bits per pixel is not 24: %d\n",file[i],bpp);
      if (k!=0)    Fatal("%s compressed files not supported\n",file[i]);
   #ifndef GL_VERSION_2_0
      //  OpenGL 2.0 lifts the restriction that texture size must be a power of two
      for (k=1;k<dx;k*=2);
      if (k!=dx) Fatal("%s image width not a power of two: %d\n",file[i],dx);
      for (k=1;k<dy;k*=2);
      if (k!=dy) Fatal("%s image height not a power of two: %d\n",file[i],dy);
   #endif

      //  Allocate image memory
      size = 3*dx*dy;
      image = (unsigned char*) malloc(size);
      if (!image) Fatal("Cannot allocate %d bytes of memory for image %s\n",size,file[i]);
      //  Seek to and read image
      if (fseek(f,off,SEEK_SET) || fread(image,size,1,f)!=1) Fatal("Error reading data from image %s\n",file[i]);
      fclose(f);
      //  Reverse colors (BGR -> RGB)
      for (k=0;k<size;k+=3)
      {
         unsigned char temp = image[k];
         image[k]   = image[k+2];
         image[k+2] = temp;
      }

      //  Sanity check
      ErrCheck("LoadTexBMP");

      //  Copy image
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i,0,3,dx,dy,0,GL_RGB,GL_UNSIGNED_BYTE,image);
      if (glGetError()) Fatal("Error in glTexImage2D %s %dx%d\n",file,dx,dy);

      //  Free image memory
      free(image);
   }
   //  Scale linearly when image size doesn't match
   glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
;
   //  Return texture name
   return texture;
}
