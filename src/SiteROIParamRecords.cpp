#include "stdafx.h"
#include "SiteROIParamRecords.h"
#include "tinyxml.h"
#include "tinystr.h"
#include <string.h>

using namespace std;
using namespace cv;

SiteROIParamRecords::SiteROIParamRecords(int nImgWid, int nImgHei)
{
	m_ROIRegion = Mat::zeros(nImgHei, nImgWid, CV_8UC1);
	m_SiteRect = Rect(0, 0, 0, 0);
	m_ROINameShapeMap.insert(make_pair(string("rectangle"), ROI_RECTANGLE));
	m_ROINameShapeMap.insert(make_pair(string("circle"), ROI_CIRCLE));
	m_ROINameShapeMap.insert(make_pair(string("polygon"), ROI_FREEHAND_LINE));
}

SiteROIParamRecords::~SiteROIParamRecords()
{
	m_paramMap.clear();
}

// ------------------------------------------------------------
// Description : 设置site的区域
// Parameters :  siteRect: site的区域
// Return Value :true - siteRect参数有效
//				 false - siteRect参数无效
// ------------------------------------------------------------
bool SiteROIParamRecords::SetSiteRegion(const cv::Rect& siteRect)
{
	if (siteRect.x < 0 || siteRect.y < 0 || siteRect.width <= 0 
		|| siteRect.height)
	{
		return false;
	}
	m_SiteRect = siteRect;
	return true;
}

// ------------------------------------------------------------
// Description : 向map中增加一个新的参数记录
// Parameters :  nIdx: 记录索引
//				 UserParam: 需增加的参数记录
// Return Value :1: map中没有nIdx的记录，增加成功
//				 0: map中有nIdx的记录，修改记录
// ------------------------------------------------------------
int SiteROIParamRecords::InsertNewRecord(int nIdx, ROIParam param)
{
	ROIParamMap::iterator it = m_paramMap.find(nIdx);
	if (it == m_paramMap.end())
	{
		m_paramMap.insert(make_pair(nIdx, param));
		return 1;
	}
	else
	{
		it->second = param;
		return 0;
	}
}

