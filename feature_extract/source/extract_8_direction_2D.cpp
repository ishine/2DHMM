// EngOCR.cpp : �������̨Ӧ�ó������ڵ㡣
//
#if WIN32 
#include "opencv2/opencv.hpp" 
//#include "cv.h"
//#include "highgui.h"
#else
#include <opencv2/opencv.hpp>
#endif

//#include <vld.h>

#include <stdio.h>
#include <string>
#include <iostream>

using namespace std;
using namespace cv;

//#define SHOW_DETAIL
//#define VS2017
//#define USE_UNION
#define NORM_COLS 32
#define NORM_ROWS 64
//#define NORM_D 1024
#define  ARAN 80 
#define GAUSS_WIN_SIZE 16
#define FEAT_DIM 128

float * gauss_win = NULL;

#define swap16(x)  ( (((x)<<8) & 0xff00) | (((x)>>8) & 0x0ff) )
#define swap32(x)  ( (((x)<<24) & 0xff000000) | (((x)<<8) & 0x0ff0000) |  (((x)>>8) & 0x0ff00) | (((x)>>24) & 0x0ff) )

#ifdef USE_UNION
union var
{
	char c[4];
	int tmp_int;
	float tmp;
};
#endif


void HanningFilter(Mat &srcImg, Mat &dstImg)
{
	int rows = srcImg.rows;
	int cols = srcImg.cols;

	Mat ResizedImg(rows, cols, CV_8UC1);

	for (int j = 0; j < cols; j++) {
		float w_j = 0.5 * (1 - cos(2 * CV_PI * (j / float(cols))));
		for (int i = 0; i < rows; i++) {
			float w_i = 0.5 * (1 - cos(2 * CV_PI * (i / float(cols))));
			ResizedImg.at<uchar>(i, j) = 255 - int((255 - srcImg.at<uchar>(i, j)) * w_j *w_i);
		}
	}
	ResizedImg.copyTo(dstImg);
}


void GetImgInfo(string srcImgPath, string &dirPath, string &imgName, string &imgType)
{
	int nPos1 = srcImgPath.find_last_of("\\");
	int nPos2 = srcImgPath.find_last_of("/");
	int nPos = nPos1;

	if (nPos1 < nPos2)
	{
		nPos = nPos2;
	}

	dirPath = srcImgPath.substr(0, nPos);
	string name = srcImgPath.substr(nPos + 1, srcImgPath.length() - 1);
	nPos = name.find_last_of(".");
	imgName = name.substr(0, nPos);
	imgType = name.substr(nPos + 1, name.length() - 1);
}


float * calc_gauss_window(int N)  //for gaussion blur
{
	assert(N % 2 == 0);
	float sigma = 3.6;
	float * gauss_mask = new float[N*N]; //ע���ͷ�
	for (int i = -N / 2; i < N / 2; i++)
	{
		for (int j = -N / 2; j < N / 2; j++)
		{
			gauss_mask[(i + N / 2)*N + (j + N / 2)] = exp(-(i * i + j * j) / (2 * sigma * sigma)) / (2 * 3.1415926 * sigma * sigma);
		}
	}
	return gauss_mask;
}


