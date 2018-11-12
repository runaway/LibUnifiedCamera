#include "stdafx.h"
//#include <cuda_runtime.h>
#include "DefectsAnalysis.h"
#include "DefectsClassify.h"
#include "DefectDetection.h"
#include "DataConvert.h"
#include "DefectsClustering.h"
#include <shlwapi.h>
#include "ComFunc.h"


#pragma comment(lib, "shlwapi.lib")


//#define WITH_CUDA
// 目标机是否有CUDA
bool gHaveCUDA = false;

// 是否已检测过目标机CUDA的设置
bool gHaveQueryCUDA = false;


using namespace std;
using namespace cv;


#if SAVE_RESULT_TO_FILE
string RESULT_SAVE_PATH = ".\\resultPath\\";
#endif

static bool bHaveQueryCUDA = false;		// 是否已经查询过CUDA配置


// ------------------------------------------------------------
// Description : 根据配置文件参数去除无关区域
// Parameters :  ioRegionMat: 【输入输出值】检出区域
//				 configManage: 配置参数对象
// Return Value :true - 成功
//				 false - 无轮廓区域
// ------------------------------------------------------------
bool RemoveIrrelevantRegions(Mat& ioRegionMat, const ConfigurationManage& configManage)
{
	vector< vector<cv::Point> > contours;	
	vector<Vec4i> hierarchy;

	findContours(ioRegionMat, contours, hierarchy,
			CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);	

	if (contours.empty())
	{
		return false;
	}

	vector< vector<cv::Point> >::iterator it = contours.begin();
	int nContourNum = contours.size();
	int erasedNum = 0;
	bool setToZero = false;

	for (int i = 0; i < nContourNum; ++i)
	{
		// 将小于配置规定面积的区域填充为0
		double area = contourArea(contours[i]);
		if (configManage.m_MinArea > 0)
		{
			setToZero = (area < configManage.m_MinArea);
		}		

		// 将小于配置规定size的区域填充为0
		if (configManage.m_MinSize > 0)
		{
			RotatedRect boundingBox = minAreaRect(contours[i]);
			setToZero = (setToZero || boundingBox.size.height < configManage.m_MinSize);
		}
		
		if (setToZero)
		{
			drawContours(ioRegionMat, contours, i, Scalar(0), CV_FILLED);
			erasedNum++;
		}
	}



	if(erasedNum == nContourNum)	// 全部消除
		return false;
	return true;
}
#ifdef WITH_CUDA
// ------------------------------------------------------------
// Description : 检测当前目标机是否安装CUDA
// Parameters :  无
// Return Value :true - 有CUDA
//				 false - 无CUDA
// ------------------------------------------------------------
bool QueryCUDA()
{
	int deviceCount = 0;
	cudaError_t error_id = cudaGetDeviceCount(&deviceCount);

	if (error_id != cudaSuccess)
	{
		printf("cudaGetDeviceCount returned %d\n-> %s\n", (int)error_id, cudaGetErrorString(error_id));
		printf("Result = FAIL\n");
	}

	if (deviceCount == 0)
	{
		printf("There are no available device(s) that support CUDA\n");
		return false;
	}
	else
	{
		printf("Detected %d CUDA Capable device(s)\n", deviceCount);
		return true;
	}
}
#else
bool QueryCUDA()
{
    return false;
}
#endif
// ------------------------------------------------------------
// Description : 缺陷“检测/分类/聚类”对外接口,单文件接口
// Parameters :  dstImgPath：待检图像路径
//               goldenImgPath：金标准图像文件夹路径
//			     configName: 使用的配置文件名称
//				 siteRect: site的检测区域	
//				 pixelSizeRatio: 每像素对应的物理尺寸
//				 jsonResult: 【输出值】json格式数值结果的字符串
//				 outputImgPath:【输出值】结果图像路径
//				 outputResult: 【输出值】stl容器保存的结果
// Return Value :true - 成功
//			     false - 失败
// ------------------------------------------------------------
bool DoAnalysisInner(const std::string& dstImgPath,
	const std::string& goldenImgPath,
	const std::string& configName,
	cv::Rect siteRect,
	const ROI_t& roiRegion,
	double pixelSizeRatio,
	std::string& jsonResult,
	std::string& outputImgPath,
	std::vector<defectDescription<float> >& outputResult,		// 给软件的时候删掉
	cv::Mat& resultMat,				// 给软件的时候删掉
	bool bWithRotate,				// 给软件的时候删掉
	AlignmentRange sAlignmentRange)				
{
	// 查询目标机是否安装了CUDA，如果没有安装CUDA或没有GPU，使用CPU版本
	if (!gHaveQueryCUDA)
	{
		gHaveQueryCUDA = true;
		QueryCUDA();
	}

	long startTime = GetTickCount();

	// 加载configuration配置文件	
	ConfigurationManage configManage;
	if (!configManage.LoadConfigurationFile(configName))
	{		
		return false;
	}
	
	unsigned char grayThreshold;
	if (configManage.m_thresholdType == MANUAL)
	{
		grayThreshold = configManage.m_manualThreshold;
	}
	else // TODO: 建立Threshold Map
	{
		grayThreshold = 50;
	}
	
	long endTime = GetTickCount();	
	cout << "load configuration file cost time : " << endTime - startTime << endl;

	startTime = endTime;

	//----------------------------------- step 1: 配准、相减	
	Mat dstImg = imread(dstImgPath.c_str(), CV_LOAD_IMAGE_GRAYSCALE);
	
	if (dstImg.empty())
	{
		std::cout << "open image " << dstImgPath.c_str() << "fail!" << std::endl;
		return false;
	}

	// site region的有效性判断，限制其在有效范围内
	if (siteRect.width <= 0 || siteRect.height <= 0 || siteRect.width > dstImg.cols 
		|| siteRect.height > dstImg.rows)
	{
		siteRect.x = 0;
		siteRect.y = 0;
		siteRect.width = dstImg.cols;
		siteRect.height = dstImg.rows;
	}

	siteRect.x = 128;
	siteRect.y = 128;
	siteRect.width = dstImg.cols - siteRect.x*2;
	siteRect.height = dstImg.rows - siteRect.y*2;
	

	// 创建golden die
	Mat goldenImg;
	Mat matBaseImg;
	DefectDetection defectDetection;

	goldenImg = Mat::zeros(dstImg.size(), dstImg.type());	
    
	if (!defectDetection.CreateGoldenImage(goldenImgPath, goldenImg, matBaseImg, sAlignmentRange))
	{
		return false;
	}


	Mat maskImg, maskInvImg;
	if (!defectDetection.CreateGoldenMaskImage(goldenImg, maskImg))
	{
		return false;
	}

	bool bNoRoiExist = true;
	Mat roiImg;
	vector<Rect> boundPolyRect;
	vector<Rect> boundCircleRect;
	if((roiRegion.polyNum>0) || (roiRegion.circleNum>0))
	{
		bNoRoiExist = false;
		roiImg = Mat::zeros(dstImg.size(), dstImg.type());
		if(roiRegion.polyNum>0)
		{
			vector<vector<Point> > contours_poly( roiRegion.polyNum );
			
			for(int i=0; i<roiRegion.polyNum; i++)
			{
				int pt_num = roiRegion.polyArray[i].vertexNum;
				Point* poly_points = new Point[pt_num];
				for(int k=0; k<pt_num; k++)
				{
					Point pt;
					pt.x = roiRegion.polyArray[i].vertexArray[k].x;
					pt.y = roiRegion.polyArray[i].vertexArray[k].y;

					poly_points[k] = pt;
					contours_poly[i].push_back(pt);
				}
				const Point* ppt[1] = { poly_points };
  				int npt[] = { pt_num };
				fillPoly( roiImg, ppt,  npt, 1, Scalar( 255), LINE_8 );
				delete [] poly_points;

				Rect bound;
				bound = boundingRect( Mat(contours_poly[i]) );
				boundPolyRect.push_back(bound);
				//rectangle( roiImg, bound.tl(), bound.br(), Scalar( 255), 2, 8, 0 );
			}
		}
		
		if(roiRegion.circleNum>0)
		{
			for(int i=0; i<roiRegion.circleNum; i++)
			{
				Point center;
				center.x = roiRegion.circleArray[i].center.x;
				center.y = roiRegion.circleArray[i].center.y;
				int radius;
				radius = roiRegion.circleArray[i].radius;
				circle(roiImg, center, radius, Scalar(255), FILLED, LINE_8);

				Rect bound;
				bound.x = center.x - radius;
				bound.y = center.y - radius;
				bound.width = radius*2;
				bound.height = radius*2;
				boundCircleRect.push_back(bound);
			}
		}

	}
	Mat defectRegionsFg, defectRegionsBg;	
	Mat diffMatFg, diffMatBg, diffMat;	
	if (!defectDetection.Detection(dstImg, goldenImg, maskImg, siteRect, grayThreshold, defectRegionsFg, defectRegionsBg, diffMat,
		bWithRotate))
	{
		return false;
	}

	//----------------------------------- step 2: 根据配置文件参数去除无关区域
	bool bVerifyFg, bVerifyBg;
	bVerifyFg = RemoveIrrelevantRegions(defectRegionsFg, configManage);
	bVerifyBg = RemoveIrrelevantRegions(defectRegionsBg, configManage);
	
	if ((!bVerifyFg) && (!bVerifyBg))
	{
		return false;
	}
	
	//----------------------------------- step 3: 缺陷分类	

	DefectsClassify defectClassify;
	Mat resultMatFg, resultMatBg;
	std::vector<defectDescription<float> > outputFg, outputBg;
	
	bVerifyFg = defectClassify.DoClassify(defectRegionsFg, dstImg, pixelSizeRatio,
		configManage.m_strategyName, resultMatFg, outputFg);
	bVerifyBg = defectClassify.DoClassify(defectRegionsBg, dstImg, pixelSizeRatio,
		configManage.m_strategyName, resultMatBg, outputBg);
	//bVerifyBg = false;
	
	if ((!bVerifyFg) && (!bVerifyBg))
	{
		std::cout << "DefectsClassify::DoClassify() fail!" << std::endl;
		return false;
	}

	resultMat = resultMatFg | resultMatBg;

	outputResult.clear();
	if(outputFg.size() > 0)
	{
		int size = outputFg.size();
		for(int i=0; i<size; i++)
			outputResult.push_back(outputFg[i]);
	}
	if(outputBg.size() > 0)
	{
		int size = outputBg.size();
		for(int i=0; i<size; i++)
			outputResult.push_back(outputBg[i]);
	}

	//cv::rectangle(resultMat, siteRect, Scalar(0, 255, 255), 2);

	// 保存图像结果到文件
	string rstImgPath = outputImgPath + "\\resultImage.png";
	imwrite(rstImgPath, resultMat);

	//----------------------------------- step 4: 缺陷聚类
	// 还有问题，先封上
#if 0
	if (configManage.m_bEnableClustering)
	{		
		DefectsClustering cluster;
		string FileName = "";
		if (configManage.m_clusterType != CLUSTER_NONE)
		{
			FileName = (configManage.m_clusterType == CLUSTER_ROI)
				? configManage.m_ROIMaskName : configManage.m_clusterStrategyName;
		}
		cluster.DoClustering(configManage.m_clusterType, FileName,
			configManage.m_clusterRadius, pixelSizeRatio, defectRegions,
			outputResult);		
	}
#endif

	//----------------------------------- step 5: 将结果写入json文件
#if SAVE_RESULT_TO_FILE
	if (!PathIsDirectory(RESULT_SAVE_PATH.c_str()))
	{
		CreateDirectory(RESULT_SAVE_PATH.c_str(), NULL);
	}

	string imgFileName = "ResultImage.bmp";
	string imgSavePath = RESULT_SAVE_PATH + imgFileName;
	imwrite(imgFileName, resultMat);

	string fileName = "DefectsData.json";
	string savePath = RESULT_SAVE_PATH + fileName;
	DefectDescriptionStructToJsonFile(outputResult, savePath);
#endif
}

