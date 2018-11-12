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
// Description : ���ز��������ļ�
// Parameters :  strategyName��strategy����
// Return Value :true - �ɹ�
//				 false - ʧ��
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

		// bin ����		
		TiXmlAttribute* elementAttribute = binItem->FirstAttribute();
		binParam.binName = elementAttribute->Value();

		// �����Χ		
		TiXmlElement* paramElement = binItem->FirstChildElement();
		elementAttribute = paramElement->FirstAttribute();
		binParam.areaRange.lowerBound = elementAttribute->DoubleValue();
		elementAttribute = elementAttribute->Next();
		binParam.areaRange.upperBound = elementAttribute->DoubleValue();
			
		// ����ȷ�Χ
		paramElement = paramElement->NextSiblingElement();
		elementAttribute = paramElement->FirstAttribute();
		binParam.aspectRatioRange.lowerBound = elementAttribute->DoubleValue();
		elementAttribute = elementAttribute->Next();
		binParam.aspectRatioRange.upperBound = elementAttribute->DoubleValue();

		// ���ȷ�Χ
		paramElement = paramElement->NextSiblingElement();
		elementAttribute = paramElement->FirstAttribute();
		binParam.brightnessRange.lowerBound = elementAttribute->DoubleValue();
		elementAttribute = elementAttribute->Next();
		binParam.brightnessRange.upperBound = elementAttribute->DoubleValue();

		// �ߴ緶Χ
		paramElement = paramElement->NextSiblingElement();
		elementAttribute = paramElement->FirstAttribute();
		binParam.sizeRange.lowerBound = elementAttribute->DoubleValue();
		elementAttribute = elementAttribute->Next();
		binParam.sizeRange.upperBound = elementAttribute->DoubleValue();

		// ������
		paramElement = paramElement->NextSiblingElement();
		elementAttribute = paramElement->FirstAttribute();
		binParam.className = elementAttribute->Value();
		
		m_strategyList.push_back(binParam);
	}
	return true;
}

// ------------------------------------------------------------
// Description : ��ȡ���������б�
// Parameters :  ��
// Return Value :���������б�
// ------------------------------------------------------------
StrategyList& DefectBinningStrategy::GetStrategyList(void)
{
	return m_strategyList;
}

// ------------------------------------------------------------
// Description : ��ȡ���������б�
// Parameters :  strategyName: Strategy ����
//				 outputList: �����ֵ�����Բ�����
// Return Value :���������б�
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
// Description : ���浱ǰ�������õ��ļ�
// Parameters :  path: �ļ�·��
// Return Value :true - �ɹ�
//				 false - ʧ��
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
// Description : ��һ��bin����strategy
// Parameters :  binParam: bin����
//				 nOrder: �����˳��Ĭ��ֵ-1��ʾ���뵽��ǰ
// strategy�����
// Return Value :true - �ɹ�
//				 false - ʧ��
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
// Description : ����������strategyɾ��һ��bin
// Parameters :  nID: ��ɾ��bin��strategy�е�λ��
// Return Value :true - �ɹ�
//				 false - ʧ��
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
// Description : �������ƴ�strategyɾ��bin
// Parameters :  binName: ��ɾ����bin������
// Return Value :true - �ɹ�
//				 false - ʧ��
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