//��ÿһ��block��ȡ�ݶ�����
float * grad_feature_extract(Mat img)//���� ����
{
	int size_wds = GAUSS_WIN_SIZE;
	//Mat aran_img_mat(NORM_ROWS, NORM_COLS, CV_8UC1);
	//resize(img, aran_img_mat, Size(NORM_COLS, NORM_ROWS));
	//unsigned char* aran_img = new unsigned char[64 * 32];
	int width = img.cols;
	int height = img.rows;
	unsigned char* array_img = new unsigned char[width * height];
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			array_img[i * width + j] = img.ptr(i)[j];
		}
	}
	//_CrtDumpMemoryLeaks();
	//aran_img_mat.release();
	float* direct_img_1 = new float[width * height];
	float* direct_img_2 = new float[width * height];
	float* direct_img_3 = new float[width * height];
	float* direct_img_4 = new float[width * height];
	float* direct_img_5 = new float[width * height];
	float* direct_img_6 = new float[width * height];
	float* direct_img_7 = new float[width * height];
	float* direct_img_8 = new float[width * height];
	for (int i = 0; i < width * height; i++)
	{
		direct_img_1[i] = direct_img_2[i] = direct_img_3[i] = direct_img_4[i] = direct_img_5[i] = direct_img_6[i] = direct_img_7[i] = direct_img_8[i] = 0;
	}
	float dfx = 0;
	float dfy = 0;
	float abs_dfx = 0;
	float abs_dfy = 0;
	float a1;
	float a2;
	for (int i = 1; i < height - 1; i++)
	{
		for (int j = 1; j < width - 1; j++)
		{
			dfx = (array_img[(i + 1) * width + j - 1] + 2 * array_img[(i + 1) * width + j] + array_img[(i + 1) * width + j + 1] \
				- array_img[(i - 1) * width + j - 1] - 2 * array_img[(i - 1) * width + j] - array_img[(i - 1) * width + j + 1]) / 8;
			dfy = (array_img[(i - 1) * width + j + 1] + 2 * array_img[(i)* width + j + 1] + array_img[(i + 1) * width + j + 1] \
				- array_img[(i - 1) * width + j - 1] - 2 * array_img[(i)* width + j - 1] - array_img[(i + 1) * width + j - 1]) / 8;

			abs_dfx = fabs(dfx);
			abs_dfy = fabs(dfy);
			if (abs_dfx < 0.0001 && abs_dfy < 0.0001)
			{
				a1 = 0;
				a2 = 0;
			}
			else
			{
				a1 = fabs(abs_dfx - abs_dfy); // / sqrt(abs_dfx*abs_dfx + abs_dfy*abs_dfy);
				float min_df = (abs_dfx < abs_dfy) ? abs_dfx : abs_dfy;
				a2 = sqrt(2.0)*min_df; // /sqrt(abs_dfx*abs_dfx + abs_dfy*abs_dfy);
			}

			if (dfx >= 0 && abs_dfy <= abs_dfx)
			{
				direct_img_7[i * width + j] += a1;
			}
			else if (dfx < 0 && abs_dfy <= abs_dfx)
			{
				direct_img_3[i * width + j] += a1;
			}
			else if (dfy >= 0 && abs_dfy > abs_dfx)
			{
				direct_img_5[i * width + j] += a1;
			}
			else if (dfy < 0 && abs_dfy > abs_dfx)
			{
				direct_img_1[i * width + j] += a1;
			}

			if (dfx >= 0 && dfy >= 0)
			{
				direct_img_6[i * width + j] += a2;
			}
			else if (dfx >= 0 && dfy < 0)
			{
				direct_img_8[i * width + j] += a2;
			}
			else if (dfx < 0 && dfy < 0)
			{
				direct_img_2[i * width + j] += a2;
			}
			else if (dfx < 0 && dfy >= 0)
			{
				direct_img_4[i * width + j] += a2;
			}
		}
	}
	delete[] array_img;

	float * feature = new float[FEAT_DIM];
	int index_gauss = 0;
	int index_feat = 0;
	float feat1 = 0;
	float feat2 = 0;
	float feat3 = 0;
	float feat4 = 0;
	float feat5 = 0;
	float feat6 = 0;
	float feat7 = 0;
	float feat8 = 0;
	for (int i = 2; i < height; i += 5) //iָ�ڼ��У������ʾÿ��5�����ز���һ�Σ���8��20��20��������8��4��4
	{
		for (int j = 2; j < width; j += 5) //jָ�ڼ���
		{
			for (int k = i - size_wds / 2; k < i + size_wds / 2; k++) //k+size_wds/2ָ��˹���еĵڼ���
			{
				for (int l = j - size_wds / 2; l < j + size_wds / 2; l++) //l+size_wds/2ָ��˹���еĵڼ���
				{
					if (k >= 0 && k < height && l >= 0 && l < width)
					{
						feat1 += gauss_win[index_gauss] * direct_img_1[k * width + l];
						feat2 += gauss_win[index_gauss] * direct_img_2[k * width + l];
						feat3 += gauss_win[index_gauss] * direct_img_3[k * width + l];
						feat4 += gauss_win[index_gauss] * direct_img_4[k * width + l];
						feat5 += gauss_win[index_gauss] * direct_img_5[k * width + l];
						feat6 += gauss_win[index_gauss] * direct_img_6[k * width + l];
						feat7 += gauss_win[index_gauss] * direct_img_7[k * width + l];
						feat8 += gauss_win[index_gauss] * direct_img_8[k * width + l];
					}
					index_gauss++; //size_wds���ص���width��k��l���൱��ÿ��direct_img�е���������
				}
			}
			feature[index_feat++] = feat1;
			feature[index_feat++] = feat2;
			feature[index_feat++] = feat3;
			feature[index_feat++] = feat4;
			feature[index_feat++] = feat5;
			feature[index_feat++] = feat6;
			feature[index_feat++] = feat7;
			feature[index_feat++] = feat8; // ��ÿ���������8������������feature����
			feat1 = 0;
			feat2 = 0;
			feat3 = 0;
			feat4 = 0;
			feat5 = 0;
			feat6 = 0;
			feat7 = 0;
			feat8 = 0;
			index_gauss = 0;
		}
	}
	delete[] direct_img_1; // �ͷ��ڴ�
	delete[] direct_img_2;
	delete[] direct_img_3;
	delete[] direct_img_4;
	delete[] direct_img_5;
	delete[] direct_img_6;
	delete[] direct_img_7;
	delete[] direct_img_8;

	return feature;
}


