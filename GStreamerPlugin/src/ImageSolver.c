
#define _CRT_SECURE_DEPRECATE_MEMORY
#include <memory.h>
#include <stdlib.h>   // For _MAX_PATH definition
#include <stdio.h>
#include <malloc.h>
#include <math.h>
#include "stdafx.h"
#include "ImageSolver.h"


img_present* createImgPresent(int _width, int _height, int _precision, int _color_type, int _timer, unsigned char *_source)
{
	//Init struct
	img_present* ret = (img_present*)malloc(sizeof(img_present));
	ret->mid_brightness = 0;
	ret->width = _width;
	ret->height = _height;
	ret->precision = _precision;
	ret->color_type = _color_type;
	ret->timer = _timer;
	//Init data storage
	if (_source != NULL){
		ret->source = (unsigned char ***)malloc(sizeof(unsigned char **)*_width);
		for (int i=0;i<_width;i++){
			_p_translateLine(ret,_source,i);
		}
		ret->mid_brightness = ret->mid_brightness/(ret->width*ret->height);
	}
	else {
		ret->source = NULL;
	}
	//Init not involved part
	ret->bit_matrix = NULL;
	ret->encryption_x = NULL;
	ret->encryption_y = NULL;
	return ret;
}

void _p_translateLine(img_present *img, unsigned char *_source, int column)
{
	if ((img != NULL) && (_source != NULL)){
		//Column malloc
		img->source[column] = (unsigned char **)malloc(sizeof(unsigned char *)*img->height);
		for (int i=0;i<img->height;i++){
			//Pixel malloc
			unsigned char* k=_source+(i*img->width+column)*3;
			img->source[column][i] = (unsigned char *)malloc(sizeof(unsigned char)*3);
			//Proceed stream data to YUV massive matrix 
			if (img->color_type == 1){
				img->source[column][i][0] = 0.299 * (*(_source+(i*img->width+column)*3)) + 0.587 * (*(_source+(i*img->width+column)*3+1)) + 0.114 * (*(_source+(i*img->width+column)*3+2));
				img->source[column][i][1] = -0.14713 * (*(_source+(i*img->width+column)*3)) - 0.28886 * (*(_source+(i*img->width+column)*3+1)) + 0.436 * (*(_source+(i*img->width+column)*3+2)) + 128;
				img->source[column][i][2] = 0.615 * (*(_source+(i*img->width+column)*3)) - 0.51499 * (*(_source+(i*img->width+column)*3+1)) - 0.10001 * (*(_source+(i*img->width+column)*3+2)) + 128;
			}
			else if (img->color_type == 2){
				img->source[column][i][0] = *(k++);
				img->source[column][i][1] = *(k++);
				img->source[column][i][2] = *(k++);
				img->mid_brightness += img->source[column][i++][0];
				img->source[column][i][0] = *(k++);
				img->source[column][i][1] = img->source[column][i-1][1];
				img->source[column][i][2] = img->source[column][i-1][2];
				img->mid_brightness += img->source[column][i++][0];
				img->source[column][i][0] = *(k++);
				img->source[column][i][1] = *(k++);
				img->source[column][i][2] = *(k++);
				img->mid_brightness += img->source[column][i++][0];
				img->source[column][i][0] = *(k++);
				img->source[column][i][1] = img->source[column][i-1][1];
				img->source[column][i][2] = img->source[column][i-1][2];
				i+=3;
			}
			else{
				img->source[column][i][0] = *(_source+(i*img->width+column)*3);
				img->source[column][i][1] = *(_source+(i*img->width+column)*3+1);
				img->source[column][i][2] = *(_source+(i*img->width+column)*3+2);
			}
			img->mid_brightness += img->source[column][i][0];
		}
	}
}