#if 0
extern "C"  __declspec(dllexport)
bool DoAnalysis(char* dstImgPath,
	char* goldenImgPath,
	char* configName,
	URect siteRect,
	double pixelSizeRatio,
	char* jsonResult,
	char* outputImgPath)
{
	//AfxMessageBox("DoAnalysis Void Begin");
	AfxMessageBox(dstImgPath);

	return true;
}
#endif

#if 0
// 拆分之前
extern "C"  __declspec(dllexport)
bool DoAnalysis(char* dstImgPath,
	char*  goldenImgPath,
	char*  configName,
	URect siteRect,
	double pixelSizeRatio,
	char* jsonResult,
	char* outputImgPath)
{
	bool bParallel = true;
	bool bWithRotate = true;
    std::vector<defectDescription<float> > outputResult;
	Mat resultMat;

	//AfxMessageBox("DoAnalysis Begin");
#if 0
	char szTemp[255];
	sprintf(szTemp, "(%f, %f, %f, %f)", siteRect.x, siteRect.y, siteRect.width, siteRect.height);
	AfxMessageBox((LPCTSTR)szTemp, 0);
#endif

	// 查询目标机是否安装了CUDA，如果没有安装CUDA或没有GPU，使用CPU版本
	if (!gHaveQueryCUDA)
	{
		gHaveQueryCUDA = true;
		QueryCUDA();
	}

	long startTime = GetTickCount();

	// 加载configuration配置文件	
	ConfigurationManage configManage;
	if (!configManage.LoadConfigurationFile(configName))
	{	
		AfxMessageBox("configManage.LoadConfigurationFile Fail!");
		return false;
	}
	
	unsigned char grayThreshold;
	if (configManage.m_thresholdType == MANUAL)
	{
		grayThreshold = configManage.m_manualThreshold;
	}
	else // TODO: 建立Threshold Map
	{
		grayThreshold = 50;
	}
	
	long endTime = GetTickCount();	
	cout << "load configuration file cost time : " << endTime - startTime << endl;

	startTime = endTime;

	//----------------------------------- step 1: 配准、相减	
	Mat dstImg = imread(dstImgPath, CV_LOAD_IMAGE_GRAYSCALE);
	
	if (dstImg.empty())
	{
		std::cout << "open image " << dstImgPath << "fail!" << std::endl;
		AfxMessageBox("dstImg.empty() Fail!");
		return false;
	}

	// site region的有效性判断，限制其在有效范围内
	if (siteRect.width <= 0 || siteRect.height <= 0 || siteRect.width > dstImg.cols 
		|| siteRect.height > dstImg.rows)
	{
		siteRect.x = 0;
		siteRect.y = 0;
		siteRect.width = dstImg.cols;
		siteRect.height = dstImg.rows;
	}

	cv::Rect cvrectSite;
	cvrectSite.x = siteRect.x;
	cvrectSite.y = siteRect.y;
	cvrectSite.width = siteRect.width;
	cvrectSite.height = siteRect.height;

	// 创建golden die
	Mat goldenImg;
	DefectDetection defectDetection;
    
	if (!defectDetection.CreateGoldenImage(goldenImgPath, goldenImg))
	{
		AfxMessageBox("defectDetection.CreateGoldenImage Fail!");
		return false;
	}

    imwrite("GoldenImg.bmp", goldenImg);
	//AfxMessageBox("After imwrite GoldenImg.bmp");

	Mat defectRegions;
	if (!defectDetection.Detection(dstImg, goldenImg, cvrectSite, grayThreshold, defectRegions,
		bWithRotate))
	{
		AfxMessageBox("defectDetection.Detection Fail!");

		return false;
	}
	
	//----------------------------------- step 2: 根据配置文件参数去除无关区域
	if (!RemoveIrrelevantRegions(defectRegions, configManage))
	{
		AfxMessageBox("RemoveIrrelevantRegions Fail!");
		return false;
	}
	//imwrite("./refinedDiffRegion.png", defectRegions);

	//----------------------------------- step 3: 缺陷分类	
// 	 Mat resultMat;			// 给软件的时候打开
// 	std::vector<defectDescription<float> > outputResult;	// 给软件的时候打开
	DefectsClassify defectClassify;	
	
	if (!defectClassify.DoClassify(defectRegions, dstImg, pixelSizeRatio,
		configManage.m_strategyName, resultMat, outputResult))
	{
		std::cout << "DefectsClassify::DoClassify() fail!" << std::endl;
		AfxMessageBox("defectClassify.DoClassify Fail!");
		return false;
	}

	//cv::rectangle(resultMat, cvrectSite, Scalar(0, 255, 255), 2);

	// 保存图像结果到文件
	//string rstImgPath = outputImgPath + "\\resultImage.png";
	string rstImgPath = outputImgPath;
	rstImgPath += "\\resultImage.png";
	imwrite(rstImgPath, resultMat);

	//----------------------------------- step 4: 缺陷聚类
	// 还有问题，先封上
#if 0
	if (configManage.m_bEnableClustering)
	{		
		DefectsClustering cluster;
		string FileName = "";
		if (configManage.m_clusterType != CLUSTER_NONE)
		{
			FileName = (configManage.m_clusterType == CLUSTER_ROI)
				? configManage.m_ROIMaskName : configManage.m_clusterStrategyName;
		}
		cluster.DoClustering(configManage.m_clusterType, FileName,
			configManage.m_clusterRadius, pixelSizeRatio, defectRegions,
			outputResult);		
	}
#endif

	//----------------------------------- step 5: 将结果写入json文件
#if SAVE_RESULT_TO_FILE
	if (!PathIsDirectory(RESULT_SAVE_PATH.c_str()))
	{
		CreateDirectory(RESULT_SAVE_PATH.c_str(), NULL);
	}

	string imgFileName = "resultImage.bmp";
	string imgSavePath = RESULT_SAVE_PATH + imgFileName;
	imwrite(imgFileName, resultMat);

	string fileName = "defectsData.json";
	string savePath = RESULT_SAVE_PATH + fileName;
	DefectDescriptionStructToJsonFile(outputResult, savePath);
#endif
}
#endif


