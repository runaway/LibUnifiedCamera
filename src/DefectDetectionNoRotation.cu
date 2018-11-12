#include <iostream>
#include<cuda_runtime.h>
#include<device_launch_parameters.h>
#include "DefectDetectionNoRotation.h"


#define shift 4


#define OFFSET_TIME 81

DefectDetectionNoRotation* DefectDetectionNoRotation::m_pInstance = NULL;		// 单例类指针
DefectDetectionNoRotation::DeleteItSelf DefectDetectionNoRotation::m_deleteInstance;


extern "C"
using namespace std;
using namespace cv;

__global__ static void bitwise_and_test(int width, int height, uchar* Images, uchar* immedian, int size_z, int* sum)
{
	int x = blockIdx.x * blockDim.x + threadIdx.x;

	if (x >= size_z)
	{
		return;
	}
	int temp_temp_z = x / OFFSET_TIME;															//第几张图像
	int temp_z = x - (temp_temp_z * OFFSET_TIME);												//第temp_temp_z张图像的temp_z个z值
	int temp_y = temp_z / 9;															//y偏移量
	int temp_x = temp_z - ((temp_z / 9) * 9);											//x偏移量
	for (int temp_height = temp_y; temp_height < height - 8 + temp_y; temp_height++)	//双for循环遍历像素相减
	{
		for (int temp_width = temp_x; temp_width < width - 8 + temp_x; temp_width++)
		{
			int temp = abs(Images[temp_height*width + temp_width + (temp_temp_z)* width*height] - immedian[(temp_height - temp_y + 4)*width + (temp_width - temp_x + 4)]);
			sum[x] = sum[x] + temp;
		}
	}
	
}

DefectDetectionNoRotation::DefectDetectionNoRotation() :
	m_imgWid(0),
	m_imgHei(0),
	m_nNumImage(0),
	m_d_img_src(NULL),
	m_d_img_immedian(NULL),
	m_sum(NULL),
	m_sum1(NULL),
	m_offset_x(NULL),
	m_offset_y(NULL),
	m_sum_min(NULL),
	m_count(NULL)
{
}

DefectDetectionNoRotation::~DefectDetectionNoRotation()
{
	DeleteCUDAMemory();	
}

DefectDetectionNoRotation* DefectDetectionNoRotation::GetInstance()
{
	if (NULL == m_pInstance)
	{
		m_pInstance = new DefectDetectionNoRotation();
	}
	return m_pInstance;
}