int saveToFileImgPresent(char *filename, img_present *img)
{
	if (img != NULL){
		FILE *stream;
		// Open for read 
		if ( (stream  = fopen( filename, "w+" )) == NULL ) // C4996
			return -1;

		fprintf(stream,"%d %d %d %d %d %d \n",img->width,img->height,img->precision,img->color_type,img->mid_brightness,img->timer);
		if (img->source != NULL){
			fprintf(stream,"1\n");
			for (int i=0;i<img->width;i++){
				for (int j=0;j<img->height;j++){
					fprintf(stream,"(%d %d %d) ",(int)img->source[i][j][0],(int)img->source[i][j][1],(int)img->source[i][j][2]);
				}
				fprintf(stream,"\n");
			}
		}
		else{
			fprintf(stream,"0\n");
		}
		if (img->bit_matrix != NULL){
			fprintf(stream,"1\n");
			for (int i=0;i<(img->width/img->precision);i++){
				for (int j=0;j<(img->height/img->precision);j++){
					fprintf(stream,"(%d) ",(int)img->bit_matrix[i][j]);
				}
				fprintf(stream,"\n");
			}
		}
		else{
			fprintf(stream,"0\n");
		}
		if (img->encryption_x != NULL){
			fprintf(stream,"1\n");
			for (int i=0;i<(img->width/img->precision);i++){
				fprintf(stream,"(%d) ",img->encryption_x[i][0]);
				for (int j=1;j<img->encryption_x[i][0];j++){
					fprintf(stream,"(%d) ",img->encryption_x[i][j]);
				}
				fprintf(stream,"\n");
			}
		}
		else{
			fprintf(stream,"0\n");
		}
		if (img->encryption_y != NULL){
			fprintf(stream,"1\n");
			for (int i=0;i<(img->height/img->precision);i++){
				fprintf(stream,"(%d) ",img->encryption_y[i][0]);
				for (int j=1;j<img->encryption_y[i][0];j++){
					fprintf(stream,"(%d) ",img->encryption_y[i][j]);
				}
				fprintf(stream,"\n");
			}
		}
		else{
			fprintf(stream,"0\n");
		}

		// Close stream if it is not NULL 
		if ( stream ){
			fclose( stream );
		}
	}
	return 0;
}
img_present* openFromFileImgPresent(char *filename)
{
	FILE *stream;
	int nt = 0,t1 = 0,t2 = 0,t3 = 0;
	img_present *ret = NULL;
	// Open for read 
	if ( (stream  = fopen( filename, "r" )) == NULL ) // C4996
		return NULL;
	ret = createImgPresent(0,0,0,0,0,NULL);
	fscanf(stream,"%d %d %d %d %d %d \n",&(ret->width),&(ret->height),&(ret->precision),&(ret->color_type),&(ret->mid_brightness),&(ret->timer));
	fscanf(stream,"%d\n",&nt);
	if (nt == 1){
		ret->source = (unsigned char ***)malloc(sizeof(unsigned char **)*ret->width);
		for (int i=0;i<ret->width;i++){
			ret->source[i] = (unsigned char **)malloc(sizeof(unsigned char *)*ret->height);
			for (int j=0;j<ret->height;j++){
				ret->source[i][j] = (unsigned char *)malloc(sizeof(unsigned char)*3);
				fscanf(stream,"(%d %d %d) ",&t1,&t2,&t3);
				ret->source[i][j][0] = (unsigned char)t1;
				ret->source[i][j][1] = (unsigned char)t2;
				ret->source[i][j][2] = (unsigned char)t3;
			}
			fscanf(stream,"\n");
		}
	}
	else{
		ret->source = NULL;
	}
	fscanf(stream,"%d\n",&nt);
	if (nt == 1){
		ret->bit_matrix = (unsigned char **)malloc(sizeof(unsigned char *)*(ret->width/ret->precision));
		for (int i=0;i<(ret->width/ret->precision);i++){
			ret->bit_matrix[i] = (unsigned char *)malloc(sizeof(unsigned char)*(ret->height/ret->precision));
			for (int j=0;j<(ret->height/ret->precision);j++){
				fscanf(stream,"(%d) ",&t1);
				ret->bit_matrix[i][j] = (unsigned char)t1;
			}
			fscanf(stream,"\n");
		}
	}
	else{
		ret->bit_matrix = NULL;
	}
	fscanf(stream,"%d\n",&nt);
	if (nt == 1){
		ret->encryption_x = (int **)malloc(sizeof(int *)*ret->width);
		for (int i=0;i<(ret->width/ret->precision);i++){
			fscanf(stream,"(%d)",&t1);
			ret->encryption_x[i] = (int *)malloc(sizeof(int)*t1);
			ret->encryption_x[i][0] = t1;
			for (int j=0;j<ret->encryption_x[i][0];j++){
				fscanf(stream,"(%d)",&ret->encryption_x[i][j]);
			}
			fscanf(stream,"\n");
		}
	}
	else{
		ret->encryption_x = NULL;
	}
	fscanf(stream,"%d\n",&nt);
	if (nt == 1){
		ret->encryption_y = (int **)malloc(sizeof(int *)*ret->height);
		for (int i=0;i<(ret->height/ret->precision);i++){
			fscanf(stream,"(%d)",&t1);
			ret->encryption_y[i] = (int *)malloc(sizeof(int)*t1);
			ret->encryption_y[i][0] = t1;
			for (int j=0;j<ret->encryption_y[i][0];j++){
				fscanf(stream,"(%d)",&ret->encryption_y[i][j]);
			}
			fscanf(stream,"\n");
		}
	}
	else{
		ret->encryption_y = NULL;
	}

	// Close stream if it is not NULL 
	if ( stream )
	{
		fclose( stream );
	}
	return ret;
}
int destroyImgPresent(img_present *img)
{
	if (img->source != NULL){
		for (int i=0;i<img->width;i++){
			for (int j=0;j<img->height;j++){
				free((void*)(img->source[i][j]));
			}
			free((void*)(img->source[i]));
		}
		free((void*)(img->source));
	}
	if (img->bit_matrix != NULL){
		for (int i=0;i<(img->width/img->precision);i++){
			free((void*)(img->bit_matrix[i]));
		}
		free((void*)(img->bit_matrix));
	}
	if (img->encryption_x != NULL){
		for (int i=0;i<(img->width/img->precision);i++){
			free((void*)(img->encryption_x[i]));
		}
		free((void*)(img->encryption_x));
	}
	if (img->encryption_y != NULL){
		for (int i=0;i<(img->height/img->precision);i++){
			free((void*)(img->encryption_y[i]));
		}
		free((void*)(img->encryption_y));
	}
	free((void*)img);
	return 0;
}
img_present*  clone(img_present *img)
{	
	img_present* ret = (img_present*)malloc(sizeof(img_present));
	ret->mid_brightness = img->mid_brightness;
	ret->width = img->width;
	ret->height = img->height;
	ret->precision = img->precision;
	ret->color_type = img->color_type;
	ret->timer = img->timer;
	if (img->source != NULL){
		ret->source = (unsigned char ***)malloc(sizeof(unsigned char **)*ret->width);
		for (int i=0;i<img->width;i++){
			ret->source[i] = (unsigned char **)malloc(sizeof(unsigned char *)*ret->height);
			for (int j=0;j<img->height;j++){
				ret->source[i][j] = (unsigned char *)malloc(sizeof(unsigned char)*3);
				memcpy(ret->source[i][j],img->source[i][j],3);
			}
		}
	}
	else{
		img->source = NULL;
	}
	if (img->bit_matrix != NULL){
		ret->bit_matrix = (unsigned char **)malloc(sizeof(unsigned char *)*(ret->width/ret->precision));
		for (int i=0;i<(ret->width/ret->precision);i++){
			ret->bit_matrix[i] = (unsigned char *)malloc(sizeof(unsigned char)*(ret->height/ret->precision));
			memcpy(ret->bit_matrix[i],img->bit_matrix[i],(ret->height/ret->precision));
		}
	}
	else{
		img->bit_matrix = NULL;
	}
	if (img->encryption_x != NULL){
		ret->encryption_x = (int **)malloc(sizeof(int *)*(ret->width/ret->precision));
		for (int i=0;i<(ret->width/ret->precision);i++){
			ret->encryption_x[i] = (int *)malloc(sizeof(int)*img->encryption_x[i][0]);
			memcpy(ret->encryption_x[i]+1,img->encryption_x[i]+1,img->encryption_x[i][0]);
		}
	}
	else{
		img->encryption_x = NULL;
	}
	if (img->encryption_y != NULL){
		ret->encryption_y = (int **)malloc(sizeof(int *)*(ret->height/ret->precision));
		for (int i=0;i<(ret->height/ret->precision);i++){
			ret->encryption_y[i] = (int *)malloc(sizeof(int)*img->encryption_y[i][0]);
			memcpy(ret->encryption_y[i]+1,img->encryption_y[i]+1,img->encryption_y[i][0]);
		}
	}
	else{
		img->encryption_y = NULL;
	}
	return ret;
}
img_present*  make_precendent(img_present *img,int x1,int y1,int x2,int y2,int _timer)
{
	img_present* ret = (img_present*)malloc(sizeof(img_present));
	ret->mid_brightness = img->mid_brightness;
	ret->width = x2 - x1;
	ret->height = y2 - y1;
	ret->precision = img->precision;
	ret->color_type = img->color_type;
	ret->timer = img->timer;
	if (img->source != NULL){
		ret->source = (unsigned char ***)malloc(sizeof(unsigned char **)*ret->width);
		for (int i=x1;(i<img->width) && (i<=x2);i++){
			ret->source[i] = (unsigned char **)malloc(sizeof(unsigned char *)*ret->height);
			for (int j=y1;(j<img->height) && (j<=y2);j++){
				ret->source[i][j] = (unsigned char *)malloc(sizeof(unsigned char)*3);
				memcpy(ret->source[i-x1][j-y1],img->source[i][j],3);
			}
		}
	}
	else{
		img->source = NULL;
	}
	ret->bit_matrix = NULL;
	degrade(ret);
	ret->encryption_x = NULL;
	encrypt_x(ret);
	ret->encryption_y = NULL;
	encrypt_y(ret);
	return ret;
}