extern "C"  __declspec(dllexport)
int DoAnalysis(
	unsigned char* pbyDstImgData,
	unsigned char* pbyGoldenImgData,
	char*  configName,
	URectangle urectSite,
	char* szROIRegion,
	char* szROINegRegion,
	double pixelSizeRatio,
	char* jsonResult,
	char* outputImgPath,
	char* szDieID,
	char* szSiteID,
	MatchType matchtype)
{
	bool bParallel = true;
	bool bWithRotate = true;
    std::vector<defectDescription<float> > outputResult;
	Mat resultMat;

	Mat matDstImg(IMAGE_HEIGHT, IMAGE_WIDTH, CV_8UC1, Scalar(0));
    Mat matGoldenImg(IMAGE_HEIGHT, IMAGE_WIDTH, CV_8UC1, Scalar(0));

	// 查询目标机是否安装了CUDA，如果没有安装CUDA或没有GPU，使用CPU版本
	if (!gHaveQueryCUDA)
	{
		gHaveQueryCUDA = true;
		QueryCUDA();
	}

// 	LARGE_INTEGER litmp;
// 	LONGLONG p1, p2;
// 	double dFreq, dTime, t[20];
// 	QueryPerformanceFrequency(&litmp);
// 	dFreq = (double)litmp.QuadPart;

	// 加载configuration配置文件	
	ConfigurationManage configManage;
    if (!configManage.LoadConfigurationFile(configName)) {	
		return LOAD_CONFIG_FAIL;
	}
	
	unsigned char grayThreshold;

    if (configManage.m_thresholdType == MANUAL)
	{
		grayThreshold = configManage.m_manualThreshold;
	}
	else // TODO: 建立Threshold Map
	{
		grayThreshold = 50;
	}

	//----------------------------------- step 1: 配准、相减	
	memcpy(matDstImg.ptr<uchar>(0), pbyDstImgData, IMAGE_HEIGHT * IMAGE_WIDTH);
	//imshow("matDstImg Dll", matDstImg); waitKey(1);
	if (matDstImg.empty())
	{
		AfxMessageBox("dstImg.empty() Fail!");
		return false;
	}

#if 0
	// site region的有效性判断，限制其在有效范围内
	if (siteRect.width <= 0 || siteRect.height <= 0 || siteRect.width > matDstImg.cols
		|| siteRect.height > matDstImg.rows)
	{
		siteRect.x = 100;
		siteRect.y = 100;
		siteRect.width = matDstImg.cols - siteRect.x*2;
		siteRect.height = matDstImg.rows - siteRect.y*2;
	}
#endif
	// 根据新规则限制site region在有效范围内
	if (urectSite.x + urectSite.width <= 0 || urectSite.y + urectSite.height <= 0
	 || urectSite.x >= matDstImg.cols - 1 || urectSite.y >= matDstImg.rows - 1)
	{
		urectSite.x = 0;
		urectSite.y = 0;
		urectSite.width = matDstImg.cols;
		urectSite.height = matDstImg.rows;
	}
	else 
	{
		if (urectSite.x < 0 && urectSite.x + urectSite.width > 0)
		{
			urectSite.x = 0;
		}

		if (urectSite.y < 0 && urectSite.y + urectSite.height > 0)
		{
			urectSite.y = 0;
		}

		if (urectSite.x + urectSite.width > matDstImg.cols)
		{
			urectSite.width -= urectSite.x + urectSite.width - matDstImg.cols;
		}

		if (urectSite.y + urectSite.height > matDstImg.rows)
		{
			urectSite.height -= urectSite.y + urectSite.height - matDstImg.rows;
		}

	}


	cv::Rect cvrectSite;
	cvrectSite.x = urectSite.x;
	cvrectSite.y = urectSite.y;
	cvrectSite.width = urectSite.width;
	cvrectSite.height = urectSite.height;

	DefectDetection defectDetection;
#if 0
	// 创建golden die
	Mat matGoldenImg;
    
	if (!defectDetection.CreateGoldenImage(goldenImgPath, matGoldenImg))
	{
		AfxMessageBox("defectDetection.CreateGoldenImage Fail!");
		return false;
	}
    
#else

    memcpy(matGoldenImg.ptr<uchar>(0), pbyGoldenImgData, IMAGE_HEIGHT * IMAGE_WIDTH); 
#endif

// 	QueryPerformanceCounter(&litmp);
// 	p1 = litmp.QuadPart;
	//imshow("matGoldenImg Dll", matGoldenImg);waitKey(1);

	Mat maskImg;
	if (!defectDetection.CreateGoldenMaskImage(matGoldenImg, maskImg))
	{
		return false;
	}

	bool bNoRoiExist = true;

    // 从字符串读取
    //std::string json_val;
    // 设置json_val的内容
    //......
    //slothjson::::fxxx_gfw_t obj_val;
	ROI_t roiRegion;
    ROI_t roiNegRegion;
    //bool rc = slothjson::decode(json_val, obj_val);

    // 从文件读取
    //std::string path = "fxxx_gfw_t.json";
    //slothjson::fxxx_gfw_t obj_val;
    //bool rc = slothjson::load(path, obj_val);
	//JsonCppRead();
	RoiSettingJsonToStruct(szROIRegion, roiRegion);

    RoiSettingJsonToStruct(szROINegRegion, roiNegRegion);

	Mat roiImg;
	vector<Rect> boundPolyRect;
	vector<Rect> boundCircleRect;



	if ((roiRegion.polyNum > 0) || (roiRegion.circleNum > 0))
	{
		bNoRoiExist = false;
		roiImg = Mat::zeros(matDstImg.size(), matDstImg.type());

        if (roiRegion.polyNum > 0)
		{			
			for(int i=0; i<roiRegion.polyNum; i++)
			{
				int pt_num = roiRegion.polyArray[i].vertexNum;
				
				Point* poly_points = new Point[pt_num];
				vector<Point> contours_poly;
				contours_poly.clear();
			
				for(int k=0; k<pt_num; k++)
				{
					Point pt;
					pt.x = roiRegion.polyArray[i].vertexArray[k].x;
					pt.y = roiRegion.polyArray[i].vertexArray[k].y;

					poly_points[k] = pt;
					contours_poly.push_back(pt);
				}
				const Point* ppt[1] = { poly_points };
  				int npt[] = { pt_num };
				fillPoly( roiImg, ppt,  npt, 1, Scalar( 255), LINE_8 );
				delete [] poly_points;

				Rect bound;
				bound = boundingRect( Mat(contours_poly) );
				boundPolyRect.push_back(bound);
				
			}
		}

		if (roiRegion.circleNum > 0)
		{
			for(int i=0; i<roiRegion.circleNum; i++)
			{
				Point center;
				center.x = roiRegion.circleArray[i].center.x;
				center.y = roiRegion.circleArray[i].center.y;
				float radius;
				radius = roiRegion.circleArray[i].radius;
				circle(roiImg, center, radius, Scalar(255), FILLED, LINE_8);

				Rect bound;
				bound.x = center.x - radius;
				bound.y = center.y - radius;
				bound.width = radius*2;
				bound.height = radius*2;
				boundCircleRect.push_back(bound);
			}
		}
	}

    Mat matNegRoiImg;
	vector<Rect> vecrectBoundPoly;
	vector<Rect> vecrectBoundCircle;

	if ((roiNegRegion.polyNum > 0) || (roiNegRegion.circleNum > 0))
	{
		bNoRoiExist = false;
		matNegRoiImg = Mat::zeros(matDstImg.size(), matDstImg.type());

        if (roiNegRegion.polyNum > 0)
		{
			//vector<vector<Point> > contours_poly( roiNegRegion.polyNum );
			
			for (int i = 0; i<roiNegRegion.polyNum; i++)
			{
				int pt_num = roiNegRegion.polyArray[i].vertexNum;
				
				//char buf[128];
				//sprintf(buf, "vertexNum: %d, polynum:%d\n", pt_num, roiRegion.polyNum);
				//AfxMessageBox(buf);
				//fprintf(stderr, "vertexNum %d \n", roiRegion.polyArray[i].vertexNum);

				Point* poly_points = new Point[pt_num];
                vector<Point> contours_poly;
				contours_poly.clear();
			
				for(int k=0; k<pt_num; k++)
				{
					Point pt;
					pt.x = roiNegRegion.polyArray[i].vertexArray[k].x;
					pt.y = roiNegRegion.polyArray[i].vertexArray[k].y;

					poly_points[k] = pt;
					contours_poly.push_back(pt);
				}
				const Point* ppt[1] = { poly_points };
  				int npt[] = { pt_num };
				fillPoly(matNegRoiImg, ppt,  npt, 1, Scalar( 255), LINE_8 );
#ifdef DEBUG1                
                imshow("matNegRoiImg", matNegRoiImg);waitKey(1);
#endif
				delete [] poly_points;

				Rect rectBound;
				rectBound = boundingRect(Mat(contours_poly));
				vecrectBoundPoly.push_back(rectBound);
				
			}
		}

		if (roiNegRegion.circleNum > 0)
		{
			for (int i = 0; i < roiNegRegion.circleNum; i++)
			{
				Point center;
				center.x = roiNegRegion.circleArray[i].center.x;
				center.y = roiNegRegion.circleArray[i].center.y;
				float radius;
				radius = roiNegRegion.circleArray[i].radius;
				circle(matNegRoiImg, center, radius, Scalar(255), FILLED, LINE_8);

				//char buf[128];
				//sprintf(buf, "radius: %4.2f, x:%d,  y:%d;  polynum:%d\n", radius, center.x, center.y, roiRegion.circleNum);
				//AfxMessageBox(buf);

				Rect rectBound;
				rectBound.x = center.x - radius;
				rectBound.y = center.y - radius;
				rectBound.width = radius*2;
				rectBound.height = radius*2;
				vecrectBoundCircle.push_back(rectBound);
			}
		}
	}
	Mat diffMatFg, diffMatBg, diffMat;
    gHaveQueryCUDA = false;
	Mat defectRegionsFg, defectRegionsBg, defectRegionsRoi;

    if (!defectDetection.Detection(matDstImg, matGoldenImg, maskImg, cvrectSite, grayThreshold, diffMatFg, diffMatBg,
		diffMat, bWithRotate, matchtype))
	{
		return DETECTION_FAIL;
	}
	//imshow("matDstImg", matDstImg);imshow("matGoldenImg", matGoldenImg);imshow("defectRegions", defectRegions);waitKey(1);

	bool bDefectFound = false;

	if(bNoRoiExist)
	{
		threshold(diffMatFg, defectRegionsFg, grayThreshold, 255, THRESH_BINARY);

		bool ret;
		ret =  RemoveIrrelevantRegions(defectRegionsFg, configManage);
		if(ret)	bDefectFound = true;

		/*	不检测背景区域
		Mat m0;
		threshold(diffMatBg, defectRegionsBg, 80, 255, THRESH_BINARY);
		threshold(matDstImg, m0, 180, 255, THRESH_BINARY);		//180是创建mask的阈值，只把背景中的高亮部分检测为缺陷
		defectRegionsBg = defectRegionsBg & m0;

		ret = RemoveIrrelevantRegions(defectRegionsBg, configManage);
		if(ret)	bDefectFound = true;
		*/
	}
	else
	{
		Mat noRoiImg = ~roiImg;
		noRoiImg &= matNegRoiImg;

		threshold(diffMatFg, defectRegionsFg, grayThreshold, 255, THRESH_BINARY);	
		defectRegionsFg &= noRoiImg;
		
		bool ret;
		ret =  RemoveIrrelevantRegions(defectRegionsFg, configManage);
		if(ret)	bDefectFound = true;

		/*
		Mat m0;
		threshold(diffMatBg, defectRegionsBg, 80, 255, THRESH_BINARY);
		threshold(matDstImg, m0, 180, 255, THRESH_BINARY);		//180是创建mask的阈值，只把背景中的高亮部分检测为缺陷
		defectRegionsBg = defectRegionsBg & m0;
		defectRegionsBg &= noRoiImg;

		ret = RemoveIrrelevantRegions(defectRegionsBg, configManage);
		if(ret)	bDefectFound = true;	//暂时关闭背景区域检测
		*/
		Mat matDiff;
        diffMat.copyTo(matDiff);

		ConfigurationManage config;
		diffMat = diffMat&roiImg;
		defectRegionsRoi = Mat::zeros(diffMat.size(), diffMat.type());
		//imshow("roiImg", roiImg); imshow("diffMat", diffMat); waitKey(1);
		if(roiRegion.polyNum>0)
		{
			for(int i=0; i<roiRegion.polyNum; i++)
			{
				Rect bound = boundPolyRect[i];
				//config.m_manualThreshold = roiRegion.polyArray[i].param.manualThreshold;
				config.m_MinArea = roiRegion.polyArray[i].param.minArea;
				config.m_MinSize = roiRegion.polyArray[i].param.minSize;

				Mat roiMat = diffMat(bound);
				Mat roiRegion = defectRegionsRoi(bound); //imshow("roiMat", roiMat);
				threshold(roiMat, roiRegion, config.m_manualThreshold, 255, THRESH_BINARY);  //imshow("roiRegion", roiRegion); imshow("defectRegionsRoi", defectRegionsRoi); waitKey(1);
				ret =  RemoveIrrelevantRegions(roiRegion, config); 
				if (ret)	bDefectFound = true;
			}
		}
		
		if(roiRegion.circleNum>0)
		{
			for(int i=0; i<roiRegion.circleNum; i++)
			{
				Rect bound = boundCircleRect[i];
				//config.m_manualThreshold = roiRegion.circleArray[i].param.manualThreshold;
				config.m_MinArea = roiRegion.circleArray[i].param.minArea;
				config.m_MinSize = roiRegion.circleArray[i].param.minSize;

				Mat roiMat = diffMat(bound);
				Mat roiRegion = defectRegionsRoi(bound);
				threshold(roiMat, roiRegion, config.m_manualThreshold, 255, THRESH_BINARY);
				ret =  RemoveIrrelevantRegions(roiRegion, config);
				if(ret)	bDefectFound = true;
			}
		}		
		Mat matSite = Mat::zeros(matDstImg.size(), matDstImg.type());
        rectangle(matSite, Rect(urectSite.x, urectSite.y, urectSite.width, urectSite.height), Scalar(255), - 1); //imshow("matSite", matSite);
        matSite &= noRoiImg; //imshow("matSite &= noRoiImg", matSite);
		Rect rectBound = Rect(urectSite.x, urectSite.y, urectSite.width, urectSite.height);
        ConfigurationManage configSite;
    	configSite.m_manualThreshold = urectSite.manualThreshold;
    	configSite.m_MinSize = urectSite.minSize;
    	configSite.m_MinArea = urectSite.minArea;

		Mat matDiffBound = matDiff(rectBound);
		Mat matDiffDst = matSite(rectBound); //imshow("matDiffDst", matDiffDst);
		threshold(matDiffBound, matDiffDst, configSite.m_manualThreshold, 255, THRESH_BINARY);  //imshow("matDiffBound", matDiffBound); imshow("matDiffDst", matDiffDst); waitKey(1);
		ret =  RemoveIrrelevantRegions(matDiffDst, configSite); //imshow("matDiffDst After RemoveIrrelevantRegions", matDiffDst);
		if (ret)	bDefectFound = true;
	}


	if(!bDefectFound)
		return REMOVEIRRELEVANTREGIONS_FAIL;
	//----------------------------------- step 2: 根据配置文件参数去除无关区域

#if SAVE_RESULT_TO_FILE
        // 保存图像结果到文件
    	string strSavePath = outputImgPath;
    	strSavePath += "\\";
    	strSavePath += szDieID;
        strSavePath += szSiteID;
    	string strImgSavePath = strSavePath;
    	string strJsonSavePath = strSavePath;
        strImgSavePath += ".bmp";
        strJsonSavePath += ".json";
#endif
	//----------------------------------- step 3: 缺陷分类	
	DefectsClassify defectClassify;
	bool bVerifyFg, bVerifyBg, bVerifyRoi;
	Mat resultMatFg, resultMatBg, resultMatRoi;
	std::vector<defectDescription<float> > outputFg, outputBg, outputRoi;
	
	bVerifyFg = defectClassify.DoClassify(defectRegionsFg, matDstImg, pixelSizeRatio,
		configManage.m_strategyName, resultMatFg, outputFg);
//	bVerifyBg = defectClassify.DoClassify(defectRegionsBg, matDstImg, pixelSizeRatio,
//		configManage.m_strategyName, resultMatBg, outputBg);
	bVerifyBg = false;	//暂时关闭背景区域检测

	if(false==bNoRoiExist)
	{
		bVerifyRoi = defectClassify.DoClassify(defectRegionsRoi, matDstImg, pixelSizeRatio,
		configManage.m_strategyName, resultMatRoi, outputRoi);
	}
	else
		bVerifyRoi = false;
	
	if ((!bVerifyFg) && (!bVerifyBg) && (!bVerifyRoi))
	{
		//std::cout << "DefectsClassify::DoClassify() fail!" << std::endl;
		return DOCLASSIFY_FAIL;
	}


// 	QueryPerformanceCounter(&litmp);
// 	p2 = litmp.QuadPart;
// 	t[0] = (double)(p2 - p1) / dFreq;
// 	p1 = p2;
// 	cout << "DoClassify cost time : " << t[0] << endl;

	
	if(bNoRoiExist)
		resultMat = resultMatFg | resultMatBg;
	else
		resultMat = resultMatFg | resultMatBg | resultMatRoi;
	
	outputResult.clear();
	if(outputFg.size() > 0)
	{
		int size = outputFg.size();
		for(int i=0; i<size; i++)
			outputResult.push_back(outputFg[i]);
	}
/*	if(outputBg.size() > 0)	//暂时关闭背景区域检测
	{
		int size = outputBg.size();
		for(int i=0; i<size; i++)
			outputResult.push_back(outputBg[i]);
	}*/
	if(outputRoi.size() > 0)
	{
		int size = outputRoi.size();
		for(int i=0; i<size; i++)
			outputResult.push_back(outputRoi[i]);
	}
    
#if SAVE_RESULT_TO_FILE
    imwrite(strImgSavePath.c_str(), resultMat);
#endif

	//----------------------------------- step 4: 缺陷聚类
	// 还有问题，先封上
#if 0
	if (configManage.m_bEnableClustering)
	{		
		DefectsClustering cluster;
		string FileName = "";
		if (configManage.m_clusterType != CLUSTER_NONE)
		{
			FileName = (configManage.m_clusterType == CLUSTER_ROI)
				? configManage.m_ROIMaskName : configManage.m_clusterStrategyName;
		}
		cluster.DoClustering(configManage.m_clusterType, FileName,
			configManage.m_clusterRadius, pixelSizeRatio, defectRegions,
			outputResult);		
	}
#endif

	//----------------------------------- step 5: 将结果写入json文件
#if SAVE_RESULT_TO_FILE
	DefectDescriptionStructToJsonFile(outputResult, strJsonSavePath);
#endif

// 	QueryPerformanceCounter(&litmp);
// 	p2 = litmp.QuadPart;
// 	t[0] = (double)(p2 - p1) / dFreq;
// 	p1 = p2;
// 	cout << "save cost time : " << t[0] << endl;

    return ERROR_NONE;
}