void DefectDetectionNoRotation::image_abs(
    const Mat& immedian, 
    const cv::Rect& siteRect,
	vector<Mat>& images, 
	vector<Mat>& images_out)
{
	cout << "cuda no rotate" << endl;
	//在gpu上分配空间
	InitCUDAMemory(immedian.cols, immedian.rows, images.size());
	memset(m_sum1, 0, sizeof(int) * OFFSET_TIME * images.size());
	memset(m_offset_x, 0, sizeof(int) * images.size());
	memset(m_offset_y, 0, sizeof(int) * images.size());
	m_nNumImage = images.size();
	m_imgWid = immedian.cols;
	m_imgHei = immedian.rows;
	

	memset(m_count, 0, m_nNumImage * sizeof(int));
	//cudaMemset(m_sum, 0, OFFSET_TIME * m_nNumImage * sizeof(int));
	cudaMemcpy(m_d_img_immedian, immedian.data, m_imgWid * m_imgHei * sizeof(uchar), 
		cudaMemcpyHostToDevice);
	cudaMemcpy(m_sum, m_sum1, m_nNumImage * OFFSET_TIME * sizeof(int), cudaMemcpyHostToDevice);
	dim3  grid(m_nNumImage * OFFSET_TIME / 32 + 1);
	dim3  block(32);
	for (int i = 0; i < m_nNumImage; i++)
	{
		cudaMemcpy(&m_d_img_src[m_imgWid * m_imgHei * i], images[i].data,
			m_imgWid * m_imgHei * sizeof(uchar), cudaMemcpyHostToDevice);
	}

	bitwise_and_test << <grid, block >> > (m_imgWid, m_imgHei, m_d_img_src, m_d_img_immedian,
		m_nNumImage * OFFSET_TIME, m_sum);

	cudaMemcpy(m_sum1, m_sum, m_nNumImage * OFFSET_TIME * sizeof(int), cudaMemcpyDeviceToHost);

	
	for (int i = 0; i < m_nNumImage; i++)
	{
		m_sum_min[i] = std::numeric_limits<int>::max();
	}	
	
	for (int m_temp = 0; m_temp < 81 * m_nNumImage; m_temp++)
	{
		int temp_z = m_temp / OFFSET_TIME;
		if (m_sum_min[temp_z] > m_sum1[m_temp])//sum_temp1是作为收集最小值的数组，m_temp_count是作为收集偏移量的数组
		{
			m_count[temp_z] = m_temp - (m_temp / OFFSET_TIME) * OFFSET_TIME;
			m_sum_min[temp_z] = m_sum1[m_temp];
		}
	}

	for (int i = 0; i < m_nNumImage; i++)
	{
		m_offset_x[i] = (m_count[i]) - ((m_count[i] / 9) * 9) - 4;
		m_offset_y[i] = (m_count[i]) / 9 - 4;
	}
		

	for (int i = 0; i < m_nNumImage; i++)
	{
		Mat out = Mat::zeros(images[i].size(), images[i].type());
		images[i](Rect(shift + m_offset_x[i], shift + m_offset_y[i],images[i].cols - 2 * shift, 
			images[i].rows - 2 * shift)).copyTo(out(cv::Rect(shift, shift,images[i].cols - 2 * shift,
				images[i].rows - 2 * shift)));
		
		Mat diff = abs(immedian - out);
		
		Mat DiffRegions = Mat::zeros(diff.size(), diff.type());
		diff(siteRect).copyTo(DiffRegions(siteRect));
		
		images_out.push_back(DiffRegions);
	}	
}

void DefectDetectionNoRotation::ImageAbs(const Mat& immedian, const cv::Rect& siteRect,
	vector<Mat>& images, vector<Mat>& images_out)
{
	cout << "cuda no rotate" << endl;
	//在gpu上分配空间
	InitCUDAMemory(immedian.cols, immedian.rows, images.size());
	memset(m_sum1, 0, sizeof(int) * OFFSET_TIME * images.size());
	memset(m_offset_x, 0, sizeof(int) * images.size());
	memset(m_offset_y, 0, sizeof(int) * images.size());
	m_nNumImage = images.size();
	m_imgWid = immedian.cols;
	m_imgHei = immedian.rows;
	

	memset(m_count, 0, m_nNumImage * sizeof(int));
	//cudaMemset(m_sum, 0, OFFSET_TIME * m_nNumImage * sizeof(int));
	cudaMemcpy(m_d_img_immedian, immedian.data, m_imgWid * m_imgHei * sizeof(uchar), 
		cudaMemcpyHostToDevice);
	cudaMemcpy(m_sum, m_sum1, m_nNumImage * OFFSET_TIME * sizeof(int), cudaMemcpyHostToDevice);
	dim3  grid(m_nNumImage * OFFSET_TIME / 32 + 1);
	dim3  block(32);
	for (int i = 0; i < m_nNumImage; i++)
	{
		cudaMemcpy(&m_d_img_src[m_imgWid * m_imgHei * i], images[i].data,
			m_imgWid * m_imgHei * sizeof(uchar), cudaMemcpyHostToDevice);
	}

	bitwise_and_test << <grid, block >> > (m_imgWid, m_imgHei, m_d_img_src, m_d_img_immedian,
		m_nNumImage * OFFSET_TIME, m_sum);

	cudaMemcpy(m_sum1, m_sum, m_nNumImage * OFFSET_TIME * sizeof(int), cudaMemcpyDeviceToHost);

	
	for (int i = 0; i < m_nNumImage; i++)
	{
		m_sum_min[i] = std::numeric_limits<int>::max();
	}	
	
	for (int m_temp = 0; m_temp < 81* m_nNumImage; m_temp++)
	{
		int temp_z = m_temp / OFFSET_TIME;
		if (m_sum_min[temp_z] > m_sum1[m_temp])//sum_temp1是作为收集最小值的数组，m_temp_count是作为收集偏移量的数组
		{
			m_count[temp_z] = m_temp - (m_temp / OFFSET_TIME) * OFFSET_TIME;
			m_sum_min[temp_z] = m_sum1[m_temp];
		}
	}

	for (int i = 0; i < m_nNumImage; i++)
	{
		m_offset_x[i] = (m_count[i]) - ((m_count[i] / 9) * 9) - 4;
		m_offset_y[i] = (m_count[i]) / 9 - 4;
	}
		

	for (int i = 0; i < m_nNumImage; i++)
	{
		Mat out = Mat::zeros(images[i].size(), images[i].type());
		images[i](Rect(shift + m_offset_x[i], shift + m_offset_y[i],images[i].cols - 2 * shift, 
			images[i].rows - 2 * shift)).copyTo(out(cv::Rect(shift, shift,images[i].cols - 2 * shift,
				images[i].rows - 2 * shift)));
		
		Mat diff = abs(immedian - out);
		
		Mat DiffRegions = Mat::zeros(diff.size(), diff.type());
		diff(siteRect).copyTo(DiffRegions(siteRect));
		
		images_out.push_back(DiffRegions);
	}	
}