bool timer_check(img_present *img)
{
	if (img->timer <= 0 )
		return true;
	else
		return false;
}
int timer_dec(img_present *img)
{
	return --img->timer;
}
int shapeFilter(img_present *img)
{
	if ((img != NULL) && (img->source != NULL)){
	}
	return 0;
}
int degrade(img_present *img)
{
	if ((img != NULL) && (img->source != NULL)){
		img->bit_matrix = (unsigned char**)malloc(sizeof(unsigned char*)*(img->width/img->precision));
		for (int i=0;i<(img->width);i+=img->precision){
			img->bit_matrix[i/img->precision] = (unsigned char*)malloc(sizeof(unsigned char)*(img->height/img->precision));
			for (int j=0;j<(img->height);j+=img->precision){
				if (img->source[i][j][0] >= img->mid_brightness){
					img->bit_matrix[i/img->precision][j/img->precision] = 0;
				}
				else{
					img->bit_matrix[i/img->precision][j/img->precision] = 1;
				}
			}
		}
	}
	return 0;
}
int encrypt_x(img_present *img)
{
	img->encryption_x = (int**)malloc(sizeof(int*)*(img->height/img->precision));
	for (int i=0;i<(img->height/img->precision);i++){
		int n = 1;
		char cur = img->bit_matrix[0][i];
		for (int j=1;j<(img->width/img->precision);j++){
			if (cur != img->bit_matrix[j][i]){
				n++;
				cur = img->bit_matrix[j][i];
			}
		}
		img->encryption_x[i] = (int*)malloc(sizeof(int)*(n));
		img->encryption_x[i][0] = n;
		n = 0;
		int nn=1;
		cur = img->bit_matrix[0][i];
		for (int j=1;j<(img->width/img->precision);j++){
			n++;
			if (cur != img->bit_matrix[j][i]){
				img->encryption_x[i][nn] = n;
				n = 0;
				cur = img->bit_matrix[j][i];
				nn++;
			}
		}
	}
	return 0;
}
int encrypt_y(img_present *img)
{
	img->encryption_y = (int**)malloc(sizeof(int*)*(img->width/img->precision));
	for (int i=0;i<(img->width/img->precision);i++){
		int n = 1;
		char cur = img->bit_matrix[i][0];
		for (int j=1;j<(img->height/img->precision);j++){
			if (cur != img->bit_matrix[i][j]){
				n++;
				cur = img->bit_matrix[i][j];
			}
		}
		img->encryption_y[i] = (int*)malloc(sizeof(int)*(n));
		img->encryption_y[i][0] = n;
		n = 0;
		int nn=1;
		cur = img->bit_matrix[i][0];
		for (int j=1;j<(img->height/img->precision);j++){
			n++;
			if (cur != img->bit_matrix[i][j]){
				img->encryption_y[i][nn] = n;
				n = 0;
				cur = img->bit_matrix[i][j];
				nn++;
			}
		}
	}
	return 0;
}

