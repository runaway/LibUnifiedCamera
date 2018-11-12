#include "stdafx.h"
#include <shlwapi.h>
#include <sstream>

#include "DataConvert.h"
#include "document.h"
#include "DefectsClassify.h"
#include "AlgoGoldenAnalysis.h"
#include "DefectsClustering.h"

using namespace rapidjson;

#pragma comment(lib, "shlwapi.lib")

bool  AlgoGoldenAnalysis::bCreateGoldenImageByMedianFilter = true;

// ------------------------------------------------------------
// Description : 创建金标准图片
// Parameters :  strFilesPath 存放金标准图片的文件路径
//				 unsigned char* baseImage 金标准图片的基准图片
//				 unsigned char* goldenImage 返回生成的金标准图片
//				 offX: match水平偏移范围[-offX, offX]
//				 offY: match垂直偏移范围[-offX, offX]
//				 jsonRegion: 用于对正的ROI区域
// Return Value :true - 设置成功
//				 false - 设置失败
// ------------------------------------------------------------
bool AlgoGoldenAnalysis::CreateGoldenImage(const string& strFilesPath, unsigned char* goldenImageData, unsigned char* baseImageData, int offX, int offY, const char* jsonRegion)
{
	vector<Rect> regions;
	regions.clear();
	ExtractRegionJson(jsonRegion, regions);
	int regionNum = regions.size();
	
	if(regionNum > 0)
	{
		for(int i=0; i<regionNum; i++)
		{
			if((regions[i].width <= offX*2) || (regions[i].height <= offY*2))
			{
				cout << "ERROR: golden pattern region too small " << endl;
				return false;
			}
		}
	}
	//--------------------------

	if(!goldenImageData)
	{
		cout << "ERROR: 返回golden图像数据指针不能为空 " << endl;
		return false;
	}

	if (!PathIsDirectory(strFilesPath.c_str()))
	{
		cout << "ERROR: golden path 错误 " << endl;
		return false;
	}
	
	vector<string> vImageFileNames;
	GetImgNameFromDir(strFilesPath, vImageFileNames);   //获取目录下所有文件名

	int fileNum = vImageFileNames.size();
	
	if(1==fileNum)
	{//如果目录下只有一个文件，直接用该文件作为golden image
		string fname = strFilesPath + vImageFileNames[0];
		Mat img = imread(fname.c_str(), CV_LOAD_IMAGE_GRAYSCALE);
		Mat gold = Mat(img.size(), CV_8UC1, goldenImageData);
		img.copyTo(gold);
		return true;
	}
	
	if(fileNum == 0)
	{
		cout << "ERROR: golden path 文件数目不足 " << endl;	//用于生成golden图片的目录下至少有一个图像文件
		return false;	
	}

	vector<Mat> vCandiImages;
	vCandiImages.clear();
	for(int i=0; i<fileNum; i++)
	{
		string fname = strFilesPath + vImageFileNames[i];
		Mat img = imread(fname.c_str(), CV_LOAD_IMAGE_GRAYSCALE);
		vCandiImages.push_back(img);
	}

	Mat baseImage, goldenImage;

	if (baseImageData)
	{
		baseImage = Mat(vCandiImages[0].size(), CV_8UC1, baseImageData);
		vCandiImages.insert(vCandiImages.begin(), baseImage);	//baseImage放在第一个位置
	}
	else
	{//没有baseImageData输入，用目录下的第一个文件作为baseImage
		baseImage = vCandiImages[0];
	}
	goldenImage = Mat(baseImage.size(), CV_8UC1, goldenImageData);

	if(0==regionNum)
	{//没有设置对正区间，用整幅图像对正
		Rect region;
		region.x = 0;
		region.y = 0;
		region.width = baseImage.cols;
		region.height = baseImage.rows;
		
		regions.push_back(region);
		regionNum = 1;
	}

	cout << "GoldenImage aligning: " << endl;
	vector<Point> vImageOffset;	
	vImageOffset.clear();
    float fScore = 0.0f;
    
	for(int i=1; i<vCandiImages.size(); i++)
	{
		float sx, sy, score;
		sx = sy = 0;
        fScore = 0;
		for(int j=0; j<regionNum; j++)
		{
			Mat base, align;
			base = baseImage(regions[j]);
			align = vCandiImages[i](regions[j]);
			
			Point off = CalcAlignOffset_ocv_cpu(base, align, offX, offY, score);
			cout << "  Image " << i << ", region " << j << ":  off_x=" <<off.x <<", off_y=" << off.y << endl;

			sx += off.x;
			sy += off.y;
            fScore += score;
		}
		cout << endl;

		sx = sx / regionNum;
		sy = sy / regionNum;
        fScore = fScore / regionNum;

		Point shift;
		shift.x = (int)sx;
		shift.y = (int)sy;
		vImageOffset.push_back(shift);
	}

	int imageWidth, imageHeight;
	imageWidth = baseImage.cols;
	imageHeight = baseImage.rows;
	
	for(int i=1; i<vCandiImages.size(); i++)
	{
		Mat img = vCandiImages[i].clone();
		Point off = vImageOffset[i-1];

		int dx, dy;
		dx = off.x;
		dy = off.y;

		Rect  srcRect, dstRect;
        //计算当前图片与base图片的公共区域
		if(dx >= 0)
		{
			srcRect.x = dx;			
			dstRect.x = 0;			
		}
		else
		{
			srcRect.x = 0;
			dstRect.x = -dx;		
		}
		srcRect.width = imageWidth - abs(dx);
		dstRect.width = srcRect.width;

		if(dy >= 0)
		{
			srcRect.y = dy;
			dstRect.y = 0;
		}
		else
		{
			srcRect.y = 0;
			dstRect.y = -dy;
		}
		srcRect.height = imageHeight - abs(dy);
		dstRect.height = srcRect.height;
        //拷贝当前图片的数据至对应位置
		img(srcRect).copyTo(vCandiImages[i](dstRect));
	}

	if(bCreateGoldenImageByMedianFilter)
	{
		CreateGoldenImageByMedianFilter(vCandiImages, goldenImage);
	}
	else 
	{
		CreateGoldenImageByAverageFilter(vCandiImages, goldenImage);
	}

	return true;
}
// ------------------------------------------------------------
// Description: 使用中值滤波创建金标准图片
// Parameters :  
//              vImages: 用于创建金标准图片的图片组
//				goldeImage: 返回的金准图片
// Return Value:无
// ------------------------------------------------------------
void AlgoGoldenAnalysis::CreateGoldenImageByMedianFilter(vector<Mat>& vImages, Mat& goldenImage)
{
    int num = vImages.size();
	int idm = num/2;
    int nRows = goldenImage.rows;
	int nCols = goldenImage.cols;
    vector<uchar> vPixels;

    for(int i = 0; i < nRows; i++)
    {
        for (int j = 0; j < nCols; j++)
        {
            vPixels.clear();
            
            for(int n=0; n<num; n++)           
                vPixels.push_back(vImages[n].ptr<uchar>(i)[j]);
            
            sort(vPixels.begin(), vPixels.end());

            goldenImage.ptr<uchar>(i)[j] = vPixels[idm];
        }
    }
}
// ------------------------------------------------------------
// Description: 使用均值滤波创建金标准图片
// Parameters :  
//               vImages:   用于创建金标准图片的图片组
//				 goldeImage: 返回的金准图片
// Return Value: 无
// ------------------------------------------------------------
void AlgoGoldenAnalysis::CreateGoldenImageByAverageFilter(vector<Mat>& vImages, Mat& goldeImage)
{
	Mat sum, cur;
	int num = vImages.size();
	
	vImages[0].convertTo(sum, CV_32FC1);
	
	for(int i=1; i<num; i++)
	{
		vImages[i].convertTo(cur, CV_32FC1);
		sum = sum + cur;
	}

	sum = sum / num;
	sum.convertTo(goldeImage, CV_8U);
}
//============================================================================================
AlgoGoldenAnalysis::AlgoGoldenAnalysis()
{
	m_bInited = false;
	m_bUseCuda = false;
	m_bSaveImage = false;

	m_AlignOffX = 64;	
	m_AlignOffY = 64;

	m_MatchMode = FASTEST;
	m_fMinAlignScore = 95.0f;

	m_nRingThres = 20;
	m_nRingWidth = 7;
}
AlgoGoldenAnalysis::~AlgoGoldenAnalysis()
{
	
}
// ------------------------------------------------------------
// Description : 算法初始化
// Parameters : 
//				fPixelSizeRatio: "像素-实际物理尺寸"的比例
//				configPath: 配置文件路径
//				outputPath: json数据和缺陷图片的保存路径；如果设置为NULL，则不保存json数据和图片
// Return Value :true - 初始化成功
//				 false - 获取初始化--------------------------------------
bool AlgoGoldenAnalysis::Init(int threadId, int imageWidth, int imageHeight, float fPixelSizeRatio, const char* configPath, const char* outputPath)
{
	// 加载configuration配置文件
	if (!m_configManage.LoadConfigurationFile(configPath))	
	{
		cout << "ERROR: (Init) 加载configuration配置文件出错 " << endl;
		return false;
	}

	m_configPath = configPath;
	m_outputPath = outputPath;

	m_fPixelSizeRatio = fPixelSizeRatio;

	m_iImageWidth = imageWidth;
	m_iImageHeight = imageHeight;

	for(int y=0; y<MAX_SITE_ROWS; y++)
		for(int x=0; x<MAX_SITE_COLS; x++)
		{
			m_bUseRoI[y][x] = false;

			m_SiteRect[y][x].x = 64;
			m_SiteRect[y][x].y = 64;
			m_SiteRect[y][x].width = imageWidth - 2*m_SiteRect[y][x].x;
			m_SiteRect[y][x].height = imageHeight - 2*m_SiteRect[y][x].y;	//设置缺省值

            m_AlignRegions[y][x].clear();
			m_AlignRegions[y][x].push_back(m_SiteRect[y][x]);   //缺省对正区域
		}

	m_MorphElement = getStructuringElement(MORPH_RECT, Size(1, 1), Point(-1, -1) );
	m_BufferMat = Mat::zeros(m_iImageHeight, m_iImageWidth, CV_8UC1);

	m_DiffFGImage = Mat::zeros(m_iImageHeight, m_iImageWidth, CV_8UC1);
	m_DiffRegions = Mat::zeros(m_iImageHeight, m_iImageWidth, CV_8UC1);

	m_ResultImage = Mat::zeros(m_iImageHeight, m_iImageWidth, CV_8UC3);
	
	m_HorzProjMat = Mat::zeros(1, m_iImageHeight, CV_32FC1);
	m_VertProjMat = Mat::zeros(1, m_iImageWidth,  CV_32FC1);
	m_ProdMat = Mat::zeros(1, max(m_iImageWidth, m_iImageHeight),  CV_32FC1);

	m_bInited = true;
	m_ThreadId = threadId;

	return true;
}
bool AlgoGoldenAnalysis::IsRectInRect(Rect& innerRect, Rect& outerRect)
{
	bool isIn = (innerRect.x >= outerRect.x) \
			 && (innerRect.y >= outerRect.y) \
			 && (innerRect.x+innerRect.width)<=(outerRect.x+outerRect.width) \
			 && (innerRect.y+innerRect.height)<=(outerRect.y+outerRect.height);

	return isIn;
}
// ------------------------------------------------------------
// Description:  设置site检测区域及检测参数
// Parameters :  
//               siteRect: 检测区域
//               param:  检测参数
//               thresh: 阈值集
//				 nSiteX: site 水平位置
//				 nSiteY: site 垂直位置
//
// Return Value: true - 设置成功
//				 false - 设置失败
// ------------------------------------------------------------
bool AlgoGoldenAnalysis::SetSiteRect(Rect_t siteRect, Param_t param, ThreshGroup_t thresh, int nSiteX, int nSiteY)
{
	if((nSiteX>=MAX_SITE_COLS) || (nSiteY>=MAX_SITE_ROWS))
	{
		cout << "ERROR: (SetSiteRect) site数超出最大值 " << endl;
		return false;
	}

	if(!m_bInited)
	{
		cout << "ERROR: unintied " << endl;
		return false;
	}
	if((siteRect.x < 0) || (siteRect.width < 0) || (siteRect.y < 0) || (siteRect.height < 0))
		return false;
	
	if((siteRect.x + siteRect.width) > m_iImageWidth)
		return false;

	if((siteRect.y + siteRect.height) > m_iImageHeight)
		return false;

	Rect rtSite;

	rtSite.x = siteRect.x;
	rtSite.y = siteRect.y;
	rtSite.width = siteRect.width;
	rtSite.height = siteRect.height;

	m_SiteRect[nSiteY][nSiteX] = rtSite;
	m_SiteParam[nSiteY][nSiteX] = param;
    m_siteThreshGroups[nSiteY][nSiteX] = thresh;

	return true;
}
// ------------------------------------------------------------
// Description:  根据goldenImage和threshGroup参数计算每个像素的阈值
// Parameters :  
//				 threshGroup: 阈值集
//               goldenImage: 金标准图像
//               highThreshMat:高于金标准图像的阈值
//               lowThreshMat:低于金标准图像的阈值
//
// Return Value: 无
//				 
// ------------------------------------------------------------
void AlgoGoldenAnalysis::CreateHighLowThreshMat(ThreshGroup_t& threshGroup, Mat& goldenImage, Mat& highThreshMat, Mat& lowThreshMat)
{
    int nRows = goldenImage.rows;
	int nCols = goldenImage.cols;

    highThreshMat = Mat::zeros(goldenImage.size(), CV_8UC1);
    lowThreshMat  = Mat::zeros(goldenImage.size(), CV_8UC1);

    int i, j;
    const uchar* pGold;
    uchar* pHigh, *pLow;
    
	if(threshGroup.highGray == threshGroup.lowGray)
    {//等同于明暗分区阈值分割
        for( i = 0; i < nRows; ++i)
    	{
    		pGold = goldenImage.ptr<uchar>(i);
            pHigh = highThreshMat.ptr<uchar>(i);
            pLow = lowThreshMat.ptr<uchar>(i);
    		
    		for ( j = 0; j < nCols; ++j)
    		{
    			uchar gold_pix = pGold[j];
                int high, low, above, below;

                if(gold_pix > threshGroup.highGray)
                {
                    above = threshGroup.highThreshAbove;
                    below = threshGroup.highThreshBelow;
                }
                else
                {
                    above = threshGroup.lowThreshAbove;
                    below = threshGroup.lowThreshBelow;
                }

                high = gold_pix + above;
                low  = gold_pix - below;
                
                if(high > 255)
                    high = 255;
                if(low < 0)
                    low = 0;
                
                pHigh[j] = (uchar)high;
                pLow[j]  = (uchar)low;
    		}
    	}
    }
    else
    {
        for( i = 0; i < nRows; ++i)
        {
            pGold = goldenImage.ptr<uchar>(i);
            pHigh = highThreshMat.ptr<uchar>(i);
            pLow = lowThreshMat.ptr<uchar>(i);
    		
    		for ( j = 0; j < nCols; ++j)
    		{
    			uchar gold_pix = pGold[j];
                int high, low, above, below;

                if(gold_pix >= threshGroup.highGray)
                {                    
                    high = gold_pix + threshGroup.highThreshAbove;
                    low  = gold_pix - threshGroup.highThreshBelow;
                    
                    if(high > 255)
                        high = 255;
                    if(low < 0)
                        low = 0;
                    
                    pHigh[j] = (uchar)high;
                    pLow[j]  = (uchar)low;
                }                
                else if (gold_pix <= threshGroup.lowGray)
                {                    
                    high = gold_pix + threshGroup.lowThreshAbove;
                    low  = gold_pix - threshGroup.lowThreshBelow;
                    
                    if(high > 255)
                        high = 255;
                    if(low < 0)
                        low = 0;
                    
                    pHigh[j] = (uchar)high;
                    pLow[j]  = (uchar)low;
                }
                else
                {
                    float kh, kl, dist;
                    kh = (float)(threshGroup.highThreshAbove-threshGroup.lowThreshAbove)/(threshGroup.highGray-threshGroup.lowGray);
                    kl = (float)(threshGroup.highThreshBelow-threshGroup.lowThreshBelow)/(threshGroup.highGray-threshGroup.lowGray);

                    dist = (float)(gold_pix - threshGroup.lowGray);

                    float highThresh, lowThresh;
                    highThresh = threshGroup.lowThreshAbove + kh*dist;
                    lowThresh  = threshGroup.lowThreshBelow + kl*dist;

                    high = gold_pix + (int)highThresh;
                    low  = gold_pix - (int)lowThresh;
                    
                    if(high > 255)
                        high = 255;
                    if(low < 0)
                        low = 0;
                    
                    pHigh[j] = (uchar)high;
                    pLow[j]  = (uchar)low;
                }
    		}
        }
    }
}
// ------------------------------------------------------------
// Description:  标记待检图像与goldenImage图像差异大于阈值的区域
// Parameters :  
//				 currImage: 待检图像
//               highThreshMat:高于金标准图像的阈值
//               lowThreshMat:低于金标准图像的阈值
//               resultImage: 输出的二值标记图像
//
// Return Value: 无
//				 
// ------------------------------------------------------------
void AlgoGoldenAnalysis::MarkDefects(Mat& currImage, Mat& highThreshMat, Mat& lowThreshMat, Mat& resultImage)
{
    Mat highMat, lowMat;
    compare(currImage, highThreshMat, highMat, CMP_GT);
    compare(currImage, lowThreshMat, lowMat, CMP_LT);

    resultImage = highMat | lowMat;
}
// ------------------------------------------------------------
// Description: 创建ROI mask图像
// Parameters :  
//              jsonROI: ROI参数json数据
//				roiMask: 在整帧图像上标示ROI的生效区域 
//				roiInfos: 每个ROI的相关参数 
// Return Value: 无
//	
// ------------------------------------------------------------
void AlgoGoldenAnalysis::CreateRoIMaskImage(const char* jsonROI, Mat& roiMask, vector<RoiInfo_t>& roiInfos)
{
	Document doc;
	doc.Parse(jsonROI);

	int polyNum = doc["polyNum"].GetInt();
	int circleNum = doc["circleNum"].GetInt();

	roiInfos.clear();
	Mat mask;
	roiMask = Mat::zeros(m_iImageHeight, m_iImageWidth, CV_8UC1);	//初始化保存所有ROI区域的mask

	if(polyNum > 0)
	{
		Value& polyArray = doc["polyArray"];

		for(int i=0; i<polyNum; i++)
		{
			mask = Mat::zeros(m_iImageHeight, m_iImageWidth, CV_8UC1);
			
			RoiInfo_t roi_info;
			roi_info.isInclude = polyArray[i]["IsInclude"].GetBool();

			int vertexNum = polyArray[i]["vertexNum"].GetInt();
			Point* poly_points = new Point[vertexNum];
			vector<Point> contours_poly;
			contours_poly.clear();

			Value& vertexArray = polyArray[i]["vertexArray"];
			for(int k=0; k<vertexNum; k++)
			{
				Point pt;
				pt.x = (int)vertexArray[k]["x"].GetFloat();
				pt.y = (int)vertexArray[k]["y"].GetFloat();
				
				poly_points[k] = pt;
				contours_poly.push_back(pt);
			}

			Value& param = polyArray[i]["param"];

			//roi_info.param.manualThreshold = param["manualThreshold"].GetFloat();
			roi_info.param.minSize = param["minSize"].GetFloat();
			roi_info.param.minArea = param["minArea"].GetFloat();
			roi_info.param.minWidth = param["minWidth"].GetFloat();
			roi_info.param.minLength = param["minLength"].GetFloat();
			roi_info.param.maxWidth = param["maxWidth"].GetFloat();
			roi_info.param.maxLength = param["maxLength"].GetFloat();

            Value& thresh = polyArray[i]["thresh"];
            roi_info.thresh.highGray = thresh["highGray"].GetInt();
            roi_info.thresh.lowGray  = thresh["lowGray"].GetInt();
            roi_info.thresh.highThreshAbove = thresh["highThreshAbove"].GetInt();
            roi_info.thresh.highThreshBelow = thresh["highThreshBelow"].GetInt();
            roi_info.thresh.lowThreshAbove = thresh["lowThreshAbove"].GetInt();
            roi_info.thresh.lowThreshBelow = thresh["lowThreshBelow"].GetInt();

			const Point* ppt[1] = { poly_points };
			int npt[] = { vertexNum };
			fillPoly( roiMask, ppt,  npt, 1, Scalar( 255), LINE_8 );
			fillPoly( mask, ppt,  npt, 1, Scalar( 255), LINE_8 );   //必须画2次，因为有ROI内嵌的可能
			delete [] poly_points;

			Rect bound;
			bound = boundingRect( Mat(contours_poly) );

			roi_info.bound = bound;
			mask(bound).copyTo(roi_info.mask);
			roiInfos.push_back(roi_info);
		}
	}

	if(circleNum>0)
	{
		Value& circleArray = doc["circleArray"];

		for(int i=0; i<circleNum; i++)
		{
			mask = Mat::zeros(m_iImageHeight, m_iImageWidth, CV_8UC1);
			
			RoiInfo_t roi_info;
			roi_info.isInclude = circleArray[i]["IsInclude"].GetBool();

			Value& center = circleArray[i]["center"];
			Point pt_center;
			pt_center.x = (int)center["x"].GetFloat();
			pt_center.y = (int)center["y"].GetFloat();

            float radius = circleArray[i]["radius"].GetFloat();

			Value& param = circleArray[i]["param"];

			//roi_info.param.manualThreshold = param["manualThreshold"].GetFloat();
			roi_info.param.minSize = param["minSize"].GetFloat();
			roi_info.param.minArea = param["minArea"].GetFloat();
			roi_info.param.minWidth = param["minWidth"].GetFloat();
			roi_info.param.minLength = param["minLength"].GetFloat();
			roi_info.param.maxWidth = param["maxWidth"].GetFloat();
			roi_info.param.maxLength = param["maxLength"].GetFloat();

            Value& thresh = circleArray[i]["thresh"];
            roi_info.thresh.highGray = thresh["highGray"].GetInt();
            roi_info.thresh.lowGray  = thresh["lowGray"].GetInt();
            roi_info.thresh.highThreshAbove = thresh["highThreshAbove"].GetInt();
            roi_info.thresh.highThreshBelow = thresh["highThreshBelow"].GetInt();
            roi_info.thresh.lowThreshAbove = thresh["lowThreshAbove"].GetInt();
            roi_info.thresh.lowThreshBelow = thresh["lowThreshBelow"].GetInt();

			circle(roiMask, pt_center, radius, Scalar(255), FILLED, LINE_8);
			circle(mask, pt_center, radius, Scalar(255), FILLED, LINE_8);   //必须画2次，因为有ROI内嵌的可能

			Rect bound;
			bound.x = pt_center.x - radius;
			bound.y = pt_center.y - radius;
			bound.width = (int)radius*2;
			bound.height = (int)radius*2;

			roi_info.bound = bound;
			mask(bound).copyTo(roi_info.mask);
			roiInfos.push_back(roi_info);
		}
	}

	int roiNum = roiInfos.size();
	if(roiNum < 2)  //ROI数目不大于1，不会有内嵌ROI，返回
		return;

	for(int i=0; i<roiNum; i++)
	{//处理可能出现ROI内嵌的情况
		RoiInfo_t& roi_outer = roiInfos[i];
		
		for(int j=0; j<roiNum; j++)
		{
			if(i==j)
				continue;

			RoiInfo_t& roi_inner = roiInfos[j];

			Rect boundOuter, boundInner;
			boundOuter = roi_outer.bound;
			boundInner = roi_inner.bound;
			
			if(IsRectInRect(boundInner, boundOuter))
			{// following: remove roi mask from roi mask
				Rect innerRect;
				innerRect.x = boundInner.x - boundOuter.x;
				innerRect.y = boundInner.y - boundOuter.y;
				innerRect.width = boundInner.width;
				innerRect.height = boundInner.height;

				roi_outer.mask(innerRect) = roi_outer.mask(innerRect) & (~roi_inner.mask);
			}
		}
	}
}

