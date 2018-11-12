#pragma once

#include <opencv2/opencv.hpp>
#include <vector>

using namespace cv;
using namespace std;

#include "ComFunc.h"
#include "ConfigurationManage.h"

#define MAX_SITE_COLS		32		//ÿ��die�У�����site����
#define MAX_SITE_ROWS		32		//ÿ��die�У�����site����

#define MAX_THREAD_NUM		32		//֧�ֵ�����߳���

//#define MAX_ALIGN_X		128		//��������뾶����ˮƽ������Χ��128����
//#define MAX_ALIGN_Y		128		//��������뾶������ֱ������Χ��128����

class AlgoGoldenAnalysis
{

public:
	AlgoGoldenAnalysis();
   ~AlgoGoldenAnalysis();

    bool Init(int threadId, int imageWidth, int imageHeight, float fPixelSizeRatio, const char* configPath, const char* outputPath);

    bool SetSiteRect(Rect_t siteRect, Param_t param, ThreshGroup_t thresh, int nSiteX, int nSiteY);
    bool SetRoI(const char* jsonRoiSetting, int nSiteX, int nSiteY);
    void SetAlignRange(int alignOffX, int alignOffY);
	void SetAlignRegion(const char* jsonRegion, int nSiteX, int nSiteY);

	void SetSaveDefectImage(bool bSaveImage, bool bDrawDefects) { m_bSaveImage = bSaveImage; m_bDrawDefects = bDrawDefects; }

    bool SetGoldenSiteImage(unsigned char* imageData, int nSiteX, int nSiteY);

	void SetMatchMode(MatchType mode) { m_MatchMode = mode; }
	void SetRingWidth(int nThres, int nWidth) { m_nRingThres = nThres; m_nRingWidth = nWidth; }
    void SetAlignMinScore(float alignScore) { m_fMinAlignScore = alignScore; };

    AlgorithmErrorType  Analysis(unsigned char* imageData, int nDieId, int nSiteX, int nSiteY);
    AlgorithmErrorType  AnalysisInner(unsigned char* imageData, int nDieId, int nSiteX, int nSiteY,
   									     vector<defectDescription<float> >& outputResult, unsigned char* resultImage);

	static bool	CreateGoldenImage(const string& strFilesPath, unsigned char* goldenImageData, unsigned char* baseImageData, int offX, int offY, const char* jsonRegion);

private:

	static void CreateGoldenImageByMedianFilter(vector<Mat>& vImages, Mat& goldeImage);
	static void CreateGoldenImageByAverageFilter(vector<Mat>& vImages, Mat& goldeImage);
	
	static Point CalcAlignOffset_ocv_cpu(const Mat& baseImage, const Mat& alignImage, int offX, int offY, float &fScore);

	Point MatchImage_CPU(const Mat& baseImage, const Mat& currImage, int nSiteX, int nSiteY, float& alignScore);
	Point CalcAlignOffset_best_cpu(const Mat& baseImage, const Mat& alignImage, int nSiteX, int nSiteY, int offX, int offY, float &fScore);
	Point CalcAlignOffset_proj_cpu(const Mat& alignImage, int offX, int offY, float &fScore);
	double CalcMatchCoeff(const Mat& mat1, const Mat& mat2, double m2, double s2, Mat& prodMat);
		
	bool CalcMatchedRect(Point shiftLoc, Rect& siteRect, Rect& matchRect);
	bool IsRectInRect(Rect& innerRect, Rect& outerRect);

	static void ExtractRegionJson(const char* jsonRegion, vector< Rect > & regions);
	
	bool RemoveIrrelevantRegions(Mat& ioRegionMat, const ConfigurationManage& configManage, vector< vector<Point> >& contours);
	void OutputResult(string& outputPath, vector<defectDescription<float> >& outputResult, Mat& resultMat, int nDieId, int nSiteX, int nSiteY);
	bool ClassifyResult(vector< vector<Point> >& defectsContours, vector<defectDescription<float> >& outputResult, Mat& currImage, Mat& resultImage, Mat& matClustering);
	AlgorithmErrorType  DoAnalysis_cpu(const Mat& currImage, int nSiteX, int nSiteY, vector< vector<Point> >& defectsContours);

	void MarkDefects(Mat& currImage, Mat& highThreshMat, Mat& lowThreshMat, Mat& resultImage);
	void CreateHighLowThreshMat(ThreshGroup_t& threshGroup, Mat& goldenImage, Mat& highThreshMat, Mat& lowThreshMat);
	void CreateRoIMaskImage(const char* jsonROI, Mat& roiMask, vector<RoiInfo_t>& roiInfos);
	void CreateRingMaskImage(const Mat &goldenImage, Mat &maskRingImage, int nThresBin, int nRingWidth);