// ------------------------------------------------------------
// Description : 保存参数到xml文件
// Parameters :  path: 参数保存路径
// Return Value :true - 保存成功
//				 false - 保存失败
// ------------------------------------------------------------
bool SiteROIParamRecords::SaveRecordsToFile(char* path)
{
	if (m_paramMap.empty())
	{
		return false;
	}

	string xmlPath(path);
	xmlPath += "SiteROIParamRecords.xml";
		
	TiXmlDocument *doc = new TiXmlDocument();

	TiXmlDeclaration *pDeclaration = new TiXmlDeclaration("1.0",
		"UTF-8", "");
	doc->LinkEndChild(pDeclaration);

	TiXmlElement *RootLv1 = new TiXmlElement("die");
	doc->LinkEndChild(RootLv1);

	TiXmlElement *RootLv2 = new TiXmlElement("site");
	RootLv1->LinkEndChild(RootLv2);
	RootLv2->SetAttribute("id", 1);
	RootLv2->SetAttribute("locationX", m_SiteRect.x);
	RootLv2->SetAttribute("locationY", m_SiteRect.y);
	RootLv2->SetAttribute("width", m_SiteRect.width);
	RootLv2->SetAttribute("height", m_SiteRect.height);

	ROIParamMap::iterator it = m_paramMap.begin();
	for (; it != m_paramMap.end(); ++it)
	{
		TiXmlElement *RootLv3 = new TiXmlElement("roi");
		RootLv2->LinkEndChild(RootLv3);
		string strIndex = to_string(it->first);
		RootLv3->SetAttribute("index", strIndex.c_str());
		
		TiXmlElement *xmlconfigName = new TiXmlElement("configName");
		RootLv3->LinkEndChild(xmlconfigName);
		TiXmlText *valconfigName = new TiXmlText(it->second.configName.c_str());
		xmlconfigName->LinkEndChild(valconfigName);

		TiXmlElement *xmlROIShape = new TiXmlElement("shape");
		RootLv3->LinkEndChild(xmlROIShape);
		string strROIShape = to_string(it->second.shape);
		TiXmlText *valROIShape = new TiXmlText(strROIShape.c_str());
		xmlROIShape->LinkEndChild(valROIShape);
		
		if (it->second.shape == ROI_RECTANGLE)
		{
			TiXmlElement *xmlLocation = new TiXmlElement("location");
			RootLv3->LinkEndChild(xmlLocation);
			xmlLocation->SetAttribute("x", it->second.data.rect.x);
			xmlLocation->SetAttribute("y", it->second.data.rect.y);
			
			TiXmlElement *xmlWidth = new TiXmlElement("width");
			RootLv3->LinkEndChild(xmlWidth);
			string strWidth = to_string(it->second.data.rect.width);
			TiXmlText *valWidth = new TiXmlText(strWidth.c_str());
			xmlWidth->LinkEndChild(valWidth);

			TiXmlElement *xmlHeight = new TiXmlElement("height");
			RootLv3->LinkEndChild(xmlHeight);
			string strHeight = to_string(it->second.data.rect.height);
			TiXmlText *valHeight = new TiXmlText(strHeight.c_str());
			xmlHeight->LinkEndChild(valHeight);
		}
		else if (it->second.shape == ROI_CIRCLE)
		{
			TiXmlElement *xmlCenter = new TiXmlElement("center");
			RootLv3->LinkEndChild(xmlCenter);
			xmlCenter->SetAttribute("x", it->second.data.circle.x);
			xmlCenter->SetAttribute("y", it->second.data.circle.y);

			TiXmlElement *xmlRadius = new TiXmlElement("radius");
			RootLv3->LinkEndChild(xmlRadius);
			string strRadius = to_string(it->second.data.circle.radius);
			TiXmlText *valRadius = new TiXmlText(strRadius.c_str());
			xmlRadius->LinkEndChild(valRadius);
		}
		else if (it->second.shape == ROI_FREEHAND_LINE)
		{
			vector<cv::Point>::iterator contourIt = (it->second).data.contour.begin();
			for (; contourIt != (it->second).data.contour.end(); ++contourIt)
			{
				TiXmlElement *xmlPoint = new TiXmlElement("point");
				RootLv3->LinkEndChild(xmlPoint);
				xmlPoint->SetAttribute("x", contourIt->x);
				xmlPoint->SetAttribute("y", contourIt->y);
			}
		}
	}

	doc->SaveFile(xmlPath.c_str());	

	return true;
}