// ------------------------------------------------------------
// Description : 创建Ring mask图像
// Parameters :  
//				 
//				 
// Return Value :
//	
void AlgoGoldenAnalysis::CreateRingMaskImage(const Mat &goldenImage, Mat &maskRingImage, int nThresBin, int nRingWidth)
{
	Mat brightRegion;
	threshold(goldenImage, brightRegion, nThresBin, 255, CV_THRESH_BINARY);
	
	if (nRingWidth == 0) {
		//如果传入的ring宽是0，则不存在ring区
		maskRingImage = brightRegion - brightRegion;
		return;
	}

	//对筛选之后的连通域去环形区域
	Mat ContourDilate;
	Mat ContourErode;
	Mat element = getStructuringElement(MORPH_RECT, Size(nRingWidth, nRingWidth), Point(-1, -1));
	morphologyEx(brightRegion, ContourDilate, MORPH_DILATE, element);
	morphologyEx(brightRegion, ContourErode, MORPH_ERODE, element);

	maskRingImage = ContourDilate - ContourErode;
/*
	Mat darkregion;
	threshold(goldenImage, darkregion, 30, 255, CV_THRESH_BINARY_INV);

	vector<vector<Point>> contours;
	findContours(darkregion, contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);//CV_RETR_LIST   //CV_RETR_EXTERNAL

	double fArea = 0.0f;
	double dLength = 0.0f;
	RotatedRect rcBox;
	vector<vector<Point>>::iterator iter;

#if 0   //这样写在opencv3.3会出错
	for (iter = contours.begin(); iter != contours.end(); iter++) {
		fArea = contourArea(*iter);
		if (fArea <= 5000) {
			contours.erase(iter);
			iter--;
			continue;
		}

		
		rcBox = minAreaRect(*iter);
		dLength = max(rcBox.size.height, rcBox.size.width);

		if (dLength > 350) {
			contours.erase(iter);
			iter--;
			continue;
		}
	}
#else
    iter = contours.begin();
    while(iter!=contours.end())
    {
        fArea = contourArea(*iter);
		if (fArea <= 5000) {
			iter = contours.erase(iter);
			continue;
		}

        rcBox = minAreaRect(*iter);
		dLength = max(rcBox.size.height, rcBox.size.width);

		if (dLength > 350) {
			iter = contours.erase(iter);
			continue;
		}

        iter++;
    }
#endif

	Mat MatContour = Mat::zeros(goldenImage.size(), goldenImage.type());
	drawContours(MatContour, contours, -1, Scalar(255), CV_FILLED);
	
	//对筛选之后的连通域去环形区域
	Mat ContourDilate;
	Mat ContourErode;
	Mat element = getStructuringElement(MORPH_RECT, Size(11, 11), Point(-1, -1));
	morphologyEx(MatContour, ContourDilate, MORPH_DILATE, element);
	morphologyEx(MatContour, ContourErode, MORPH_ERODE, element);

	maskRingImage = ContourDilate - ContourErode;

//	imwrite("D://1Dil.bmp", ContourDilate);
//	imwrite("D://1Ero.bmp", ContourErode);
//	imwrite("D://1maskRingImage.bmp", maskRingImage);
*/
}

