#include <iostream>
#include <cuda_runtime.h>
#include<device_launch_parameters.h>
#include "DefectDetectionGPU.h"

#define shift 4

#define ROTATE_TIME  21
#define OFFSET_TIME 81
#define OPERATION_TIME  (21 * 81)
static float START_ANGLE = -1.0;
static float END_ANGLE = 1.0;
static float STEP_ANGLE = 0.1;
DefectDetectionGPU* DefectDetectionGPU::m_pInstance = NULL;		// 单例类指针
DefectDetectionGPU::DeleteItSelf DefectDetectionGPU::m_deleteInstance;


extern "C"
using namespace std;
using namespace cv;
 
//cuda中的带旋转的模板匹配相减
__global__ static void rotate_abs(int width, int height, uchar* Images, uchar* immedian, int* sum, int size_t)

{ 
	int x = blockIdx.x * blockDim.x + threadIdx.x;
	if (x >= size_t)
	{
		return;
	}
	{
		int img_num = x / OPERATION_TIME;					//处理的第几张图像
		int temp = x - img_num * OPERATION_TIME;			//第img_num张图像的第temp次处理，作为求后续值的依据
		int immedian_num = temp / OFFSET_TIME;				//第img_num张图像对应的旋转的第immedian_num张图像
		int temp_z = temp - OFFSET_TIME * immedian_num;		//从0到80中平移的第temp_z次，作为求x,y轴平移距离的依据
		int temp_y = temp_z / 9;							//y轴平移向下移动的距离
		int temp_x = temp_z - temp_y * 9;					//x轴向右移动的距离
		for (int temp_height = temp_y; temp_height < height - 8 + temp_y; temp_height++)//双for循环遍历像素相减
		{
			for (int temp_width = temp_x; temp_width < width - 8 + temp_x; temp_width++)
			{
				int temp = abs(Images[temp_height*width + temp_width + img_num* width*height] - immedian[(temp_height - temp_y + 4)*width + (temp_width - temp_x + 4) + immedian_num*width*height]);
				sum[x] = sum[x] + temp;
			}
		}
	}
}

DefectDetectionGPU::DefectDetectionGPU():
	m_imgWid(0),
	m_imgHei(0),
	m_nNumImage(0),
	m_d_img_src(NULL),
	m_d_img_immedian_rotate(NULL),
	m_sum(NULL),
	m_sum1(NULL),
	m_offset_x(NULL),
	m_offset_y(NULL),
	m_offset_z(NULL),
	m_sum_min(NULL),
	m_count(NULL)
{
	
}

DefectDetectionGPU::~DefectDetectionGPU()
{
	DeleteCUDAMemory();	
}

DefectDetectionGPU* DefectDetectionGPU::GetInstance()
{
	if (NULL == m_pInstance)
	{
		m_pInstance = new DefectDetectionGPU();
	}
	return m_pInstance;
}