cv::Mat ImageCropPadding(cv::Mat srcImage, cv::Rect rect)
{
	//cv::Mat srcImage = image.clone();
	int crop_x1 = cv::max(0, rect.x);
	int crop_y1 = cv::max(0, rect.y);
	int crop_x2 = cv::min(srcImage.cols, rect.x + rect.width); // ͼ��Χ 0��cols-1, 0��rows-1      
	int crop_y2 = cv::min(srcImage.rows, rect.y + rect.height);

	int left_x = (-rect.x);
	int top_y = (-rect.y);
	int right_x = rect.x + rect.width - srcImage.cols;
	int down_y = rect.y + rect.height - srcImage.rows;  
	cv::Mat roiImage = srcImage(cv::Rect(crop_x1, crop_y1, (crop_x2 - crop_x1), (crop_y2 - crop_y1)));

	if (top_y > 0 || down_y > 0 || left_x > 0 || right_x > 0)//ֻҪ���ڱ߽�Խ������������Ҫ�߽����    
	{
		left_x = (left_x > 0 ? left_x : 0);
		right_x = (right_x > 0 ? right_x : 0);
		top_y = (top_y > 0 ? top_y : 0);
		down_y = (down_y > 0 ? down_y : 0);
		cv::copyMakeBorder(roiImage, roiImage, top_y, down_y, left_x, right_x, cv::BORDER_CONSTANT, cv::Scalar(255));
	}
	return roiImage;
}