extern "C"  __declspec(dllexport)
bool 
CreateGoldenImage(
    char* szGoldenImgPath, 
    unsigned char* pbyGoldenImg,
    unsigned char* pbyBaseImg,
	AlignmentRange sAlignmentRange)
{  
	// 创建golden die
	Mat matGoldenImg(IMAGE_HEIGHT, IMAGE_WIDTH, CV_8UC1, Scalar(0));
    Mat matBaseImg(IMAGE_HEIGHT, IMAGE_WIDTH, CV_8UC1, Scalar(0));

    memcpy(matBaseImg.ptr<uchar>(0), pbyBaseImg, IMAGE_HEIGHT * IMAGE_WIDTH); 
    //imshow("matBaseImg0", matBaseImg);

	DefectDetection defectDetection;

    //imshow("matBaseImg0", matBaseImg);
	if (!defectDetection.CreateGoldenImage(szGoldenImgPath, matGoldenImg, matBaseImg, sAlignmentRange))
	{
		AfxMessageBox("defectDetection.CreateGoldenImage Fail!");
		return false;
	}

    //memcpy(pbyGoldenImg, matGoldenImg.ptr<uchar>(0), IMAGE_HEIGHT * IMAGE_WIDTH);
	for (int i = 0; i < IMAGE_HEIGHT; i++) 
	{
		for (int j = 0; j < IMAGE_WIDTH; j++) 
		{
			*(pbyGoldenImg + i * IMAGE_WIDTH + j) = matGoldenImg.ptr<uchar>(i, j)[0];
		}
	}
    //imwrite("GoldenImg.bmp", matGoldenImg);
    
	return true;
}

