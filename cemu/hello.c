#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <string.h>

struct BMPHeader
{
    char bfType[2];       /* "BM" */
    int bfSize;           /* Size of file in bytes */
    int bfReserved;       /* set to 0 */
    int bfOffBits;        /* Byte offset to actual bitmap data (= 54) */
    int biSize;           /* Size of BITMAPINFOHEADER, in bytes (= 40) */
    int biWidth;          /* Width of image, in pixels */
    int biHeight;         /* Height of images, in pixels */
    short biPlanes;       /* Number of planes in target device (set to 1) */
    short biBitCount;     /* Bits per pixel (24 in this case) */
    int biCompression;    /* Type of compression (0 if no compression) */
    int biSizeImage;      /* Image size, in bytes (0 if no compression) */
    int biXPelsPerMeter;  /* Resolution in pixels/meter of display device */
    int biYPelsPerMeter;  /* Resolution in pixels/meter of display device */
    int biClrUsed;        /* Number of colors in the color table (if 0, use 
                             maximum allowed by biBitCount) */
    int biClrImportant;   /* Number of important colors.  If 0, all colors 
                             are important */
};

int
read_bmp(const char *filename, int *width, int *height, unsigned char *rgb)
{
    fprintf(stderr, "Sorry, reading of .bmp files isn't supported yet.\n");
    return(0);
}

int 
write_bmp(const char *filename, int width, int height, char *rgb)
{
    int i, j, ipos;
    int bytesPerLine;
    unsigned char *line;

    FILE *file;
    struct BMPHeader bmph;

    /* The length of each line must be a multiple of 4 bytes */

    bytesPerLine = (3 * (width + 1) / 4) * 4;

    strcpy(bmph.bfType, "BM");
    bmph.bfOffBits = 54;
    bmph.bfSize = bmph.bfOffBits + bytesPerLine * height;
    bmph.bfReserved = 0;
    bmph.biSize = 40;
    bmph.biWidth = width;
    bmph.biHeight = height;
    bmph.biPlanes = 1;
    bmph.biBitCount = 24;
    bmph.biCompression = 0;
    bmph.biSizeImage = bytesPerLine * height;
    bmph.biXPelsPerMeter = 0;
    bmph.biYPelsPerMeter = 0;
    bmph.biClrUsed = 0;       
    bmph.biClrImportant = 0; 

    file = fopen (filename, "wb");
    if (file == NULL) return(0);
  
    fwrite(&bmph.bfType, 2, 1, file);
    fwrite(&bmph.bfSize, 4, 1, file);
    fwrite(&bmph.bfReserved, 4, 1, file);
    fwrite(&bmph.bfOffBits, 4, 1, file);
    fwrite(&bmph.biSize, 4, 1, file);
    fwrite(&bmph.biWidth, 4, 1, file);
    fwrite(&bmph.biHeight, 4, 1, file);
    fwrite(&bmph.biPlanes, 2, 1, file);
    fwrite(&bmph.biBitCount, 2, 1, file);
    fwrite(&bmph.biCompression, 4, 1, file);
    fwrite(&bmph.biSizeImage, 4, 1, file);
    fwrite(&bmph.biXPelsPerMeter, 4, 1, file);
    fwrite(&bmph.biYPelsPerMeter, 4, 1, file);
    fwrite(&bmph.biClrUsed, 4, 1, file);
    fwrite(&bmph.biClrImportant, 4, 1, file);
  
    line = (unsigned char*)malloc(bytesPerLine);
    if (line == NULL)
    {
        fprintf(stderr, "Can't allocate memory for BMP file.\n");
        return(0);
    }

    for (i = height - 1; i >= 0; i--)
    {
        for (j = 0; j < width; j++)
        {
            ipos = 3 * (width * i + j);
            line[3*j] = rgb[ipos + 2];
            line[3*j+1] = rgb[ipos + 1];
            line[3*j+2] = rgb[ipos];
        }
        fwrite(line, bytesPerLine, 1, file);
    }

    free(line);
    fclose(file);

    return(1);
}











#define epsilon 0.000000001

typedef struct {
	double  x;
	double 	y;
	double 	z;
} Vector;
typedef Vector Point;
typedef Vector Color;

typedef struct {
	Point origin;
	Vector direction;
} Ray;

typedef struct {
	Vector normal;
	double distance;
	Color color;
} Plane;

typedef struct {
	Point point;
	double lambda;
	Plane* plane;
} Intersection;