bool DefectDetectionNoRotation::InitCUDAMemory(int nWid, int nHei, int nNumImage)
{
	///lq 如已经按当前输入情况分配过内存，不再进行分配。内存分配会耗费时间
	if (m_imgWid == nWid && m_imgHei == nHei && m_nNumImage == nNumImage
		&& NULL != m_sum && NULL != m_d_img_src && NULL != m_d_img_immedian)
	{
		return false;
	}

	//	cout << "InitCUDAMemory()!!!!!!!!!!!!!!!!!" << endl;

	///lq 如要重新分配，先释放内存
	DeleteCUDAMemory();

	cudaMalloc((void**)&m_d_img_immedian, nWid * nHei * sizeof(uchar));
	cudaMalloc((void**)&m_d_img_src, nNumImage * nWid * nHei * sizeof(uchar));
	cudaMalloc((void**)&m_sum, nNumImage * OFFSET_TIME * sizeof(int));


	m_sum1 = new int[OFFSET_TIME * nNumImage];			// 对应sum
	m_offset_x = new int[nNumImage];						// 对应偏移量x
	m_offset_y = new int[nNumImage];
	m_sum_min = new int[nNumImage];
	m_count = new int[nNumImage];

	return true;
}

bool DefectDetectionNoRotation::DeleteCUDAMemory()
{
	if (NULL != m_sum)
	{
		cudaFree(m_sum);
		m_sum = NULL;
	}

	if (NULL != m_d_img_src)
	{
		cudaFree(m_d_img_src);
		m_d_img_src = NULL;
	}

	if (NULL != m_d_img_immedian)
	{
		cudaFree(m_d_img_immedian);
		m_d_img_immedian = NULL;
	}

	if (NULL != m_offset_x)
	{
		delete[] m_offset_x;
		m_offset_x = NULL;
	}

	if (NULL != m_offset_y)
	{
		delete[] m_offset_y;
		m_offset_y = NULL;
	}

	if (NULL != m_sum_min)
	{
		delete[] m_sum_min;
		m_sum_min = NULL;
	}

	if (NULL != m_count)
	{
		delete[] m_count;
		m_count = NULL;
	}

	return true;
}