void DefectDetectionGPU::image_abs(const Mat& immedian1, const cv::Rect& siteRect, vector<Mat>& images,
	vector<Mat>& images_out)
{
	cout << "cuda with rotate" << endl;
	//在gpu上分配空间
	InitCUDAMemory(immedian1.cols, immedian1.rows, images.size());

	memset(m_sum1, 0, sizeof(int) * OPERATION_TIME * images.size());
	memset(m_offset_x, 0, sizeof(int) * images.size());
	memset(m_offset_y, 0, sizeof(int) * images.size());
	memset(m_offset_z, 0, sizeof(int) * images.size());
	
	m_nNumImage = images.size();
	m_imgWid = immedian1.cols;
	m_imgHei = immedian1.rows;

	for (int i = 0; i < m_nNumImage; i++)
	{
		m_sum_min[i] = std::numeric_limits<int>::max();
	}
	memset(m_count, 0, m_nNumImage * sizeof(int));
	cudaMemset(m_sum, 0, OPERATION_TIME * m_nNumImage * sizeof(int));

	vector<Mat> rotate_images;	//images作为存放初始图像数据的数组，rotate_images作为存放标准图像旋转后得到的图像数组
	vector<float> float_temp;	//对目标图片设置旋转角度并保存在float_temp中
	
	// 记录标准图像旋转每个角度得到的图像
	Mat image2;
	for (float j = START_ANGLE; j <= END_ANGLE; j += STEP_ANGLE)
	{
		image2 = immedian1.clone();
		float angle = j;
		Point2f center((float)(immedian1.cols / 2), (float)(immedian1.rows / 2));
		Mat affine_matrix = getRotationMatrix2D(center, angle, 1.0);//旋转矩阵
		warpAffine(image2, image2, affine_matrix, immedian1.size());//旋转
		rotate_images.push_back(image2);//放入图像数组
	}
	
	//使用上面的数据为gpu上的数组赋初值
	for (int i = 0; i < rotate_images.size(); i++)
	{
		cudaMemcpy(&m_d_img_immedian_rotate[m_imgWid * m_imgHei * i],
			rotate_images[i].data, 
			m_imgWid * m_imgHei * sizeof(uchar), cudaMemcpyHostToDevice);
	}

	for (int i = 0; i < m_nNumImage; i++)
	{
		cudaMemcpy(&m_d_img_src[m_imgWid * m_imgHei * i], 
			images[i].data, m_imgWid * m_imgHei * sizeof(uchar),
			cudaMemcpyHostToDevice);
	}

	//设置cuda的线程数通过gird和block的分配，现在使用的是一维的
	dim3  grid(OPERATION_TIME * m_nNumImage / 32 + 1);
	dim3  block(32);
	rotate_abs <<<grid, block >>> (m_imgWid, m_imgHei, m_d_img_src,
		m_d_img_immedian_rotate, m_sum, OPERATION_TIME * m_nNumImage);

	//将数据传回内存,这里只将结果数组传回
	cudaMemcpy(m_sum1, m_sum, OPERATION_TIME * m_nNumImage * sizeof(int), cudaMemcpyDeviceToHost);

	//遍历所有的和值记录每张图像对应的OPERATION_TIME个和值中的最小值以及最小值的对应位置
	for (int m_temp = 0; m_temp < OPERATION_TIME * m_nNumImage; m_temp++)
	{
		int temp_offset = m_temp / OPERATION_TIME;	//找出
		if (m_sum_min[temp_offset] > m_sum1[m_temp])
		{
			m_sum_min[temp_offset] = m_sum1[m_temp];
			m_count[temp_offset] = m_temp;
		}
	}

	//通过最小值所对应的位置，计算出偏移量m_offset_x，m_offset_y和旋转角m_offset_z
	for (int i = 0; i < m_nNumImage; i++)
	{
		int temp_offset = m_count[i] - OPERATION_TIME * i;
		m_offset_z[i] = temp_offset / OFFSET_TIME;
		int temp_xy_offset = temp_offset - (temp_offset / OFFSET_TIME) * OFFSET_TIME;
		m_offset_y[i] = temp_xy_offset / 9 - 4;
		m_offset_x[i] = temp_xy_offset - ((temp_xy_offset / 9) * 9) - 4;
	}

	//存储相减后的图像结果
	for (int i = 0; i < m_nNumImage; i++)
	{
		Mat out = Mat::zeros(images[i].size(), images[i].type());
		images[i](Rect(shift + m_offset_x[i], shift + m_offset_y[i],
			images[i].cols - 2 * shift, images[i].rows 
			- 2 * shift)).copyTo(out(cv::Rect(shift, shift, 
				images[i].cols - 2 * shift, images[i].rows - 2 * shift)));

		Mat diff = abs(rotate_images[m_offset_z[i]] - out);

		Mat DiffRegions = Mat::zeros(diff.size(), diff.type());
		diff(siteRect).copyTo(DiffRegions(siteRect));

		//imwrite("image" + to_string(i) + ".bmp", diff);
		images_out.push_back(DiffRegions);
	}	
}

bool DefectDetectionGPU::InitCUDAMemory(int nWid, int nHei, int nNumImage)
{
	///lq 如已经按当前输入情况分配过内存，不再进行分配。内存分配会耗费时间
	if (m_imgWid == nWid && m_imgHei == nHei && m_nNumImage == nNumImage
		&& NULL != m_sum && NULL != m_d_img_src && NULL != m_d_img_immedian_rotate)
	{
		return false;
	}

	cout << "InitCUDAMemory()!!!!!!!!!!!!!!!!!" << endl;

	///lq 如要重新分配，先释放内存
	DeleteCUDAMemory();

	cudaMalloc((void**)&m_d_img_immedian_rotate, ROTATE_TIME * nWid * nHei * sizeof(uchar));
	cudaMalloc((void**)&m_d_img_src, nNumImage * nWid * nHei * sizeof(uchar));
	cudaMalloc((void**)&m_sum, OPERATION_TIME * nNumImage * sizeof(int));	
	

	m_sum1 = new int[OPERATION_TIME * nNumImage];			// 对应sum
	m_offset_x = new int[nNumImage];						// 对应偏移量x
	m_offset_y = new int[nNumImage];						// 对应偏移量y
	m_offset_z = new int[nNumImage];
	m_sum_min = new int[nNumImage];
	m_count = new int[nNumImage];
	

	return true;
}

bool DefectDetectionGPU::DeleteCUDAMemory()
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

	if (NULL != m_d_img_immedian_rotate)
	{
		cudaFree(m_d_img_immedian_rotate);
		m_d_img_immedian_rotate = NULL;
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

	if (NULL != m_offset_z)
	{
		delete[] m_offset_z;
		m_offset_z = NULL;
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