bool CallDoAnalysis(
	unsigned char* pbyDstImgData,
	unsigned char* pbyGoldenImgData,
	char*  configName,
	URect siteRect,
	double pixelSizeRatio,
	char* jsonResult,
	char* outputImgPath,
	char* szDieID,
	char* szSiteID)
{
/*	DoAnalysis(
		pbyDstImgData,
		pbyGoldenImgData,
		configName,
		siteRect,
		pixelSizeRatio,
		jsonResult,
		outputImgPath,
		szDieID,
		szSiteID);
*/
	return true;
}

bool OneBatchProc(vector<string> &imgNameVec, 
	const std::string& dstImgPath,
	const Mat& goldenImg, 
	unsigned char grayThreshold, 
	const cv::Rect& siteRect, 
	bool bParallel, 
	bool bWithRotate, 	
	ConfigurationManage &configManage, 
	double pixelSizeRatio, 
	cv::Mat& resultMat, 
	std::vector<defectDescription<float> >& outputResult,
	int& nDefectRegionSize)
{
	//----------------------------------- step 1: 配准、相减	
	DefectDetection defectDetection;
	std::vector<cv::Mat> defectRegionsVec;
    
	if (!defectDetection.Detection(imgNameVec, dstImgPath, goldenImg, grayThreshold,
		siteRect, defectRegionsVec, bParallel, bWithRotate))
	{
		return false;
	}

	cout << "defectRegionsVec.size = " << defectRegionsVec.size() << endl;

	std::vector<cv::Mat>::iterator defectRegionsVecIt = defectRegionsVec.begin();
	std::vector<string>::iterator imgNameVecIt = imgNameVec.begin();

	for (; 
        defectRegionsVecIt != defectRegionsVec.end(); 
        ++defectRegionsVecIt, ++imgNameVecIt)
	{
		//----------------------------------- step 2: 根据配置文件参数去除无关区域
		if (!RemoveIrrelevantRegions(*defectRegionsVecIt, configManage))
		{
			return false;
		}

		//----------------------------------- step 3: 缺陷分类	
		DefectsClassify defectClassify;
		string imgPath = dstImgPath + *imgNameVecIt;
		Mat dstImg = imread(imgPath.c_str(), CV_LOAD_IMAGE_GRAYSCALE);

		if (!defectClassify.DoClassify(*defectRegionsVecIt, dstImg, pixelSizeRatio,
			configManage.m_strategyName, resultMat, outputResult))
		{
			std::cout << "DefectsClassify::DoClassify() fail!" << std::endl;
			return false;
		}

		//----------------------------------- step 4: 缺陷聚类
		//TODO

		//----------------------------------- step 5: 将结果写入json文件	
#if SAVE_RESULT_TO_FILE
		if (!PathIsDirectory(RESULT_SAVE_PATH.c_str()))
		{
			CreateDirectory(RESULT_SAVE_PATH.c_str(), NULL);
		}
		char tmpStr[1000];
		int nSize = imgNameVecIt->length();
		std::string strFileName;

		//strncpy(tmpStr, imgNameVecIt->c_str(), nSize - 4);

		strFileName = imgNameVecIt->c_str();
		strFileName = strFileName.substr(0, nSize - 4);
		//sprintf(tmpStr, "%s%s%s", szPathName, "Site", strSiteName.c_str());

		//string imgFileName(tmpStr);
		string imgFileName(strFileName);
		imgFileName += ".png";

		string imgSavePath = RESULT_SAVE_PATH + imgFileName;

		//cv::rectangle(resultMat, siteRect, Scalar(0, 255, 255), 2);
		imwrite(imgSavePath, resultMat);

		// tmp
// 		string tmpSrcPath = RESULT_SAVE_PATH + *imgNameVecIt;
// 		imwrite(tmpSrcPath, dstImg);

		
		string fileName = "defectsData" + string(tmpStr) + ".json";
		string savePath = RESULT_SAVE_PATH + fileName;
		DefectDescriptionStructToJsonFile(outputResult, savePath);
#endif
	}

	nDefectRegionSize = defectRegionsVec.size();
	return true;
}