	bool GrayProjections(const Mat& image, Mat& horzProj, Mat& vertProj);
	
	Mat	  m_GoldenSiteImages[MAX_SITE_ROWS][MAX_SITE_COLS];		// golden image
	Mat	  m_HighThreshMat[MAX_SITE_ROWS][MAX_SITE_COLS];		
	Mat	  m_LowThreshMat[MAX_SITE_ROWS][MAX_SITE_COLS];		
	Mat	  m_GoldenMaskRings[MAX_SITE_ROWS][MAX_SITE_COLS];		// ring mask
	Mat	  m_MaskNonRoI[MAX_SITE_ROWS][MAX_SITE_COLS];			// non RoI mask
		
	vector<RoiInfo_t> m_RoIInfos[MAX_SITE_ROWS][MAX_SITE_COLS];
	string m_RoIJson[MAX_SITE_ROWS][MAX_SITE_COLS];

	vector<Rect> m_AlignRegions[MAX_SITE_ROWS][MAX_SITE_COLS];	//��������

	Mat   m_GoldenSiteHorzProj[MAX_SITE_ROWS][MAX_SITE_COLS];	//ˮƽͶӰ����
	Mat   m_GoldenSiteVertProj[MAX_SITE_ROWS][MAX_SITE_COLS];	//��ֱͶӰ����
	
	double m_GoldenSiteHorzProjMean[MAX_SITE_ROWS][MAX_SITE_COLS];	//ˮƽͶӰ��ֵ
	double m_GoldenSiteHorzProjStd[MAX_SITE_ROWS][MAX_SITE_COLS];	//ˮƽͶӰ��׼��
	double m_GoldenSiteVertProjMean[MAX_SITE_ROWS][MAX_SITE_COLS];	//��ֱͶӰ��ֵ
	double m_GoldenSiteVertProjStd[MAX_SITE_ROWS][MAX_SITE_COLS];	//��ֱͶӰ��׼��

	Mat	  m_DiffFGImage;	//�����ͼ��goldenͼ�Ĳ���ͼ��
	Mat	  m_DiffRegions;	//ȱ�ݱ�Ƕ�ֵͼ��
	Mat   m_ResultImage;	//��ʶȱ�ݵ�RGBͼ��
	// 15M + siteNum*3*2

	Mat   m_HorzProjMat;	//ˮƽͶӰ�����������
	Mat   m_VertProjMat;	//��ֱͶӰ�����������
	Mat   m_ProdMat;		//ͶӰ�������������

	Mat	  m_MorphElement;	//��̬����
	Mat   m_BufferMat;			//�м仺��

	bool  m_bUseCuda;
	static bool  bCreateGoldenImageByMedianFilter;

	ConfigurationManage m_configManage;

	Rect  m_SiteRect[MAX_SITE_ROWS][MAX_SITE_COLS];		//site����
	Param_t m_SiteParam[MAX_SITE_ROWS][MAX_SITE_COLS];	//site������
	ThreshGroup_t m_siteThreshGroups[MAX_SITE_ROWS][MAX_SITE_COLS];
	bool  m_bUseRoI[MAX_SITE_ROWS][MAX_SITE_COLS];		//site���ʱ�Ƿ�ʹ��ROI
	
	string m_configPath;
	string m_outputPath;

	int	m_iImageWidth;
	int	m_iImageHeight;
	float m_fPixelSizeRatio;

	int m_AlignOffX;	//��������뾶����ˮƽ������Χ��m_AlignOffX����
	int m_AlignOffY;	//��������뾶������ֱ������Χ��m_AlignOffY����

	MatchType m_MatchMode;
	float m_fMinAlignScore;
	
	bool m_bInited;			//�Ƿ���ɳ�ʼ��
	
	bool m_bSaveImage;		//�Ƿ񱣴����ȱ�ݵ�ͼ��
	bool m_bDrawDefects;	//�����ͼ���Ƿ��ò�ɫ��ʶȱ��
	int  m_ThreadId;		//��ǰ���߳����

	int m_nRingThres;       //����ring����ֵ
	int m_nRingWidth;       //����ring�����
//////////////////////////////////////////////////////////////////////
public:
	void SetClustering(bool bEnableClustering, unsigned int nRadius);
private:
	Mat m_matClustering;
	bool m_bEnableClustering;
	unsigned int m_nSiteX;
	unsigned int m_nSiteY;
	unsigned int m_nDieId;
	unsigned int m_nRadius = 20;
	
	bool DoSpacialClustering(float fRadius, Mat matSrc);
	void OutputClusteringResult(string& outputPath, vector<defectDescription<float> >& outputResult, Mat& resultMat, int nDieId, int nSiteX, int nSiteY);

};
