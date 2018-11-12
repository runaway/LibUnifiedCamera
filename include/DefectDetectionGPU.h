#ifndef _DEFECT_DETECTION_GPU_H
#define _DEFECT_DETECTION_GPU_H

#include <opencv2/opencv.hpp>
#include <vector>


class DefectDetectionGPU
{
public:		
	~DefectDetectionGPU();

	static DefectDetectionGPU* GetInstance();

	void image_abs(const cv::Mat& immedian1, const cv::Rect& siteRect, std::vector<cv::Mat>& images,
		std::vector<cv::Mat>& images_out);

private:
	// 专用于析构单例类对象m_pInstance.
	class DeleteItSelf
	{
	public:
		~DeleteItSelf()
		{
			if (DefectDetectionGPU::m_pInstance)
			{
				delete m_pInstance;
			}
		}
	};

	static DeleteItSelf m_deleteInstance;	

	DefectDetectionGPU();

	bool InitCUDAMemory(int nWid, int nHei, int nNumImage);

	bool DeleteCUDAMemory();

private:
	static DefectDetectionGPU* m_pInstance;		// 单例类对象
	unsigned char* m_d_img_src;					//cuda中创建原始图像images
	unsigned char* m_d_img_immedian_rotate;		//cuda中创建旋转后的图像集合
	int* m_sum;
	int m_imgWid;
	int m_imgHei;
	int m_nNumImage;

	//创建为上述cuda中的数据赋初值的数组
	int* m_sum1;
	int* m_offset_x;
	int* m_offset_y;
	int* m_offset_z;
	int* m_sum_min;	// 存放一张图像的OPERATION_TIME个求和结果的最小值数组
	int* m_count;	// 存放第几个求和结果为最小值的数组
};


#endif