img_present*  discrete(img_present *encryption, float divider)
{
	img_present* ret = clone(encryption);
	if (ret->encryption_x != NULL){
		for (int i=0;i<(ret->width/ret->precision);i++){
			for (int j=1;j<ret->encryption_x[i][0];j++){
				ret->encryption_x[i][j] /= divider;
			}
		}
	}
	if (ret->encryption_y != NULL){
		for (int i=0;i<(ret->height/ret->precision);i++){
			for (int j=1;j<ret->encryption_y[i][0];j++){
				ret->encryption_y[i][j] /= divider;
			}
		}
	}
	return ret;
}
bool compare(img_present *encryption1, img_present *encryption2)
{
	if ((encryption1->encryption_x == NULL) || (encryption1->encryption_y == NULL))
		return false;
	if ((encryption1->width != encryption2->width) || (encryption1->height != encryption2->height))
		return false;
	for (int i=0;i<(encryption1->width/encryption1->precision);i++)
	{
		if (encryption1->encryption_x[i][0] != encryption2->encryption_x[i][0])
		{
			return false;
		}
	}
	for (int i=0;i<(encryption1->height/encryption1->precision);i++)
	{
		if (encryption1->encryption_y[i][0] != encryption2->encryption_y[i][0])
		{
			return false;
		}
	}
	return true;
}
bool insider(img_present *encryption1, img_present *encryption2)
{
	if ((encryption1->encryption_x == NULL) || (encryption1->encryption_y == NULL) || (encryption1->bit_matrix == NULL) ||
		(encryption2->encryption_x == NULL) || (encryption2->encryption_y == NULL) || (encryption2->bit_matrix == NULL))
		return false;
	for (int i=0;i < (encryption1->width/encryption1->precision);i++)
	{
		bool setx = false;
		for (int j=0;(j < (encryption2->width/encryption2->precision)) && ((i+j) < (encryption1->height/encryption1->precision));j++)
		{
			if (setx = (encryption1->encryption_x[i+j][0] < encryption2->encryption_x[j][0]))
				break;
		}
		if (!setx)
		{
			for (int k=0;k < (encryption1->height/encryption1->precision);k++)
			{
				bool sety = false;
				for (int l=0;(l < (encryption2->height/encryption2->precision)) && ((k+l) < (encryption1->height/encryption1->precision));l++)
				{
					if (sety = (encryption1->encryption_y[k+l][0] < encryption2->encryption_y[l][0]))
						break;
				}
				if (!sety)
					if (!check_in_pos(encryption1,i,k,encryption2,encryption1->precision,false))
						break;
				return true;
			}
		}
	}
	return false;
}
bool check_in_pos(img_present *encryption1, int x, int y, img_present *encryption2, int precision, bool fast)
{
	if ((encryption1->encryption_x == NULL) || (encryption1->encryption_y == NULL) || (encryption1->bit_matrix == NULL) ||
		(encryption2->encryption_x == NULL) || (encryption2->encryption_y == NULL) || (encryption2->bit_matrix == NULL))
		return false;
	if (fast)
	{
		for (int i=0;i < (encryption2->width/encryption2->precision);i++)
		{
			int eq = 0;
			for (int j=2;j < (encryption1->encryption_x[x+i][0]-1);j++)
			{
				if (/*(eq == 0) && */(abs(encryption1->encryption_x[x+i][y+j] - encryption2->encryption_x[i][j]) < precision))
					eq++;
				else 
					if (eq != encryption2->encryption_x[i][0])
						eq = 0;
					else
						break;
			}
			if (eq != encryption2->encryption_x[i][0])
				return false;
		}
		return true;
	}
	else
	{
		for (int i=0;i < (encryption2->width/encryption2->precision);i++)
		{
			for (int j=0;j < (encryption2->height/encryption2->precision);j++)
			{
				if (encryption1->bit_matrix[x+i][y+j] != encryption2->bit_matrix[i][j])
					return false;
			}
		}
		return true;
	}
}
void print_img(img_present *img)
{
	printf("IMG\n");
	for (int i=0;i<(img->width/img->precision);i++)
	{
		for (int j=0;j<(img->height/img->precision);j++)
		{
			printf("%d ",(int)img->bit_matrix[i][j]);
		}
		printf("\n");
	}
	return;
}
void print_encrypt(img_present *img)
{
	printf("Encrypt X\n");
	for (int i=0;i<(img->width/img->precision);i++)
	{
		for (int j=0;j<(img->encryption_x[i][0]);j++)
		{
			printf("%d ",img->encryption_x[i][j]);
		}
		printf("\n");
	}
	printf("Encrypt Y\n");
	for (int i=0;i<(img->height/img->precision);i++)
	{
		for (int j=0;j<(img->encryption_y[i][0]);j++)
		{
			printf("%d ",img->encryption_y[i][j]);
		}
		printf("\n");
	}
	return;
}