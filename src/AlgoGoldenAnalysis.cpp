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
// Description : �������׼ͼƬ
// Parameters :  strFilesPath ��Ž��׼ͼƬ���ļ�·��
//				 unsigned char* baseImage ���׼ͼƬ�Ļ�׼ͼƬ
//				 unsigned char* goldenImage �������ɵĽ��׼ͼƬ
//				 offX: matchˮƽƫ�Ʒ�Χ[-offX, offX]
//				 offY: match��ֱƫ�Ʒ�Χ[-offX, offX]
//				 jsonRegion: ���ڶ�����ROI����
// Return Value :true - ���óɹ�
//				 false - ����ʧ��
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
		cout << "ERROR: ����goldenͼ������ָ�벻��Ϊ�� " << endl;
		return false;
	}

	if (!PathIsDirectory(strFilesPath.c_str()))
	{
		cout << "ERROR: golden path ���� " << endl;
		return false;
	}
	
	vector<string> vImageFileNames;
	GetImgNameFromDir(strFilesPath, vImageFileNames);   //��ȡĿ¼�������ļ���

	int fileNum = vImageFileNames.size();
	
	if(1==fileNum)
	{//���Ŀ¼��ֻ��һ���ļ���ֱ���ø��ļ���Ϊgolden image
		string fname = strFilesPath + vImageFileNames[0];
		Mat img = imread(fname.c_str(), CV_LOAD_IMAGE_GRAYSCALE);
		Mat gold = Mat(img.size(), CV_8UC1, goldenImageData);
		img.copyTo(gold);
		return true;
	}
	
	if(fileNum == 0)
	{
		cout << "ERROR: golden path �ļ���Ŀ���� " << endl;	//��������goldenͼƬ��Ŀ¼��������һ��ͼ���ļ�
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
		vCandiImages.insert(vCandiImages.begin(), baseImage);	//baseImage���ڵ�һ��λ��
	}
	else
	{//û��baseImageData���룬��Ŀ¼�µĵ�һ���ļ���ΪbaseImage
		baseImage = vCandiImages[0];
	}
	goldenImage = Mat(baseImage.size(), CV_8UC1, goldenImageData);

	if(0==regionNum)
	{//û�����ö������䣬������ͼ�����
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
        //���㵱ǰͼƬ��baseͼƬ�Ĺ�������
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
        //������ǰͼƬ����������Ӧλ��
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
// Description: ʹ����ֵ�˲��������׼ͼƬ
// Parameters :  
//              vImages: ���ڴ������׼ͼƬ��ͼƬ��
//				goldeImage: ���صĽ�׼ͼƬ
// Return Value:��
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
// Description: ʹ�þ�ֵ�˲��������׼ͼƬ
// Parameters :  
//               vImages:   ���ڴ������׼ͼƬ��ͼƬ��
//				 goldeImage: ���صĽ�׼ͼƬ
// Return Value: ��
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
// Description : �㷨��ʼ��
// Parameters : 
//				fPixelSizeRatio: "����-ʵ������ߴ�"�ı���
//				configPath: �����ļ�·��
//				outputPath: json���ݺ�ȱ��ͼƬ�ı���·�����������ΪNULL���򲻱���json���ݺ�ͼƬ
// Return Value :true - ��ʼ���ɹ�
//				 false - ��ȡ��ʼ��--------------------------------------
bool AlgoGoldenAnalysis::Init(int threadId, int imageWidth, int imageHeight, float fPixelSizeRatio, const char* configPath, const char* outputPath)
{
	// ����configuration�����ļ�
	if (!m_configManage.LoadConfigurationFile(configPath))	
	{
		cout << "ERROR: (Init) ����configuration�����ļ����� " << endl;
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
			m_SiteRect[y][x].height = imageHeight - 2*m_SiteRect[y][x].y;	//����ȱʡֵ

            m_AlignRegions[y][x].clear();
			m_AlignRegions[y][x].push_back(m_SiteRect[y][x]);   //ȱʡ��������
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
// Description:  ����site������򼰼�����
// Parameters :  
//               siteRect: �������
//               param:  ������
//               thresh: ��ֵ��
//				 nSiteX: site ˮƽλ��
//				 nSiteY: site ��ֱλ��
//
// Return Value: true - ���óɹ�
//				 false - ����ʧ��
// ------------------------------------------------------------
bool AlgoGoldenAnalysis::SetSiteRect(Rect_t siteRect, Param_t param, ThreshGroup_t thresh, int nSiteX, int nSiteY)
{
	if((nSiteX>=MAX_SITE_COLS) || (nSiteY>=MAX_SITE_ROWS))
	{
		cout << "ERROR: (SetSiteRect) site���������ֵ " << endl;
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
// Description:  ����goldenImage��threshGroup��������ÿ�����ص���ֵ
// Parameters :  
//				 threshGroup: ��ֵ��
//               goldenImage: ���׼ͼ��
//               highThreshMat:���ڽ��׼ͼ�����ֵ
//               lowThreshMat:���ڽ��׼ͼ�����ֵ
//
// Return Value: ��
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
    {//��ͬ������������ֵ�ָ�
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
// Description:  ��Ǵ���ͼ����goldenImageͼ����������ֵ������
// Parameters :  
//				 currImage: ����ͼ��
//               highThreshMat:���ڽ��׼ͼ�����ֵ
//               lowThreshMat:���ڽ��׼ͼ�����ֵ
//               resultImage: ����Ķ�ֵ���ͼ��
//
// Return Value: ��
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
// Description: ����ROI maskͼ��
// Parameters :  
//              jsonROI: ROI����json����
//				roiMask: ����֡ͼ���ϱ�ʾROI����Ч���� 
//				roiInfos: ÿ��ROI����ز��� 
// Return Value: ��
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
	roiMask = Mat::zeros(m_iImageHeight, m_iImageWidth, CV_8UC1);	//��ʼ����������ROI�����mask

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
			fillPoly( mask, ppt,  npt, 1, Scalar( 255), LINE_8 );   //���뻭2�Σ���Ϊ��ROI��Ƕ�Ŀ���
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
			circle(mask, pt_center, radius, Scalar(255), FILLED, LINE_8);   //���뻭2�Σ���Ϊ��ROI��Ƕ�Ŀ���

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
	if(roiNum < 2)  //ROI��Ŀ������1����������ǶROI������
		return;

	for(int i=0; i<roiNum; i++)
	{//������ܳ���ROI��Ƕ�����
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
// Description : ����Ring maskͼ��
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
		//��������ring����0���򲻴���ring��
		maskRingImage = brightRegion - brightRegion;
		return;
	}

	//��ɸѡ֮�����ͨ��ȥ��������
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

#if 0   //����д��opencv3.3�����
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
	
	//��ɸѡ֮�����ͨ��ȥ��������
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
// Description : ����RoI����
// Parameters :  jsonRoiSetting json��ʽRoI����
//				 nSiteX: site ˮƽλ��
//				 nSiteY: site ��ֱλ��
// Return Value: true - ���óɹ�
//				 false - ����ʧ��
// ------------------------------------------------------------
bool AlgoGoldenAnalysis::SetRoI(const char* jsonRoiSetting, int nSiteX, int nSiteY)
{
	if((nSiteX>=MAX_SITE_COLS) || (nSiteY>=MAX_SITE_ROWS))
	{
		cout << "ERROR: (SetRoI) site���������ֵ " << endl;
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
// Description:  ����json�����еĶ�������
// Parameters : 
//              jsonRegion: �����������������json����
//              regions: ����������Ķ�������
//				 
// Return Value: ��
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
// Description:  ���ö���������Χ���÷�Χ�����е�site��Ч
// Parameters :  
//				 alignOffX: ˮƽ������[-alignOffX, alignOffX]
//				 alignOffY: ��ֱ������[-alignOffY, alignOffY]
// Return Value: ��
//				 
// ------------------------------------------------------------
void AlgoGoldenAnalysis::SetAlignRange(int alignOffX, int alignOffY)
{
	m_AlignOffX = alignOffX;
	m_AlignOffY = alignOffY;
}
// ------------------------------------------------------------
// Description:  �������ڶ��������䣬��Ե�ǰ��site����
// Parameters :  
//				 jsonRegion: json���ݣ��������������Ŀ������
//				 nSiteX: site ˮƽλ��
//				 nSiteY: site ��ֱλ��
//
// Return Value: true - ���óɹ�
//				 false - ����ʧ��
// ------------------------------------------------------------
void AlgoGoldenAnalysis::SetAlignRegion(const char* jsonRegion, int nSiteX, int nSiteY)
{
	ExtractRegionJson(jsonRegion, m_AlignRegions[nSiteY][nSiteX]);
}
// ------------------------------------------------------------
// Description:  ���ý��׼ͼƬ��ͬʱ���ݽ��׼ͼ������ǰ���ͱ���ͼ��
// Parameters :  imageData ���׼ͼƬ����
//				 siteX ˮƽ����
//				 siteY ��ֱ����
// Return Value: true - ���óɹ�
//				 false - ����ʧ��
// ------------------------------------------------------------
bool AlgoGoldenAnalysis::SetGoldenSiteImage(unsigned char* imageData, int nSiteX, int nSiteY)
{
	if((nSiteX>=MAX_SITE_COLS) || (nSiteY>=MAX_SITE_ROWS))
	{
		cout << "ERROR: (SetGoldenSiteImage) site���������ֵ " << endl;
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
            lowMat  = lowMat  & roiIt->mask;//ROI�������ֵ

            Mat nonRoI = ~(roiIt->mask);
            Mat highThreshMat, lowThreshMat;
            highThreshMat = m_HighThreshMat[nSiteY][nSiteX](bound);
            lowThreshMat  = m_LowThreshMat[nSiteY][nSiteX](bound);

            highThreshMat = highThreshMat & nonRoI;
            lowThreshMat  = lowThreshMat  & nonRoI; //������ROI�������ֵ

            highThreshMat = highThreshMat | highMat;
            lowThreshMat  = lowThreshMat  | lowMat; //��ROI�������ֵ��ӽ���ֵ����
            
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
// Description:  ͳ��ͼ��ˮƽ��ֱͶӰ��Ϣ
// Parameters :  
//	            image: ����Ĵ�ͳ��ͼ��
//              horzProj: ����ͼ���ˮƽͶӰ
//              vertProj: ����ͼ��Ĵ�ֱͶӰ
//
// Return Value: true - ͳ�Ƴɹ�
//				 false - ͳ��ʧ��
// ------------------------------------------------------------
bool AlgoGoldenAnalysis::GrayProjections(const Mat& image, Mat& horzProj, Mat& vertProj)
{
	if((image.cols > vertProj.cols) || (image.rows > horzProj.cols))
	{
		cout << "ERROR: (GrayProjections) ͼ�����" << endl;
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
// Description:  MOSTACCURATE��ʽ������CPU��
// Parameters :  
//	            baseImage: ����Ķ�����׼ͼ��ָgoldenImage
//              alignImage: ������Ҫ������ͼ��ָ��ǰ�����ͼ��
//				nSiteX: site ˮƽλ��
//				nSiteY: site ��ֱλ��
//				offX: ˮƽ������[-offX, offX]
//				offY: ��ֱ������[-offY, offY]
//              fScore: ƥ��ֵ
//
// Return Value: ���ƥ��λ�õ�ƫ����
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
// Description: ��������ͼƬƫ�ƣ���OpenCV��CPUƥ�亯��
// Parameters :
//	            baseImage: ����Ķ�����׼ͼ��
//              alignImage: ������Ҫ������ͼ��
//				offX: ˮƽ������[-offX, offX]
//				offY: ��ֱ������[-offY, offY]
//              fScore: ƥ��ֵ
//
// Return Value: ���ƥ��λ�õ�ƫ����
// ------------------------------------------------------------
Point AlgoGoldenAnalysis::CalcAlignOffset_ocv_cpu(const Mat& baseImage, const Mat& alignImage, int offX, int offY, float &fScore)
{
	Rect  templRect;	//����ģ������
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
// Description: FASTEST��ʽ������CPU�棬ͶӰƥ���㷨
// Parameters :  
//              alignImage: ������Ҫ������ͼ��ָ��ǰ�����ͼ��
//				nSiteX: site ˮƽλ��
//				nSiteY: site ��ֱλ��
//              fScore: ƥ��ֵ
//
// Return Value: ���ƥ��λ�õ�ƫ����
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

	//ƥ������趨ΪX,Y�����������Сֵ
	fScore = min(dmax_coex, dmax_coey) * 100;

	Point off;
	off.x = dx - m_AlignOffX;
	off.y = dy - m_AlignOffY;
	return off;
}
// ------------------------------------------------------------
// Description: ����2�����������ϵ��
// Parameters :  
//              mat1: ��������1
//              mat2: ��������2
//				m2: ����2�ľ�ֵ
//				s2: ����2�ı�׼��
//              prodMat: ����ʱ��Ҫ���м�ֵ��������
//
// Return Value: ���ϵ��
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
// Description:  ���������ļ�����ȥ���޹�����
// Parameters :  ioRegionMat: ���������ֵ���������
//				 configManage: ���ò�������
//               contours: ���������ȱ��
//
// Return Value: true - ����ȱ��
//				 false - ��ȱ�ݼ���
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
			// ��С�����ù涨������������Ϊ0
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
		// ��С�����ù涨size���������Ϊ0
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
// Description:  ���㵱ǰͼ������׼ͼ���ƫ��, CPU��
// Parameters :  goldenImage ��ǰͼ��
//				 currImage ���׼ͼ��
//				 nSiteX: site ˮƽλ��
//				 nSiteY: site ��ֱλ��
//
// Return Value: ƫ������
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
// Description : ���㵱ǰͼ��Ķ�������
// Parameters :  
//               shiftLoc: ��ǰͼ�������goldenImage��ƫ��
//               siteRect: ��ǰͼ���site����
//               matchRect: ƫ�ƺ��site����
//				 
// Return Value: true - ����ɹ�
//				 false - ����ʧ��
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
		matchRect.width -= nDelta;       //��С�������
		siteRect.width = matchRect.width;
		matchRect.x = 0;
	}

	if(matchRect.y < 0)
	{
		int nDelta = -matchRect.y;
		matchRect.height -= nDelta;       //��С�����߶�
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
// Description:  ���ͼ��ȱ��, CPU��
// Parameters :  
//				 currImage: ����ͼ��
//				 siteX: ˮƽ����
//				 siteY: ��ֱ����
//               defectsContours: ������ȱ��
//
// Return Value: �����
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
//    morphologyEx(bufferMat, diffRegions, MORPH_OPEN, m_MorphElement); //diffRegions:����ȱ�ݣ�����ROI�ϵ�

	diffRegions = bufferMat;
    //==========================================
    Mat maskRg = rgMaskImage(siteRect);
    diffRegions = diffRegions & maskRg;
    //==========================================

    
    if(bUseRoI)
    {//��ROI
        Mat maskMat = m_MaskNonRoI[nSiteY][nSiteX](siteRect);
		diffMat = diffRegions & maskMat;    //����ROI�����ȱ��
		ret = RemoveIrrelevantRegions(diffMat, config, defectsContours);//��ȡ��ROI�����ϵ�ȱ��
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
    		bDefectFound = true;    //����ȱ��
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
            Mat roiRegion = m_DiffRegions(bound);   // ��ͬ��diffRegions
            roiBuffer = roiRegion & (roiIt->mask);  // ���η�ROI�����ȱ��
            
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
				bDefectFound = true;//����ȱ��
			}

            roiIt++;
        }
    }
    else
    {//��ROI
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
    		bDefectFound = true;    //����ȱ��
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
// Description : ���ͼ��ȱ��
// Parameters :  imageData ����ͼ������
//				 nDieId die���
//				 siteX ˮƽ����
//				 siteY ��ֱ����
// Return Value: �����
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
// Description:  ���ͼ��ȱ��,�ڲ�����ʹ��
// Parameters :  imageData: ����ͼ������
//				 nDieId: die���
//				 siteX: ˮƽ����
//				 siteY: ��ֱ����
//               outputResult: ���������
//               resultData: ��ʶȱ�ݵļ����ͼ
//
// Return Value: �����
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
// Description:  ��������࣬�ڼ��ͼƬ�ϱ�ʶ
// Parameters :  
//               defectsContours: ������ȱ������
//               outputResult: ȱ������
//               currImage: �����ͼ��
//               resultImage: ȱ�ݱ�ʶͼ��
//				 
// Return Value: true - ����ɹ�
//				 false - ����ʧ��
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
// Description:  ��������
// Parameters :  
//               outputResult: ȱ������	
//               resultMat: ȱ�ݱ�ʶͼ��
//				 nDieId: die���
//				 siteX: ˮƽ����
//				 siteY: ��ֱ����
//
// Return Value: ��
// ------------------------------------------------------------
void AlgoGoldenAnalysis::OutputResult(string& outputPath, vector<defectDescription<float> >& outputResult, Mat& resultMat, int nDieId, int nSiteX, int nSiteY)
{
	 // ����ͼ�������ļ�
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
	 // ����ͼ�������ļ�
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