Ray getRay(int x, int y) {
	Ray r;
	r.origin.x = 0;
	r.origin.y = 0;
	r.origin.z = 0;
	r.direction.x = -320 + x + 0.5;
	r.direction.y = 240 - y + 0.5;
	r.direction.z = -120;
	return r;
}

Plane* createBox() {
	Plane* box = (Plane*)malloc(5*sizeof(Plane));
	int size = 5;
	Plane p0 = {-1, 0, 0, size, 255, 0, 100};//links
	box[0] = p0;
	Plane p1 = {1, 0, 0, size, 0, 255, 0};//rechts
	box[1] = p1;
	Plane p2 = {0, 0, -1, size, 255,255,255};//hinten
	box[2] = p2;
	Plane p3 = {0, 1, 0, size, 255, 255, 255};//oben
	box[3] = p3;
	Plane p4 = {0, -1, 0, size, 255, 255, 255};//unten
	box[4] = p4;
	return box;
}	

double vectorLength(Vector v) {
	return sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
}

Vector pointDifference(Point p1, Point p2) {
	Vector v = {p1.x - p2.x, p1.y - p2.y, p1.z - p2.z};
	return v; 
}

double scalarProduct(Vector* v1, Vector* v2) {
	return v1->x*v2->x + v1->y*v2->y + v1->z*v2->z;
}

Intersection rayHitsPlane(Ray* r, Plane* p)  {
	double gamma = scalarProduct(&(r->direction), &(p->normal));
	double side = p->distance - scalarProduct(&(p->normal), &(r->origin));
	Intersection intersect  = {0, 0, 0, 0, p};
	if( gamma * side > epsilon) {
		double lambda = side / gamma;
		intersect.point.x = r->origin.x + r->direction.x * lambda;
		intersect.point.y = r->origin.y + r->direction.y * lambda;
		intersect.point.z = r->origin.z + r->direction.z * lambda;
		intersect.lambda = lambda;
	} else {
	}

	return intersect;
}

Intersection getFirstIntersection(Ray* ray, Plane* box) {
	Intersection nearestIntersect; double minDouble = DBL_MAX;
	int planeCounter; 
	for(planeCounter = 0; planeCounter < 5; planeCounter++) {
		Intersection intersect = rayHitsPlane(ray, &box[planeCounter]);
		if(intersect.lambda > epsilon && intersect.lambda < minDouble) {
			nearestIntersect = intersect;
			minDouble = intersect.lambda;
		}
	}
	return nearestIntersect;
}

Color traceRay(Ray* ray, Plane* box) {
	Color color = {0, 0, 0};
	Point light = {0.5, 2, -3.5};
	Intersection intersect = getFirstIntersection(ray, box);
	if(intersect.lambda > epsilon) {
		Vector directionToLight = pointDifference(light, intersect.point); 
		Ray rayToLight = {intersect.point, directionToLight};
		Intersection tempIntersect = getFirstIntersection(&rayToLight, box);
		if(tempIntersect.lambda >= 1 || tempIntersect.lambda == 0) {
			double length = vectorLength(directionToLight);
			double scale = 5 / (length * length);
			color = intersect.plane->color;
			color.x *= scale;
			color.y *= scale;
			color.z *= scale;
		}
	}
	return color;
}

void  normalizeColor(Color* color) {
	double factor = 255/fmax(color->x, fmax(color->y, color->z));
	if(factor < 1) {
		color->x = color->x*factor;
		color->y = color->y*factor;
		color->z = color->z*factor;
	}
}

extern int write_bmp(const char* filename, int width, int height, char* rgb);

int main(int argc, char* argv[]) {
    uint64_t j = 0;
    for (int i = 0; i < 5000000; i++)
    {
        j++;
    }
    printf("%d\n", j);
    return 0;
	Plane* box = createBox();
	char* rgb = (char*)malloc(3*640*480*sizeof(char));	
	int x,y;
	for(x=0; x<640; x++) {
		for(y=0; y<480; y++) {
			Ray r = getRay(x, y);
			Color c = traceRay(&r, box);
			normalizeColor(&c);	
			int ipos = 3*(640*y+x);
			double red = c.z;
			double green  = c.y;
			double blue = c.x;
			rgb[ipos + 2] = (unsigned char)red; 
			rgb[ipos + 1] = (unsigned char)green;
			rgb[ipos] = (unsigned char)blue;
		}
	}
	write_bmp("test.bmp", 640, 480, rgb);
	free(box);free(rgb);
	return 0;
}