// ------------------------------------------------------------
// Description : 设置RoI参数
// Parameters :  jsonRoiSetting json格式RoI参数
//				 nSiteX: site 水平位置
//				 nSiteY: site 垂直位置
// Return Value: true - 设置成功
//				 false - 设置失败
// ------------------------------------------------------------
bool AlgoGoldenAnalysis::SetRoI(const char* jsonRoiSetting, int nSiteX, int nSiteY)
{
	if((nSiteX>=MAX_SITE_COLS) || (nSiteY>=MAX_SITE_ROWS))
	{
		cout << "ERROR: (SetRoI) site数超出最大值 " << endl;
		return false;
	}

	if(!m_bInited)
	{
		cout << "ERROR: unintied " << endl;
		return false;
	}

	Mat roiMask;
	CreateRoIMaskImage(jsonRoiSetting, roiMask, m_RoIInfos[nSiteY][nSiteX]);
	m_RoIJson[nSiteY][nSiteX] = jsonRoiSetting;
    m_MaskNonRoI[nSiteY][nSiteX] = ~roiMask;

    if(m_RoIInfos[nSiteY][nSiteX].size() > 0)
        m_bUseRoI[nSiteY][nSiteX] = true;
    else
        m_bUseRoI[nSiteY][nSiteX] = false;

	return true;
}
// ------------------------------------------------------------
// Description:  解析json数据中的对正区域
// Parameters : 
//              jsonRegion: 包含对正区域坐标的json数据
//              regions: 保存解析出的对正区域
//				 
// Return Value: 无
// ------------------------------------------------------------
void AlgoGoldenAnalysis::ExtractRegionJson(const char* jsonRegion, vector< Rect > & regions)
{
	Document doc;
	doc.Parse(jsonRegion);
	int regionNum = doc["regionNum"].GetInt();
	regions.clear();
	if(0==regionNum)
		return;
	Value& regionArray = doc["regionArray"];
	for(int i=0; i<regionNum; i++)
	{
		Rect region;
		region.x = regionArray[i]["x"].GetInt();
		region.y = regionArray[i]["y"].GetInt();
		region.width = regionArray[i]["width"].GetInt();
		region.height = regionArray[i]["height"].GetInt();
		regions.push_back(region);
	}
}
// ------------------------------------------------------------
// Description:  设置对正搜索范围，该范围对所有的site生效
// Parameters :  
//				 alignOffX: 水平对正在[-alignOffX, alignOffX]
//				 alignOffY: 垂直对正在[-alignOffY, alignOffY]
// Return Value: 无
//				 
// ------------------------------------------------------------
void AlgoGoldenAnalysis::SetAlignRange(int alignOffX, int alignOffY)
{
	m_AlignOffX = alignOffX;
	m_AlignOffY = alignOffY;
}
// ------------------------------------------------------------
// Description:  设置用于对正的区间，针对当前的site设置
// Parameters :  
//				 jsonRegion: json数据，包含了区间的数目及坐标
//				 nSiteX: site 水平位置
//				 nSiteY: site 垂直位置
//
// Return Value: true - 设置成功
//				 false - 设置失败
// ------------------------------------------------------------
void AlgoGoldenAnalysis::SetAlignRegion(const char* jsonRegion, int nSiteX, int nSiteY)
{
	ExtractRegionJson(jsonRegion, m_AlignRegions[nSiteY][nSiteX]);
}
// ------------------------------------------------------------
// Description:  设置金标准图片，同时根据金标准图像生成前景和背景图像
// Parameters :  imageData 金标准图片数据
//				 siteX 水平坐标
//				 siteY 垂直坐标
// Return Value: true - 设置成功
//				 false - 设置失败
// ------------------------------------------------------------
bool AlgoGoldenAnalysis::SetGoldenSiteImage(unsigned char* imageData, int nSiteX, int nSiteY)
{
	if((nSiteX>=MAX_SITE_COLS) || (nSiteY>=MAX_SITE_ROWS))
	{
		cout << "ERROR: (SetGoldenSiteImage) site数超出最大值 " << endl;
		return false;
	}

	if(!m_bInited)
	{
		cout << "ERROR: unintied " << endl;
		return false;
	}

	Mat goldenImage;
	goldenImage = Mat(m_iImageHeight, m_iImageWidth, CV_8UC1, imageData);

	m_GoldenSiteImages[nSiteY][nSiteX] = goldenImage;

    //==========================================
	Mat ring;
	ring = m_GoldenMaskRings[nSiteY][nSiteX];
	CreateRingMaskImage(goldenImage, ring, m_nRingThres, m_nRingWidth);
    m_GoldenMaskRings[nSiteY][nSiteX] = ~ring;
    //==========================================

	CreateHighLowThreshMat(m_siteThreshGroups[nSiteY][nSiteX], goldenImage, m_HighThreshMat[nSiteY][nSiteX], m_LowThreshMat[nSiteY][nSiteX]);

    if(m_siteThreshGroups[nSiteY][nSiteX].highGray==m_siteThreshGroups[nSiteY][nSiteX].lowGray)
    {
        Mat element = getStructuringElement(MORPH_ELLIPSE, Size(9, 9), Point(-1, -1) );
        Mat edge;
        Mat brightRegion;
        threshold(goldenImage, brightRegion, m_siteThreshGroups[nSiteY][nSiteX].highGray, 255, THRESH_BINARY);
        
        Mat sBright;
        erode(brightRegion, sBright, element);            
        edge = brightRegion & (~sBright);
        
        m_HighThreshMat[nSiteY][nSiteX] |= edge;    //edge set 255
        m_LowThreshMat[nSiteY][nSiteX] &= (~edge);  //edge set 0

        Mat darkRegion;
        threshold(goldenImage, darkRegion, m_siteThreshGroups[nSiteY][nSiteX].highGray, 255, THRESH_BINARY_INV);
        
        Mat sDark;
        erode(darkRegion, sDark, element);           
        edge = darkRegion & (~sDark);
        
        m_HighThreshMat[nSiteY][nSiteX] |= edge;    //edge set 255
        m_LowThreshMat[nSiteY][nSiteX] &= (~edge);  //edge set 0
    }
        
    if(m_bUseRoI[nSiteY][nSiteX])
    {
        vector<RoiInfo_t>::iterator roiIt = m_RoIInfos[nSiteY][nSiteX].begin();			
		while(roiIt!=m_RoIInfos[nSiteY][nSiteX].end())
		{
			Rect bound = roiIt->bound;				
			ThreshGroup_t threshGroup = roiIt->thresh;
            
            Mat goldenMat = goldenImage(bound);
            Mat highMat, lowMat;
                           
            CreateHighLowThreshMat(threshGroup, goldenMat, highMat, lowMat);
            highMat = highMat & roiIt->mask;
            lowMat  = lowMat  & roiIt->mask;//ROI区域的阈值

            Mat nonRoI = ~(roiIt->mask);
            Mat highThreshMat, lowThreshMat;
            highThreshMat = m_HighThreshMat[nSiteY][nSiteX](bound);
            lowThreshMat  = m_LowThreshMat[nSiteY][nSiteX](bound);

            highThreshMat = highThreshMat & nonRoI;
            lowThreshMat  = lowThreshMat  & nonRoI; //保留非ROI区域的阈值

            highThreshMat = highThreshMat | highMat;
            lowThreshMat  = lowThreshMat  | lowMat; //将ROI区域的阈值添加进阈值矩阵
            
			roiIt++;
		}
    }
	
    //-----------------------------------------------------------------------------
	m_GoldenSiteHorzProj[nSiteY][nSiteX] = Mat::zeros(1, m_iImageHeight, CV_32FC1);
	m_GoldenSiteVertProj[nSiteY][nSiteX] = Mat::zeros(1, m_iImageWidth,  CV_32FC1);

	GrayProjections(goldenImage, m_GoldenSiteHorzProj[nSiteY][nSiteX], m_GoldenSiteVertProj[nSiteY][nSiteX]);


	Rect horjRange = cvRect(m_AlignOffY, 0, (m_iImageHeight-2*m_AlignOffY), 1);
	Rect vertRange = cvRect(m_AlignOffX, 0, (m_iImageWidth-2*m_AlignOffX), 1);
	
	Mat horzProj = m_GoldenSiteHorzProj[nSiteY][nSiteX](horjRange);	
	Mat vertProj = m_GoldenSiteVertProj[nSiteY][nSiteX](vertRange);

	Mat meanMat,stddevMat;
	
	meanStdDev(horzProj, meanMat, stddevMat);
	m_GoldenSiteHorzProjMean[nSiteY][nSiteX] = meanMat.at<double>(0, 0);
	m_GoldenSiteHorzProjStd[nSiteY][nSiteX] = stddevMat.at<double>(0, 0);
	//cout<<"horz: mean = "<<meanMat<<",  stddev = "<<stddevMat<<endl;
  
	meanStdDev(vertProj, meanMat, stddevMat);
	m_GoldenSiteVertProjMean[nSiteY][nSiteX] = meanMat.at<double>(0, 0);
	m_GoldenSiteVertProjStd[nSiteY][nSiteX] = stddevMat.at<double>(0, 0);
	//cout<<"vert: mean = "<<meanMat<<",  stddev = "<<stddevMat<<endl;
	
	return true;
}
// ------------------------------------------------------------
// Description:  统计图像水平垂直投影信息
// Parameters :  
//	            image: 输入的待统计图像
//              horzProj: 输入图像的水平投影
//              vertProj: 输入图像的垂直投影
//
// Return Value: true - 统计成功
//				 false - 统计失败
// ------------------------------------------------------------
bool AlgoGoldenAnalysis::GrayProjections(const Mat& image, Mat& horzProj, Mat& vertProj)
{
	if((image.cols > vertProj.cols) || (image.rows > horzProj.cols))
	{
		cout << "ERROR: (GrayProjections) 图像过大" << endl;
		return false;
	}

	int nRows = image.rows;
	int nCols = image.cols;

	int i, j;
	float pixel;
	const uchar* pImage;
	float *pHorz, *pVert0, *pVert;

	horzProj = Mat::zeros(horzProj.size(), CV_32FC1);
	vertProj = Mat::zeros(vertProj.size(),  CV_32FC1);

	pHorz = horzProj.ptr<float>(0);
	pVert0 = vertProj.ptr<float>(0);
	pVert = pVert0;

	for( i = 0; i < nRows; ++i)
	{
		pImage = image.ptr<uchar>(i);
		
		for ( j = 0; j < nCols; ++j)
		{
			pixel = (float)pImage[j];
			*pHorz += pixel;
			*pVert++ += pixel;
		}

		pHorz++;
		pVert = pVert0;
	}

	return true;
}
// ------------------------------------------------------------
// Description:  MOSTACCURATE方式对正的CPU版
// Parameters :  
//	            baseImage: 输入的对正基准图像，指goldenImage
//              alignImage: 输入需要对正的图像，指当前待检测图像
//				nSiteX: site 水平位置
//				nSiteY: site 垂直位置
//				offX: 水平对正在[-offX, offX]
//				offY: 垂直对正在[-offY, offY]
//              fScore: 匹配值
//
// Return Value: 最佳匹配位置的偏移量
// ------------------------------------------------------------
Point AlgoGoldenAnalysis::CalcAlignOffset_best_cpu(const Mat& baseImage, const Mat& alignImage, int nSiteX, int nSiteY, int offX, int offY, float &fScore)
{
	int regionNum = m_AlignRegions[nSiteY][nSiteX].size();
	Point shift;
	if(regionNum > 0)
	{
		float sx, sy, score;
		sx = sy = 0;
		fScore = 0;
		for(int j=0; j<regionNum; j++)
		{
			Mat base, align;
			Rect region = m_AlignRegions[nSiteY][nSiteX][j];
			base = baseImage(region);
			align = alignImage(region);
			Point off = CalcAlignOffset_ocv_cpu(base, align, offX, offY, score);
			#ifdef _DEBUG
			cout << " region " << j << ":  off_x=" <<off.x <<", off_y=" << off.y << endl;
			#endif
			sx += off.x;
			sy += off.y;
            fScore += score;
		}
		sx = sx / regionNum;
		sy = sy / regionNum;
		shift.x = (int)sx;
		shift.y = (int)sy;
        fScore = fScore / regionNum;
	}
	else
	{
		shift =  CalcAlignOffset_ocv_cpu(baseImage, alignImage, m_AlignOffX, m_AlignOffY, fScore);
	}
	return shift;
}
// ------------------------------------------------------------
// Description: 计算两幅图片偏移，用OpenCV的CPU匹配函数
// Parameters :
//	            baseImage: 输入的对正基准图像
//              alignImage: 输入需要对正的图像
//				offX: 水平对正在[-offX, offX]
//				offY: 垂直对正在[-offY, offY]
//              fScore: 匹配值
//
// Return Value: 最佳匹配位置的偏移量
// ------------------------------------------------------------
Point AlgoGoldenAnalysis::CalcAlignOffset_ocv_cpu(const Mat& baseImage, const Mat& alignImage, int offX, int offY, float &fScore)
{
	Rect  templRect;	//对正模板区域
	templRect.x = offX;
	templRect.y = offY;
	templRect.width = baseImage.cols - offX*2;
	templRect.height = baseImage.rows - offY*2;

	int match_method;
	match_method = CV_TM_SQDIFF_NORMED;

	Mat templ, result;
	templ = baseImage(templRect);

	int result_cols = baseImage.cols - templ.cols + 1;
	int result_rows = baseImage.rows - templ.rows + 1;
	result.create( result_rows, result_cols, CV_32FC1 );

	matchTemplate( alignImage, templ, result, match_method );

	double minVal; double maxVal; Point minLoc; Point maxLoc;
	Point matchLoc;
	minMaxLoc( result, &minVal, &maxVal, &minLoc, &maxLoc, Mat() );

	Point off;
	off.x = minLoc.x - offX;
	off.y = minLoc.y - offY;
	
	fScore = (float)(1 - minVal) * 100;
	if (fScore < 0) {
		fScore = 0;
	}
	return off;
}
// ------------------------------------------------------------
// Description: FASTEST方式对正的CPU版，投影匹配算法
// Parameters :  
//              alignImage: 输入需要对正的图像，指当前待检测图像
//				nSiteX: site 水平位置
//				nSiteY: site 垂直位置
//              fScore: 匹配值
//
// Return Value: 最佳匹配位置的偏移量
// ------------------------------------------------------------
Point AlgoGoldenAnalysis::CalcAlignOffset_proj_cpu(const Mat& alignImage, int nSiteX, int nSiteY, float &fScore)
{
	GrayProjections(alignImage, m_HorzProjMat, m_VertProjMat);

	double mean_h, std_h, mean_v, std_v;

	mean_h = m_GoldenSiteHorzProjMean[nSiteY][nSiteX];
	std_h  = m_GoldenSiteHorzProjStd[nSiteY][nSiteX];
	
	mean_v = m_GoldenSiteVertProjMean[nSiteY][nSiteX];
	std_v  = m_GoldenSiteVertProjStd[nSiteY][nSiteX];

	Rect horjRange = cvRect(m_AlignOffY, 0, (m_iImageHeight-2*m_AlignOffY), 1);
	Rect vertRange = cvRect(m_AlignOffX, 0, (m_iImageWidth-2*m_AlignOffX), 1);
	
	Mat goldenHorzProj = m_GoldenSiteHorzProj[nSiteY][nSiteX](horjRange);	
	Mat goldenVertProj = m_GoldenSiteVertProj[nSiteY][nSiteX](vertRange);

	Mat prodMat = m_ProdMat(horjRange);

	int dx, dy;
	double dmax_coex, dmax_coey;
	dmax_coex = numeric_limits<double>::min();
	dx = dy = 0;

	for(int i=0; i<2*m_AlignOffY; i++)
	{
		horjRange.x = i;
		double coeff = CalcMatchCoeff(m_HorzProjMat(horjRange), goldenHorzProj, mean_h, std_h, prodMat);

		if(coeff > dmax_coex)
		{
			dy = i;
			dmax_coex = coeff;
		}
	}

	dmax_coey = numeric_limits<double>::min();
	prodMat = m_ProdMat(vertRange);
	for(int i=0; i<2*m_AlignOffX; i++)
	{
		vertRange.x = i;
		double coeff = CalcMatchCoeff(m_VertProjMat(vertRange), goldenVertProj, mean_v, std_v, prodMat);

		if(coeff > dmax_coey)
		{
			dx = i;
			dmax_coey = coeff;
		}
	}

	//匹配分数设定为X,Y两个方向的最小值
	fScore = min(dmax_coex, dmax_coey) * 100;

	Point off;
	off.x = dx - m_AlignOffX;
	off.y = dy - m_AlignOffY;
	return off;
}
// ------------------------------------------------------------
// Description: 计算2个向量的相关系数
// Parameters :  
//              mat1: 输入向量1
//              mat2: 输入向量2
//				m2: 向量2的均值
//				s2: 向量2的标准差
//              prodMat: 计算时需要的中间值缓存向量
//
// Return Value: 相关系数
// ------------------------------------------------------------
double AlgoGoldenAnalysis::CalcMatchCoeff(const Mat& mat1, const Mat& mat2, double m2, double s2, Mat& prodMat)
{
	Mat meanMat,stddevMat;	
	meanStdDev(mat1, meanMat, stddevMat);

	double m1, s1, m12, den;
	m1 = meanMat.at<double>(0, 0);
	s1 = stddevMat.at<double>(0, 0);
	den = s1*s2;
	if(den > -0.00001 && den < 0.00001)
		return 0.0;

	prodMat = mat1.mul(mat2);
	m12 = mean(prodMat)[0];

	den = abs(m12 - m1*m2)/den;
	return den;
}
// ------------------------------------------------------------
// Description:  根据配置文件参数去除无关区域
// Parameters :  ioRegionMat: 【输入输出值】检出区域
//				 configManage: 配置参数对象
//               contours: 保存检测出的缺陷
//
// Return Value: true - 检测出缺陷
//				 false - 无缺陷检测出
// ------------------------------------------------------------
bool AlgoGoldenAnalysis::RemoveIrrelevantRegions(Mat& ioRegionMat, const ConfigurationManage& configManage, vector< vector<Point> >& contours)
{
	//vector< vector<cv::Point> > contours;	
	//vector<Vec4i> hierarchy;

	findContours(ioRegionMat, contours,
			CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

	#ifdef _DEBUG
		cout << "first " << contours.size() <<"  defects found " <<  endl;
	#endif

	if (contours.empty())
	{
		return false;
	}

	vector< vector<cv::Point> >::iterator it = contours.begin();

	while(it!=contours.end())
	{
		bool bErase = false;
		
		if (configManage.m_MinArea > 0)
		{
			// 将小于配置规定面积的区域填充为0
			double area = contourArea(*it);
			bErase = (area < configManage.m_MinArea);

			if(!bErase)
			{
				RotatedRect boundingBox = minAreaRect(*it);

				int width = min(boundingBox.size.width, boundingBox.size.height);
				int length = max(boundingBox.size.width, boundingBox.size.height);

				if((width < configManage.m_MinWidth) && (length < configManage.m_MinLength))
				{
					bErase = true;
				}
			}
		}

		#if 0
		// 将小于配置规定size的区域填充为0
		if (configManage.m_MinSize > 0)
		{
			RotatedRect boundingBox = minAreaRect(*it);
            int size = min(boundingBox.size.width, boundingBox.size.height);         
			bErase = (bErase || size < configManage.m_MinSize);
		}
		#endif

		if(bErase)
		{
			it = contours.erase(it);
		}
		else
		{
			++it;
		}
	}

	if(0==contours.size())
		return false;

	return true;
}
// ------------------------------------------------------------
// Description:  计算当前图像与金标准图像的偏差, CPU版
// Parameters :  goldenImage 当前图像
//				 currImage 金标准图像
//				 nSiteX: site 水平位置
//				 nSiteY: site 垂直位置
//
// Return Value: 偏差坐标
// ------------------------------------------------------------
Point AlgoGoldenAnalysis::MatchImage_CPU(const Mat& goldenImage, const Mat& currImage, int nSiteX, int nSiteY, float& alignScore)
{
	Point off;
	float fScore;

	switch(m_MatchMode)
	{
		case FASTEST:
			off =  CalcAlignOffset_proj_cpu(currImage, nSiteX, nSiteY, fScore);
			break;
			
		case MOSTACCURATE:
			off =  CalcAlignOffset_best_cpu(goldenImage, currImage, nSiteX, nSiteY, m_AlignOffX, m_AlignOffY, fScore);
			break;
			
		default:
			off = CalcAlignOffset_best_cpu(goldenImage, currImage, nSiteX, nSiteY, m_AlignOffX, m_AlignOffY, fScore);
	}

	alignScore = fScore;

	return off;
}
// ------------------------------------------------------------
// Description : 计算当前图像的对正区域
// Parameters :  
//               shiftLoc: 当前图像相对于goldenImage的偏移
//               siteRect: 当前图像的site区域
//               matchRect: 偏移后的site区域
//				 
// Return Value: true - 计算成功
//				 false - 计算失败
// ------------------------------------------------------------
bool AlgoGoldenAnalysis::CalcMatchedRect(Point shiftLoc, Rect& siteRect, Rect& matchRect)
{
	int dx, dy;
	dx = shiftLoc.x;
	dy = shiftLoc.y;

	matchRect = siteRect;
	matchRect.x += dx;
	matchRect.y += dy;

	if(matchRect.x < 0)
	{
		int nDelta = -matchRect.x;
		matchRect.width -= nDelta;       //缩小样本宽度
		siteRect.width = matchRect.width;
		matchRect.x = 0;
	}

	if(matchRect.y < 0)
	{
		int nDelta = -matchRect.y;
		matchRect.height -= nDelta;       //缩小样本高度
		siteRect.height = matchRect.height;
		matchRect.y = 0;
	}

	if((matchRect.x+matchRect.width)>m_iImageWidth)
	{
		int ndelta = matchRect.x + matchRect.width - m_iImageWidth;
		matchRect.width -= ndelta;
		siteRect.width = matchRect.width;
		siteRect.x += ndelta;
	}

	if((matchRect.y+matchRect.height)>m_iImageHeight)
	{
		int ndelta = matchRect.y + matchRect.height - m_iImageHeight;
		matchRect.height -= ndelta;
		siteRect.height = matchRect.height;
		siteRect.y += ndelta;
	}

 	return true;
}
// ------------------------------------------------------------
// Description:  检测图像缺陷, CPU版
// Parameters :  
//				 currImage: 待检图像
//				 siteX: 水平坐标
//				 siteY: 垂直坐标
//               defectsContours: 检测出的缺陷
//
// Return Value: 检测结果
// ------------------------------------------------------------
AlgorithmErrorType  AlgoGoldenAnalysis::DoAnalysis_cpu(const Mat& currImage, int nSiteX, int nSiteY, vector< vector<Point> >& defectsContours)																									
{
	Mat goldenImage, rgMaskImage;

	goldenImage = m_GoldenSiteImages[nSiteY][nSiteX];
	rgMaskImage = m_GoldenMaskRings[nSiteY][nSiteX];

	if(goldenImage.empty() || rgMaskImage.empty())
		return NO_INITED;

    float alignScore;
	Point shiftLoc = MatchImage_CPU(goldenImage, currImage, nSiteX, nSiteY, alignScore);

	#ifdef _DEBUG
	printf("match shift: x=%d, y=%d, score=%4.2f \n", shiftLoc.x, shiftLoc.y, alignScore);
	#endif

    if(alignScore < m_fMinAlignScore)
        return ALIGN_FAIL;
	
	Rect matchRect, siteRect;
	siteRect = m_SiteRect[nSiteY][nSiteX];
	bool ret = CalcMatchedRect(shiftLoc, siteRect, matchRect);
	if(false==ret)
		return SITE_OUTSIDE;

	ConfigurationManage config;
	config.m_MinArea = m_SiteParam[nSiteY][nSiteX].minArea;
	config.m_MinSize = m_SiteParam[nSiteY][nSiteX].minSize;
	config.m_MinWidth = m_SiteParam[nSiteY][nSiteX].minWidth;
	config.m_MinLength = m_SiteParam[nSiteY][nSiteX].minLength;
	
	Mat currMat = currImage(matchRect);
	Mat bufferMat = m_BufferMat(siteRect);
	Mat diffMat = m_DiffFGImage(matchRect);
	Mat diffRegions = m_DiffRegions(matchRect);

	bool bDefectFound = false;
	bool bUseRoI = m_bUseRoI[nSiteY][nSiteX];

    Mat highThresh = m_HighThreshMat[nSiteY][nSiteX](siteRect); 
    Mat lowThresh = m_LowThreshMat[nSiteY][nSiteX](siteRect);
    MarkDefects(currMat, highThresh, lowThresh, bufferMat); 
//    morphologyEx(bufferMat, diffRegions, MORPH_OPEN, m_MorphElement); //diffRegions:所有缺陷，包括ROI上的

	diffRegions = bufferMat;
    //==========================================
    Mat maskRg = rgMaskImage(siteRect);
    diffRegions = diffRegions & maskRg;
    //==========================================

    
    if(bUseRoI)
    {//有ROI
        Mat maskMat = m_MaskNonRoI[nSiteY][nSiteX](siteRect);
		diffMat = diffRegions & maskMat;    //屏蔽ROI区域的缺陷
		ret = RemoveIrrelevantRegions(diffMat, config, defectsContours);//获取非ROI区域上的缺陷
        if(ret)
    	{
    		Point offset;
    		offset.x = matchRect.x;
    		offset.y = matchRect.y;
    		int num = defectsContours.size();
    		for(int i=0; i<num; i++)
    		{
    			int np = defectsContours[i].size();

    			for(int j=0; j<np; j++)
    			{
    				defectsContours[i][j] += offset;
    			}
    		}
    		bDefectFound = true;    //检测出缺陷
    	}

        vector<RoiInfo_t>::iterator roiIt = m_RoIInfos[nSiteY][nSiteX].begin();		
		while(roiIt!=m_RoIInfos[nSiteY][nSiteX].end())
        {
            if(false==roiIt->isInclude)
			{
				roiIt++;
				continue;
			}

            Rect bound = roiIt->bound;
			bound.x += shiftLoc.x;
			bound.y += shiftLoc.y;

            Param_t param = roiIt->param;            
			config.m_MinArea = param.minArea;
			config.m_MinSize = param.minSize;
			config.m_MinWidth = param.minWidth;
			config.m_MinLength = param.minLength;

            Mat roiBuffer = m_BufferMat(bound);
            Mat roiRegion = m_DiffRegions(bound);   // 等同于diffRegions
            roiBuffer = roiRegion & (roiIt->mask);  // 屏蔽非ROI区域的缺陷
            
            vector< vector<Point> > contours;
			ret =  RemoveIrrelevantRegions(roiBuffer, config, contours);
			if(ret)
			{
				Point offset;
				offset.x = bound.x;
				offset.y = bound.y;
				int num = contours.size();
				for(int i=0; i<num; i++)
				{
					int np = contours[i].size();

					for(int j=0; j<np; j++)
					{
						contours[i][j] += offset;
					}

					defectsContours.push_back(contours[i]);
				}
				bDefectFound = true;//检测出缺陷
			}

            roiIt++;
        }
    }
    else
    {//无ROI
        ret = RemoveIrrelevantRegions(diffRegions, config, defectsContours);
        if(ret)
    	{
    		Point offset;
    		offset.x = matchRect.x;
    		offset.y = matchRect.y;
    		int num = defectsContours.size();
    		for(int i=0; i<num; i++)
    		{
    			int np = defectsContours[i].size();

    			for(int j=0; j<np; j++)
    			{
    				defectsContours[i][j] += offset;
    			}
    		}
    		bDefectFound = true;    //检测出缺陷
    	}
    }

	if(!bDefectFound)
	{
		#ifdef _DEBUG
		cout << "No defect found at site: x= " << nSiteX << ",y=" << nSiteY << endl;
		#endif
		return NO_DEFECT;	
	}
	
	return ERROR_NONE;
}
// ------------------------------------------------------------
// Description : 检测图像缺陷
// Parameters :  imageData 待检图像数据
//				 nDieId die序号
//				 siteX 水平坐标
//				 siteY 垂直坐标
// Return Value: 检测结果
// ------------------------------------------------------------
AlgorithmErrorType  AlgoGoldenAnalysis::Analysis(unsigned char* imageData, int nDieId, int nSiteX, int nSiteY)	
{
	if(!m_bInited)
	{
		cout << "ERROR: unintied " << endl;
		return NO_INITED;
	}

    //////////////////////////////////////////////////////////////////////////////////////
    if (true == m_bEnableClustering)                                                    //
    {                                                                                   //
        m_nDieId = nDieId;                                                              //
        m_nSiteX = nSiteX;                                                              //
        m_nSiteY = nSiteY;                                                              //
        m_matClustering = Mat::zeros(Size(m_iImageWidth, m_iImageHeight), CV_8UC3);     //
    }                                                                                   //
    //////////////////////////////////////////////////////////////////////////////////////

	Mat currImage = Mat(m_iImageHeight, m_iImageWidth, CV_8UC1, imageData);
	vector< vector<Point> > defectsContours;
	defectsContours.clear();
	AlgorithmErrorType err = DoAnalysis_cpu(currImage, nSiteX, nSiteY, defectsContours);
	Mat resultImage;

	if(err==ERROR_NONE)
	{
		if(m_bSaveImage && m_bDrawDefects)
		{
			cvtColor(currImage, m_ResultImage, COLOR_GRAY2BGR);
			resultImage = m_ResultImage;
		}
	}
	else
		return err;

	vector<defectDescription<float> > outputResult;
	bool ret = ClassifyResult(defectsContours, outputResult, currImage, resultImage, m_matClustering);
	if(false==ret)
	{
		return DOCLASSIFY_FAIL;
	}

	if(m_bSaveImage)
	{
		if(resultImage.empty())
			resultImage = currImage;
	}
	OutputResult(m_outputPath, outputResult, resultImage, nDieId, nSiteX, nSiteY);

    //////////////////////////////////////////////////////////
    if (true == m_bEnableClustering)                        //
    {                                                       //
        DoSpacialClustering(m_nRadius, m_matClustering);    //
    }                                                       //
    //////////////////////////////////////////////////////////
    
	return err;
}
// ------------------------------------------------------------
// Description:  检测图像缺陷,内部测试使用
// Parameters :  imageData: 待检图像数据
//				 nDieId: die序号
//				 siteX: 水平坐标
//				 siteY: 垂直坐标
//               outputResult: 检测结果描述
//               resultData: 标识缺陷的检测结果图
//
// Return Value: 检测结果
// ------------------------------------------------------------
AlgorithmErrorType  AlgoGoldenAnalysis::AnalysisInner(unsigned char* imageData, int nDieId, int nSiteX, int nSiteY,
														  std::vector<defectDescription<float> >& outputResult, 
														  unsigned char* resultData)													
{
	Mat currImage = Mat(m_iImageHeight, m_iImageWidth, CV_8UC1, imageData);
	vector< vector<Point> > defectsContours;
	defectsContours.clear();
	AlgorithmErrorType err = DoAnalysis_cpu(currImage, nSiteX, nSiteY, defectsContours);
	
	Mat resultImage; 

	if(err==ERROR_NONE)
	{
		if(m_bSaveImage && m_bDrawDefects)
		{
			resultImage = Mat(m_iImageHeight, m_iImageWidth, CV_8UC3, resultData);
			cvtColor(currImage, resultImage, COLOR_GRAY2BGR);
		}
	}
	else
		return err;

	bool ret = ClassifyResult(defectsContours, outputResult, currImage, resultImage, m_matClustering);
	if(false==ret)
	{
		return DOCLASSIFY_FAIL;
	}
	if(m_bSaveImage)
	{
		if(resultImage.empty())
			resultImage = currImage;
	}
	#ifdef _DEBUG
		cout << "Total " << defectsContours.size() <<"  defects found at site: x= " << nSiteX << ",y=" << nSiteY << endl;
	#endif

	string result_save_path = ".\\resultPath\\";
	OutputResult(result_save_path, outputResult, resultImage, nDieId, nSiteX, nSiteY);

	//imwrite("result.png", resultImage);
	
	return ERROR_NONE;
}
// ------------------------------------------------------------
// Description:  检测结果分类，在检测图片上标识
// Parameters :  
//               defectsContours: 检测出的缺陷轮廓
//               outputResult: 缺陷描述
//               currImage: 待检测图像
//               resultImage: 缺陷标识图像
//				 
// Return Value: true - 分类成功
//				 false - 分类失败
// ------------------------------------------------------------
bool AlgoGoldenAnalysis::ClassifyResult(vector< vector<Point> >& defectsContours, vector<defectDescription<float> >& outputResult, Mat& currImage, Mat& resultImage, Mat& matClustering)
{
	DefectsClassify defectClassify;

	bool bVerify;
	vector<defectDescription<float> > output;
	bVerify = defectClassify.DoClassify(defectsContours, currImage, m_BufferMat, m_fPixelSizeRatio,
										  m_configManage.m_strategyName, resultImage, output);

	if (!bVerify)
	{
		//std::cout << "DefectsClassify::DoClassify() fail!" << std::endl;
		return false;
	} 
	 
	outputResult.clear();
	if(output.size() > 0)
	{
		int size = output.size();
		for(int i=0; i<size; i++)
			outputResult.push_back(output[i]);
	}
	defectClassify.m_matClustering.copyTo(matClustering);

	return true;
}
// ------------------------------------------------------------
// Description:  输出检测结果
// Parameters :  
//               outputResult: 缺陷描述	
//               resultMat: 缺陷标识图像
//				 nDieId: die序号
//				 siteX: 水平坐标
//				 siteY: 垂直坐标
//
// Return Value: 无
// ------------------------------------------------------------
void AlgoGoldenAnalysis::OutputResult(string& outputPath, vector<defectDescription<float> >& outputResult, Mat& resultMat, int nDieId, int nSiteX, int nSiteY)
{
	 // 保存图像结果到文件
    string strSavePath;
	char buffer[256];
	sprintf(buffer, "D%04dS%02d%02d", nDieId, nSiteX, nSiteY);
	strSavePath = buffer;


	string strImgSavePath = outputPath + "\\" + strSavePath;
	string strJsonSavePath = outputPath + "\\" + strSavePath;
    strImgSavePath += ".png";
    strJsonSavePath += ".json";

	if(m_bSaveImage)	
		imwrite(strImgSavePath.c_str(), resultMat);	
	
	DefectDescriptionStructToJsonFile(outputResult, strJsonSavePath);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void AlgoGoldenAnalysis::SetClustering( bool bEnableClustering, unsigned int nRadius)
{
    m_bEnableClustering = bEnableClustering;
	m_nRadius = nRadius;
}

void AlgoGoldenAnalysis::OutputClusteringResult(string& outputPath, vector<defectDescription<float> >& outputResult, Mat& resultMat, int nDieId, int nSiteX, int nSiteY)
{
	 // 保存图像结果到文件
    string strSavePath;
	char buffer[256];
	sprintf(buffer, "D%04dS%02d%02d", nDieId, nSiteX, nSiteY);
	strSavePath = buffer;


	string strImgSavePath = outputPath + "\\" + strSavePath + "Clustering";
	string strJsonSavePath = outputPath + "\\" + strSavePath + "Clustering";
    strImgSavePath += ".png";
    strJsonSavePath += ".json";

	if(m_bSaveImage)	
		imwrite(strImgSavePath.c_str(), resultMat);	
	
	DefectDescriptionStructToJsonFile(outputResult, strJsonSavePath);
}

bool AlgoGoldenAnalysis::DoSpacialClustering(float fRadius, Mat matSrc)
{
	bool ret;
    Mat matClustering;

	if (1.0 > fRadius)
	{
		return INPUT_ERROR;
	}

    
    Mat matKernel = Mat::ones(fRadius, fRadius, CV_8UC1);

    dilate(matSrc, matClustering, matKernel);
    erode(matClustering, matClustering, matKernel);
	//imshow("matSrc", matSrc);imshow("matClustering0", matClustering);

	ConfigurationManage config;

	config.m_MinArea = m_SiteParam[m_nSiteY][m_nSiteX].minArea;
	config.m_MinSize = m_SiteParam[m_nSiteY][m_nSiteX].minSize;
    
	Mat matCurrImage;
	cvtColor(matClustering, matCurrImage, COLOR_BGR2GRAY);
	vector< vector<Point> > defectsContours;
	defectsContours.clear();

	AlgorithmErrorType err = ERROR_NONE;
	//AlgorithmErrorType err = DoAnalysis_cpu(matCurrImage, m_nSiteX, m_nSiteY, defectsContours);
    ret =  RemoveIrrelevantRegions(matCurrImage, config, defectsContours);
	//if(ret)	{}

	Mat resultImage;

	if(err==ERROR_NONE)
	{
		if(m_bSaveImage && m_bDrawDefects)
		{
			cvtColor(matCurrImage, m_ResultImage, COLOR_GRAY2BGR);
			resultImage = m_ResultImage;
		}
	}
	else
		return err;

    //imshow("matCurrImage", matCurrImage);imshow("resultImage1", resultImage);
	vector<defectDescription<float> > outputResult;
	ret = ClassifyResult(defectsContours, outputResult, matCurrImage, resultImage, matClustering);
	if(false==ret)
	{
		return DOCLASSIFY_FAIL;
	}
	//imshow("resultImage2", resultImage);waitKey(1);


#if 0
	char szTemp[255];
	sprintf(szTemp, "(%f, %f, %f, %f)", fRadius);
	AfxMessageBox((LPCTSTR)szTemp, 0);
#endif
    OutputClusteringResult(m_outputPath, outputResult, resultImage, m_nDieId, m_nSiteX, m_nSiteY); //
	return true;
}