void FeatureExtract(Mat srcImg, string file_pixel_out, bool useHanning)
{
	if (file_pixel_out == "") {
		fprintf(stdout, "Result save path is null!\n");
		return;
	}
	// This is different in VS2017 and in sever187 g++
#ifdef VS2017
	FILE *fp_pixel;
	errno_t err;
	err = fopen_s(&fp_pixel, file_pixel_out.c_str(), "wb+");
	if (err) {
		fprintf(stdout, "Create file %s failed!\n", file_pixel_out.c_str());
		return;
	}
#else
	FILE *fp_pixel = fopen(file_pixel_out.c_str(), "wb+");
	if (!fp_pixel)
	{
		fprintf(stdout, "Create file %s failed!\n", file_pixel_out.c_str());
		return;
	}
#endif
	// -------- �Զ�ֵ��ͼ����г߶ȹ�һ�����ֳ�����߶ȱ�Ϊ��Reszie_Rows��pixels��-------- //
	int Reszie_Rows = 60;	//--------�޸Ĺ�һ����ͼ��ĸ߶�--------//
	Mat ResizedImg(Reszie_Rows, Reszie_Rows * srcImg.cols / srcImg.rows, CV_8UC1);
	cv::resize(srcImg, ResizedImg, ResizedImg.size());
	// -------- ��ֵ�� --------//
	//Mat src;
	//ResizedImg.copyTo(src);
	//calcOtsu(img, img.cols, img.rows);
	//src.release();

	int cols = ResizedImg.cols; //��һ��֮��ͼ�����������
	int rows = ResizedImg.rows; //��һ��֮��ͼ����������ߣ�

	int windowCols = 20;	//--------�޸Ĵ���--------//
	int windowRows = 20;	//--------�޸Ĵ���--------//
	int shift_right = 5;	//--------�޸����һ�������--------//
	int shift_down = 5;	//--------�޸����»�������--------//
	int Sample_Num_Cols = (cols - windowCols) / shift_right; //ˮƽ������Ӧ�ñ��г�����֡�����м���֡��
	int Sample_Num_Rows = (rows - windowRows) / shift_down; //��ֱ������Ӧ�ñ��г�����֡�����м���֡��
	if ((cols - windowCols) <= 0)
		Sample_Num_Cols = 1;
	else if (((float)(cols - windowCols) / (float)shift_right - Sample_Num_Cols) < 0.001) //���պ�(cols - windowCols) / shift_rightΪ����ʱ
		Sample_Num_Cols++;
	else
		Sample_Num_Cols += 2;
	if ((rows - windowRows) <= 0)
		Sample_Num_Rows = 1;
	else if (((float)(rows - windowRows) / (float)shift_down - Sample_Num_Rows) < 0.001) //���պ�(rows - windowRows) / shift_downΪ����ʱ
		Sample_Num_Rows++;
	else
		Sample_Num_Rows += 2;
	int nSamples = Sample_Num_Cols * Sample_Num_Rows; //��֡��
	int sampPeriod = 1;
	short paramKind = 9;
	short sampSize_pixel = FEAT_DIM * sizeof(float); //�޸�����ά��--------****
	// sizeof(float) = 4
	// short sampSize_moment = 4 * sizeof(float);

	nSamples = swap32(nSamples);
	sampPeriod = swap32(sampPeriod);
	paramKind = swap16(paramKind);
	sampSize_pixel = swap16(sampSize_pixel);
	//sampSize_moment = swap16(sampSize_moment);
	fwrite(&nSamples, sizeof(int), 1, fp_pixel);
	fwrite(&sampPeriod, sizeof(int), 1, fp_pixel);
	fwrite(&sampSize_pixel, sizeof(short), 1, fp_pixel);
	fwrite(&paramKind, sizeof(short), 1, fp_pixel);

	//------------------------------------------------**//
	Mat grayImg(windowRows, windowCols, CV_8UC1); // �����洢ÿһ��block����ʱ����
	Mat grayImg_hning; // �����洢ÿһ��block����ʱ����
	
#ifdef USE_UNION
	union var data;
#endif
#ifdef SHOW_DETAIL
	int index = 0;
	printf("img_after_resize: width: %d  height: %d\n", ResizedImg.cols, ResizedImg.rows);
	printf("Sample_Num_Cols : %d , Sample_Num_Rows : %d\n", Sample_Num_Cols, Sample_Num_Rows);
	namedWindow("after_resize", 0);
	imshow("after_resize", ResizedImg);
#endif
	for (int j = 0; j < Sample_Num_Rows; j++)
	{
		for (int i = 0; i < Sample_Num_Cols; i++)
		{
			// ���� Rect(int _x,int _y,int _width,int _height);
			cv::Rect window(i*shift_right, j*shift_down, windowCols, windowRows);
			// ��ResizedImg���и�windowָ���ľ���ͼ�񣬲���ֵ��grayImg,������Χʱ�ô���������䣨255��
			grayImg = ImageCropPadding(ResizedImg, window);
#ifdef SHOW_DETAIL
			printf("index:%d\n", index);
			//Mat grayImg_hning; // �����洢ÿһ��block����ʱ����
			//grayImg.copyTo(grayImg_hning);
			printf("img_after_resize: width: %d  height: %d\n", ResizedImg.cols, ResizedImg.rows);
			printf("shfit_r: %d ,shift_d: %d \n", i*shift_right, j*shift_down);
			namedWindow("block", 0);
			imshow("block", grayImg);
#endif
			float *feature = NULL;
			if (useHanning)
			{
				HanningFilter(grayImg, grayImg_hning);

				feature = grad_feature_extract(grayImg_hning);
			}
			else
				feature = grad_feature_extract(grayImg);
#ifdef SHOW_DETAIL
			namedWindow("block_after_hanning", 0);
			useHanning ? imshow("block_after_hanning", grayImg_hning) : imshow("block_after_hanning", grayImg);
#endif
			
			//save feature//
			for (int k = 0; k < FEAT_DIM; k++)
			{
#ifdef USE_UNION
				data.tmp = feature[k] * 100;
				printf("%.2f\t", data.tmp);
#else
				float tmp = feature[k] * 100;
				//printf("%.2f\t", tmp);
#endif

#ifdef SHOW_DETAIL
				printf("%.2f\t", tmp);
				//printf("%.2f\t", data.tmp);
				//printf("%d ", sizeof(char));
				//printf("%.2f\t %02X %02X %02X %02X \t", data.tmp,data.c[0], data.c[1], data.c[2], data.c[3]);
#endif
#ifdef USE_UNION
				data.tmp_int = swap32(data.tmp_int);
				//int int_tmp = data.tmp_int;
				fwrite(data.c, sizeof(float), 1, fp_pixel);
#else
				int tmp_int = *(int*)(&tmp);
				tmp_int = swap32(tmp_int);
				fwrite(&tmp_int, sizeof(float), 1, fp_pixel);
#endif
				//#ifdef SHOW_DETAIL
								//printf("%02X %02X %02X %02X\t", data.c[0], data.c[1], data.c[2], data.c[3]);
								//printf("%02X %02X %02X %02X\n", &data, data.c, &data.tmp, &data.tmp_int);
				//#endif
			}
#ifdef SHOW_DETAIL
			waitKey();
			// waitKey(time_in_ms) = ASCII
			//��һ��������ʱ����(��λms)�ȴ��û���������;
			//����û�û�а��¼�ֵΪASCII�ļ�,������ȴ�(ѭ��)
			index++;
			printf("\n");
#endif
			delete[] feature;
			feature = NULL;
		}
	}
	grayImg.release();
	grayImg_hning.release();
	ResizedImg.release();
	fclose(fp_pixel);
	//fclose(fp_pixel_txt);
}


