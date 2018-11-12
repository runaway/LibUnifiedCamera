#include "stdafx.h"
#include "DefectBinningStrategy.h"
#include "tinyxml.h"
#include <iostream>
#include <Windows.h>

using namespace std;

DefectBinningStrategy::DefectBinningStrategy()
{
}

DefectBinningStrategy::~DefectBinningStrategy()
{
}

// ------------------------------------------------------------
// Description : 加载策略配置文件
// Parameters :  strategyName：strategy名称
// Return Value :true - 成功
//				 false - 失败
// ------------------------------------------------------------
bool DefectBinningStrategy::LoadStrategyFile(const string& strategyName)
{
	string xmlPath = STRATEGY_FILE_PATH + strategyName + ".xml";

	TiXmlDocument doc(xmlPath.c_str());
	if (!doc.LoadFile())    
	{
		std::cout << "DefectBinningStrategy::LoadStrategyFile() load " 
			<< xmlPath << " fail!" << std::endl;
		std::cout << "error message: " << doc.ErrorDesc() << std::endl;
		return false;
	}
		
	m_strategyList.clear();

	TiXmlElement* root = doc.RootElement();

	if (NULL == root)
	{
		std::cout << xmlPath << " root is NULL!!" << std::endl;
		return false;
	}

		
	for (TiXmlElement* binItem = root->FirstChildElement(); binItem;
		binItem = binItem->NextSiblingElement())
	{	
		BinParam<float> binParam;

		// bin 名称		
		TiXmlAttribute* elementAttribute = binItem->FirstAttribute();
		binParam.binName = elementAttribute->Value();

		// 面积范围		
		TiXmlElement* paramElement = binItem->FirstChildElement();
		elementAttribute = paramElement->FirstAttribute();
		binParam.areaRange.lowerBound = elementAttribute->DoubleValue();
		elementAttribute = elementAttribute->Next();
		binParam.areaRange.upperBound = elementAttribute->DoubleValue();
			
		// 长宽比范围
		paramElement = paramElement->NextSiblingElement();
		elementAttribute = paramElement->FirstAttribute();
		binParam.aspectRatioRange.lowerBound = elementAttribute->DoubleValue();
		elementAttribute = elementAttribute->Next();
		binParam.aspectRatioRange.upperBound = elementAttribute->DoubleValue();

		// 亮度范围
		paramElement = paramElement->NextSiblingElement();
		elementAttribute = paramElement->FirstAttribute();
		binParam.brightnessRange.lowerBound = elementAttribute->DoubleValue();
		elementAttribute = elementAttribute->Next();
		binParam.brightnessRange.upperBound = elementAttribute->DoubleValue();

		// 尺寸范围
		paramElement = paramElement->NextSiblingElement();
		elementAttribute = paramElement->FirstAttribute();
		binParam.sizeRange.lowerBound = elementAttribute->DoubleValue();
		elementAttribute = elementAttribute->Next();
		binParam.sizeRange.upperBound = elementAttribute->DoubleValue();

		// 分类名
		paramElement = paramElement->NextSiblingElement();
		elementAttribute = paramElement->FirstAttribute();
		binParam.className = elementAttribute->Value();
		
		m_strategyList.push_back(binParam);
	}
	return true;
}

// ------------------------------------------------------------
// Description : 获取策略配置列表
// Parameters :  无
// Return Value :策略配置列表
// ------------------------------------------------------------
StrategyList& DefectBinningStrategy::GetStrategyList(void)
{
	return m_strategyList;
}

// ------------------------------------------------------------
// Description : 获取策略配置列表
// Parameters :  strategyName: Strategy 名称
//				 outputList: 【输出值】策略参数表
// Return Value :策略配置列表
// ------------------------------------------------------------
bool DefectBinningStrategy::GetStrategyList(const string& strategyName,
	StrategyList& outputList)
{	
	if (!LoadStrategyFile(strategyName))
	{
		return false;
	}
	outputList = m_strategyList;
	return true;
}