// ------------------------------------------------------------
// Description : 缺陷“检测/分类/聚类”对外接口,批处理接口
// Parameters :  dstImgPath：待检图像文件夹路径
//               goldenImgPath：金标准图像文件夹路径
//			     configName: 使用的配置文件名称
//				 siteRect: site的检测区域	
//				 pixelSizeRatio: 每像素对应的物理尺寸
//				 jsonResult: 【输出值】json格式数值结果的字符串
//				 nImgNum: 【输出值】处理图像数量
//				 outputResult: 【输出值】stl容器保存的结果
//				 resultMat: 【输出值】结果图像
// Return Value :true - 成功
//			     false - 失败
// ------------------------------------------------------------
bool DoBatchAnalysis(const std::string& dstImgPath,
	const std::string& goldenImgPath,
	const std::string& configName,
	const cv::Rect& siteRect,
	double pixelSizeRatio,
	std::string& jsonResult,
	std::string& outputImgPath,
	int& nImgNum,
	std::vector<defectDescription<float> >& outputResult,	// 给软件的时候删掉
	cv::Mat& resultMat,										// 给软件的时候删掉
	bool bParallel,											// 给软件的时候删掉			
	bool bWithRotate)										// 给软件的时候删掉			
{
	long startTime = GetTickCount();

	// 加载configuration配置文件	
	ConfigurationManage configManage;

    if (!configManage.LoadConfigurationFile(configName))
	{
		return false;
	}

	long endTime = GetTickCount();
	cout << "*********** load configuration file cost time : " << endTime - startTime << " ms" << endl;
	startTime = endTime;

	unsigned char grayThreshold;

    if (configManage.m_thresholdType == MANUAL)
	{
		grayThreshold = configManage.m_manualThreshold;
	}
	else // TODO: 建立Threshold Map
	{
		grayThreshold = 50;
	}

	// 创建golden die
	Mat goldenImg;
	Mat matBaseImg;
	DefectDetection defectDetection;

	AlignmentRange sAlignmentRange;
	sAlignmentRange.iMarginH = 128;
	sAlignmentRange.iMarginV = 128;

    
	if (!defectDetection.CreateGoldenImage(goldenImgPath, goldenImg, matBaseImg, sAlignmentRange))
	{
        
		return false;
	}

    //imshow("GoldenImg", goldenImg);
    imwrite("GoldenImg.bmp", goldenImg);

	vector<string> imgNameVec;
	GetImgNameFromDir(dstImgPath, imgNameVec);

	int nImgNumToBeProcPerLoop = 50;
	vector<string>::iterator imgNameIt = imgNameVec.begin();
	int nLoopNum = imgNameVec.size() / nImgNumToBeProcPerLoop;

	if (0 == nLoopNum)
	{
		nLoopNum = 1;
		nImgNumToBeProcPerLoop = imgNameVec.size();
	}

	cout << "imgNameVec.size() = " << imgNameVec.size() << endl;
	cout << "nLoopNum = " << nLoopNum << endl;

	nImgNum = 0;
    
	for (int i = 0; i < nLoopNum && imgNameIt != imgNameVec.end(); ++i)
	{
		int nDefectRegionSize;
		int nStep = (i != nLoopNum - 1)
			? nImgNumToBeProcPerLoop 
			: (imgNameVec.size() % nImgNumToBeProcPerLoop);
		
		vector<string> imgNameTobeProc(imgNameIt, imgNameIt + nStep);
		imgNameIt += nImgNumToBeProcPerLoop + 1;

		cout << "loop " << i << "times." << endl;
		
		OneBatchProc(imgNameTobeProc, dstImgPath, goldenImg, grayThreshold, siteRect,
			bParallel, bWithRotate, configManage, pixelSizeRatio, resultMat,
			outputResult, nDefectRegionSize);
		nImgNum += nDefectRegionSize;
	}

	endTime = GetTickCount();
	cout << "*********** filtering defect regions and classify defect cost time : " << endTime - startTime << " ms" << endl;
	startTime = endTime;

	cout << "image number: " << nImgNum << endl;
    cout << "\n\n" << endl;

	return true;
}