void Run(char* file_list_in, char* pixel_file_path_out, bool useHanning)
{
	if (!file_list_in || !pixel_file_path_out)
	{
		fprintf(stdout, "Input path is null!\n");
		return;
	}

	// This is different in VS2017 and in sever187 g++
#ifdef VS2017
	FILE *fp;
	errno_t err;
	err = fopen_s(&fp, file_list_in, "r");
	if (err)
	{
		fprintf(stdout, "Open file %s failed!\n", file_list_in);
		return;
	}
#else
	FILE *fp = fopen(file_list_in, "r");
	if (!fp)
	{
		fprintf(stdout, "Open file %s failed!\n", file_list_in);
		return;
	}
#endif

	string outPath_pixel = pixel_file_path_out;
	char c;

	while (!feof(fp))
	{
		char image_name[1024];

		// This is different in VS2017 and in sever187 g++
#ifdef VS2017
		fscanf_s(fp, "%s", image_name, _countof(image_name));
#else
		fscanf(fp, "%s", image_name);
#endif

		if ((c = fgetc(fp)) == EOF)
		{
			break;
		}

		int len = strlen(image_name);
		if (image_name[len - 1] == '\n' && image_name[len - 2] == '\r')
		{
			image_name[len - 2] = '\0';
		}
		if (image_name[len - 1] == '\n' && image_name[len - 2] != '\r')
		{
			image_name[len - 1] = '\0';
		}

		fprintf(stdout, "Processing file %s\n", image_name);
		Mat img = imread(image_name, 0);

		if (img.cols * img.rows == 0)
		{
			fprintf(stdout, "Image %s read failed!\n", image_name);
			continue;
		}

		//#ifdef SHOW_DETAIL
			//	namedWindow("gray", 0);
			//	imshow("gray", img);
		//#endif
		string fileName = image_name;
		string dir;
		string imgName;
		string imgType;
		GetImgInfo(fileName, dir, imgName, imgType);
		string pixel_save_path = outPath_pixel + "/" + imgName + ".htk";
		//printf("%s\n", pixel_save_path.c_str());
		//*******************��ֵ��******************************
		threshold(img, img, 0, 255, CV_THRESH_OTSU);
		//calcOtsu(img, img.cols, img.rows);

		FeatureExtract(img, pixel_save_path, useHanning);
		//img.release();


//#ifdef SHOW_DETAIL
	//	waitKey();
		// waitKey(time_in_ms) = ASCII
		//��һ��������ʱ����(��λms)�ȴ��û���������;
		//����û�û�а��¼�ֵΪASCII�ļ�,������ȴ�(ѭ��)
//#endif
	}
	fclose(fp);
}


int main(int argc, char* argv[])
{
	char* list_name = NULL;
	char* pixel_path = NULL;
	//char* moment_path = NULL;
	bool useHanning = false;

	if (argc != 5 && argc != 7)
	{
		fprintf(stdout, "Miss Parameter! .exe -L list_name -O feature_out[-F useHanning]\n");
		fprintf(stdout, "e.g. : \n./extract_8_direction_2D -L casia_pic_list -O fea_ark.txt -F useHanning\n");
		return -1;
	}

	for (int i = 0; i < argc - 1;)
	{
		char* cmd = argv[i];
		string str = cmd;
		if (str == "-L")
		{
			list_name = argv[i + 1];
			i += 2;
			continue;
		}
		if (str == "-O")
		{
			pixel_path = argv[i + 1];
			i += 2;
			continue;
		}
		if (str == "-F")
		{
			useHanning = true;
			i += 2;
			continue;
		}
		i++;
	}

	if (!list_name || !pixel_path)
	{
		fprintf(stdout, "Miss Parameter!\n");
		return -1;
	}

	//----------------��˹��---------------------//
	int size_wds = GAUSS_WIN_SIZE;
	if (!gauss_win)
	{
		gauss_win = calc_gauss_window(size_wds);
	}

	Run(list_name, pixel_path, useHanning);

	delete[] gauss_win; //��calc_gauss_window�����ĸ�˹�����ڴ��У��������ͷ�

	return 0;
}
