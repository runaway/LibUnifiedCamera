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
	// ר�����������������m_pInstance.
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
	static DefectDetectionGPU* m_pInstance;		// ���������
	unsigned char* m_d_img_src;					//cuda�д���ԭʼͼ��images
	unsigned char* m_d_img_immedian_rotate;		//cuda�д�����ת���ͼ�񼯺�
	int* m_sum;
	int m_imgWid;
	int m_imgHei;
	int m_nNumImage;

	//����Ϊ����cuda�е����ݸ���ֵ������
	int* m_sum1;
	int* m_offset_x;
	int* m_offset_y;
	int* m_offset_z;
	int* m_sum_min;	// ���һ��ͼ���OPERATION_TIME����ͽ������Сֵ����
	int* m_count;	// ��ŵڼ�����ͽ��Ϊ��Сֵ������
};


#endif