extern "C"  __declspec(dllexport)
bool DoBatchAnalysis(char* dstImgPath,
	char* goldenImgPath,
	char* configName,
	URect siteRect,
	double pixelSizeRatio,
	char* jsonResult,
	char* outputImgPath,
	int& nImgNum,
	AlignmentRange sAlignmentRange)
{
	bool bParallel = true;
	bool bWithRotate = true;
    std::vector<defectDescription<float> > outputResult;
	Mat resultMat;

#if 1
	char szTemp[255];
	sprintf(szTemp, "(%f, %f, %f, %f)", siteRect.x, siteRect.y, siteRect.width, siteRect.height);
	AfxMessageBox((LPCTSTR)szTemp, 0);
#endif

	long startTime = GetTickCount();

	// 加载configuration配置文件	
	ConfigurationManage configManage;

    if (!configManage.LoadConfigurationFile(configName))
	{
		return false;
	}

	long endTime = GetTickCount();
	cout << "*********** load configuration file cost time : " << endTime - startTime << " ms" << endl;
	startTime = endTime;

	unsigned char grayThreshold;

    if (configManage.m_thresholdType == MANUAL)
	{
		grayThreshold = configManage.m_manualThreshold;
	}
	else // TODO: 建立Threshold Map
	{
		grayThreshold = 50;
	}

	cv::Rect cvrectSite;
	cvrectSite.x = siteRect.x;
	cvrectSite.y = siteRect.y;
	cvrectSite.width = siteRect.width;
	cvrectSite.height = siteRect.height;

	// 创建golden die
	Mat goldenImg;
	Mat matBaseImg;
	DefectDetection defectDetection;
    
	if (!defectDetection.CreateGoldenImage(goldenImgPath, goldenImg, matBaseImg, sAlignmentRange))
	{
        
		return false;
	}

    //imshow("GoldenImg", goldenImg);
    imwrite("GoldenImg.bmp", goldenImg);

	vector<string> imgNameVec;
	GetImgNameFromDir(dstImgPath, imgNameVec);

	int nImgNumToBeProcPerLoop = 50;
	vector<string>::iterator imgNameIt = imgNameVec.begin();
	int nLoopNum = imgNameVec.size() / nImgNumToBeProcPerLoop;

	if (0 == nLoopNum)
	{
		nLoopNum = 1;
		nImgNumToBeProcPerLoop = imgNameVec.size();
	}

	cout << "imgNameVec.size() = " << imgNameVec.size() << endl;
	cout << "nLoopNum = " << nLoopNum << endl;

	nImgNum = 0;
    
	for (int i = 0; i < nLoopNum && imgNameIt != imgNameVec.end(); ++i)
	{
		int nDefectRegionSize;
		int nStep = (i != nLoopNum - 1)
			? nImgNumToBeProcPerLoop 
			: (imgNameVec.size() % nImgNumToBeProcPerLoop);
		
		vector<string> imgNameTobeProc(imgNameIt, imgNameIt + nStep);
		imgNameIt += nImgNumToBeProcPerLoop + 1;

		cout << "loop " << i << "times." << endl;
		
		OneBatchProc(imgNameTobeProc, dstImgPath, goldenImg, grayThreshold, cvrectSite,
			bParallel, bWithRotate, configManage, pixelSizeRatio, resultMat,
			outputResult, nDefectRegionSize);
		nImgNum += nDefectRegionSize;
	}

	endTime = GetTickCount();
	cout << "*********** filtering defect regions and classify defect cost time : " << endTime - startTime << " ms" << endl;
	startTime = endTime;

	cout << "image number: " << nImgNum << endl;
    cout << "\n\n" << endl;

	return true;
}