// ------------------------------------------------------------
// Description : 加载配置文件
// Parameters :  filePath: 配置文件路径
//				 nSiteID: site ID
// Return Value :true - 成功
//				 false - 失败
// ------------------------------------------------------------
bool SiteROIParamRecords::LoadConfigFile(char* filePath, int nSiteID)
{
	TiXmlDocument doc(filePath);
	if (!doc.LoadFile())
	{
		std::cout << "DefectBinningStrategy::LoadStrategyFile() load "
			<< filePath << " fail!" << std::endl;
		std::cout << "error message: " << doc.ErrorDesc() << std::endl;
		return false;
	}

	TiXmlElement* root = doc.RootElement();

	if (NULL == root)
	{
		std::cout << filePath << " root is NULL!!" << std::endl;
		return false;
	}

	TiXmlElement *siteElement = root->FirstChildElement();
	for (; siteElement;	siteElement = siteElement->NextSiblingElement())
	{
		TiXmlAttribute* elementAttribute = siteElement->FirstAttribute();
		if (nSiteID != elementAttribute->IntValue())
		{
			continue;
		}
	}
	if (!siteElement)
	{
		// 没有找到ID为nSiteID的记录
		return false;
	}
	
	TiXmlAttribute* siteAttribute = siteElement->FirstAttribute();
	m_SiteRect.x = siteAttribute->IntValue();
	siteAttribute = siteAttribute->Next();
	m_SiteRect.y = siteAttribute->IntValue();
	siteAttribute = siteAttribute->Next();
	m_SiteRect.width = siteAttribute->IntValue();
	siteAttribute = siteAttribute->Next();
	m_SiteRect.height = siteAttribute->IntValue();
	
	m_paramMap.clear();

	for (TiXmlElement* ROIItem = siteElement->FirstChildElement(); ROIItem;
		ROIItem = ROIItem->NextSiblingElement())
	{
		TiXmlElement* roiLv = ROIItem->FirstChildElement();
		ROIParam roiParam;
		int roiIdx;
		TiXmlAttribute* roiAttribute = roiLv->FirstAttribute();
		roiIdx = roiAttribute->IntValue();

		TiXmlElement *xmlconfigName = roiLv->FirstChildElement();
		TiXmlAttribute* configNameAttribute = xmlconfigName->FirstAttribute();
		roiParam.configName = configNameAttribute->Value();

		TiXmlElement *xmlROIShape = xmlconfigName->NextSiblingElement();
		TiXmlAttribute* shapeAttribute = xmlROIShape->FirstAttribute();
		roiParam.shape = m_ROINameShapeMap[shapeAttribute->Value()];
		
		switch (roiParam.shape)
		{
		case ROI_RECTANGLE:
		{
			TiXmlElement *xmlLocation = xmlROIShape->NextSiblingElement();
			TiXmlAttribute* locationAttribute = xmlLocation->FirstAttribute();
			roiParam.data.rect.x = locationAttribute->IntValue();
			locationAttribute = locationAttribute->Next();
			roiParam.data.rect.y = locationAttribute->IntValue();

			TiXmlElement *xmlWidth = xmlLocation->NextSiblingElement();
			TiXmlAttribute* widthAttribute = xmlWidth->FirstAttribute();
			roiParam.data.rect.width = widthAttribute->IntValue();

			TiXmlElement *xmlHeight = xmlWidth->NextSiblingElement();
			TiXmlAttribute* heightAttribute = xmlHeight->FirstAttribute();
			roiParam.data.rect.height = heightAttribute->IntValue();
			
			break;
		}
		case ROI_CIRCLE:
		{
			TiXmlElement *xmlCenter = xmlROIShape->NextSiblingElement();			
			TiXmlAttribute* centerAttribute = xmlCenter->FirstAttribute();
			roiParam.data.circle.x = centerAttribute->IntValue();
			centerAttribute = centerAttribute->Next();
			roiParam.data.circle.y = centerAttribute->IntValue();

			TiXmlElement *xmlRadius = xmlCenter->NextSiblingElement();
			TiXmlAttribute* radiusAttribute = xmlRadius->FirstAttribute();
			roiParam.data.circle.radius = radiusAttribute->IntValue();
			break;
		}
		case ROI_FREEHAND_LINE:
		{	
			roiParam.data.contour.clear();
			TiXmlElement *pointElement = xmlROIShape->NextSiblingElement();
			for (; pointElement; pointElement = pointElement->NextSiblingElement())
			{
				Point contourPoint;
				TiXmlAttribute* contourAttribute = pointElement->FirstAttribute();
				contourPoint.x = contourAttribute->IntValue();
				contourAttribute = contourAttribute->Next();
				contourPoint.y = contourAttribute->IntValue();
				roiParam.data.contour.push_back(contourPoint);
			}
			
			break;
		}
		default:
			return false;
		}
		m_paramMap.insert(make_pair(roiIdx, roiParam));
	}
	return true;
}


// ------------------------------------------------------------
// Description : 删除map中的一条记录
// Parameters :  ind：待删除的记录的索引
// Return Value :true: map中有nIdx的记录，删除成功
//				 false: map中没有nIdx的记录，删除失败
// ------------------------------------------------------------
bool SiteROIParamRecords::DeleteRecord(int nIdx)
{
	ROIParamMap::iterator it = m_paramMap.find(nIdx);
	if (it != m_paramMap.end())
	{
		m_paramMap.erase(it);
		return true;
	}
	return false;
}

// ------------------------------------------------------------
// Description : 获取ROI region Mat
// Parameters :  无
// Return Value :ROI region Mat
// ------------------------------------------------------------
bool SiteROIParamRecords::GetROIRegionMat(cv::Mat& roiMat)
{
	if (m_ROIRegion.empty())
	{
		return false;
	}
	roiMat = m_ROIRegion;
	return true;
}

// ------------------------------------------------------------
// Description :  根据配置文件创建ROI
// Parameters :   无
// Return Value : true - 创建成功
//				  false - 未加载配置文件
// ------------------------------------------------------------
bool SiteROIParamRecords::CreateROIMat()
{
	if (m_paramMap.empty())
	{
		return false;
	}
}