// ------------------------------------------------------------
// Description : 保存当前策略配置到文件
// Parameters :  path: 文件路径
// Return Value :true - 成功
//				 false - 失败
// ------------------------------------------------------------
bool DefectBinningStrategy::SaveStrategyToFile(char* path)
{
	if (m_strategyList.empty())
	{
		return false;
	}
	string xmlPath(path);
	string savePath = STRATEGY_FILE_PATH + xmlPath + ".xml";

	TiXmlDocument* doc = new TiXmlDocument();
	TiXmlDeclaration *pDeclaration = new TiXmlDeclaration("1.0",
		"UTF-8", "");
	doc->LinkEndChild(pDeclaration);

	TiXmlElement* rootLv1 = new TiXmlElement("root");
	doc->LinkEndChild(rootLv1);

	StrategyList::iterator it = m_strategyList.begin();
	while (it != m_strategyList.end())
	{		
		TiXmlElement* rootLv2 = new TiXmlElement("bin");
		rootLv1->LinkEndChild(rootLv2);	
		rootLv2->SetAttribute("name", it->binName.c_str());

		TiXmlElement* areaRange = new TiXmlElement("areaRange");
		rootLv2->LinkEndChild(areaRange);
		areaRange->SetDoubleAttribute("lowerBound", it->areaRange.lowerBound);
		areaRange->SetDoubleAttribute("upperBound", it->areaRange.upperBound);
	
		TiXmlElement* aspectRatioRange = new TiXmlElement("aspectRatioRange");
		rootLv2->LinkEndChild(aspectRatioRange);
		aspectRatioRange->SetDoubleAttribute("lowerBound", it->aspectRatioRange.lowerBound);
		aspectRatioRange->SetDoubleAttribute("upperBound", it->aspectRatioRange.upperBound);

		TiXmlElement* brightnessRange = new TiXmlElement("brightnessRange");
		rootLv2->LinkEndChild(brightnessRange);
		brightnessRange->SetDoubleAttribute("lowerBound", it->brightnessRange.lowerBound);
		brightnessRange->SetDoubleAttribute("upperBound", it->brightnessRange.upperBound);

		TiXmlElement* sizeRange = new TiXmlElement("sizeRange");
		rootLv2->LinkEndChild(sizeRange);
		sizeRange->SetDoubleAttribute("lowerBound", it->sizeRange.lowerBound);
		sizeRange->SetDoubleAttribute("upperBound", it->sizeRange.upperBound);

		TiXmlElement* className = new TiXmlElement("className");
		rootLv2->LinkEndChild(className);
		className->SetAttribute("className", it->className.c_str());

		++it;
	}

	doc->SaveFile(savePath.c_str());
	return true;
}

// ------------------------------------------------------------
// Description : 将一个bin加入strategy
// Parameters :  binParam: bin参数
//				 nOrder: 加入的顺序。默认值-1表示加入到当前
// strategy的最后
// Return Value :true - 成功
//				 false - 失败
// ------------------------------------------------------------
bool DefectBinningStrategy::AddBinToStrategy(const BinParam<float>& binParam, 
	int nOrder)
{
	if (-1 == nOrder)
	{
		m_strategyList.push_back(binParam);
		return true;
	}
	
	if (nOrder > m_strategyList.size())
	{
		return false;
	}
	
	m_strategyList.insert(m_strategyList.begin(), nOrder, binParam);
	return true;
}

// ------------------------------------------------------------
// Description : 按照索引从strategy删除一个bin
// Parameters :  nID: 待删除bin在strategy中的位置
// Return Value :true - 成功
//				 false - 失败
// ------------------------------------------------------------
bool DefectBinningStrategy::DeleteBinFromStrategy(unsigned int nID)
{
	if (m_strategyList.empty() || nID > m_strategyList.size())
	{
		return false;
	}
	
	StrategyList::iterator it = m_strategyList.begin();
	advance(it, nID);
	if (it != m_strategyList.end())
	{
		m_strategyList.erase(it);
		return true;
	}
	return false;
}

// ------------------------------------------------------------
// Description : 按照名称从strategy删除bin
// Parameters :  binName: 待删除的bin的名称
// Return Value :true - 成功
//				 false - 失败
// ------------------------------------------------------------
bool DefectBinningStrategy::DeleteBinFromStrategy(const string& binName)
{
	if (m_strategyList.empty())
	{
		return false;
	}

	StrategyList::iterator it = m_strategyList.begin();
	while (it != m_strategyList.end())
	{
		if (binName == it->binName)
		{
			break;
		}
		++it;
	}

	if (it == m_strategyList.end())
	{
		return false;
	}

	m_strategyList.erase(it);
	return true;
}