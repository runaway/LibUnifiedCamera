#include "stdafx.h"
#include "ConfigurationManage.h"
#include "tinyxml.h"
#include <iostream>

using namespace std;

ConfigurationManage::ConfigurationManage() :
	m_configurationName(""),		// 配置文件名称
	m_thresholdType(AUTO),			// 阈值类型
	m_manualThreshold(0),			// 人工阈值
	m_autoThreshold(MEDIUM),		// 自动阈值
	m_MinSize(0),				// 尺寸阈值
	m_MinArea(0),				// 面积阈值
	m_strategyName(""),				// 策略名称
	m_bEnableClustering(false),		// 是否聚类
	m_clusterRadius(0),				// 聚类半径
	m_clusterType(CLUSTER_NONE),	// 聚类方式
	m_ROIMaskName(""),				// ROI模板名称
	m_clusterStrategyName("")		// Cluster Defect Binning Strategy Name
{
}


// ------------------------------------------------------------
// Description : 加载Configuration文件
// Parameters :  path：文件路径
// Return Value :true - 成功
//				 false - 失败
// ------------------------------------------------------------
bool ConfigurationManage::LoadConfigurationFile(const string& configurationName)
{	
	string xmlPath = CONFIGURATION_FILE_PATH + configurationName + ".xml";

	TiXmlDocument doc(xmlPath.c_str());
	if (!doc.LoadFile())
	{			
		std::cout << "ConfigurationManage::LoadConfigurationFile() error message: " << doc.ErrorDesc() << endl;
		return false;
	}

	TiXmlElement* root = doc.RootElement();

	// configuration name
	TiXmlElement* configItem = root->FirstChildElement();	
	TiXmlAttribute* elementAttribute = configItem->FirstAttribute();
	m_configurationName = elementAttribute->Value();

	TiXmlElement* thesholdTypeElement = configItem->FirstChildElement();
	elementAttribute = thesholdTypeElement->FirstAttribute();
	m_thresholdType = (thresholdType)elementAttribute->IntValue();

	elementAttribute = elementAttribute->Next();
	m_manualThreshold = elementAttribute->DoubleValue();

	elementAttribute = elementAttribute->Next();
	m_autoThreshold = (autoThresholdType)elementAttribute->IntValue();

	TiXmlElement* filterElement = thesholdTypeElement->NextSiblingElement();
	elementAttribute = filterElement->FirstAttribute();
	m_MinSize = elementAttribute->DoubleValue();

	elementAttribute = elementAttribute->Next();
	m_MinArea = elementAttribute->DoubleValue();

	TiXmlElement* strategyElement = filterElement->NextSiblingElement();
	elementAttribute = strategyElement->FirstAttribute();
	m_strategyName = elementAttribute->Value();

	TiXmlElement* clusteringElement = strategyElement->NextSiblingElement();
	elementAttribute = clusteringElement->FirstAttribute();
	m_bEnableClustering = elementAttribute->IntValue();

	elementAttribute = elementAttribute->Next();
	m_clusterRadius = elementAttribute->DoubleValue();

	//TiXmlElement* reportElement = clusteringElement->NextSiblingElement();
	TiXmlElement* reportElement = clusteringElement->FirstChildElement();
	elementAttribute = reportElement->FirstAttribute();
	m_clusterType = (clusterType)elementAttribute->IntValue();

	elementAttribute = elementAttribute->Next();
	m_ROIMaskName = elementAttribute->Value();

	elementAttribute = elementAttribute->Next();
	m_clusterStrategyName = elementAttribute->Value();

	return true;
}

// ------------------------------------------------------------
// Description : 保存Configuration文件
// Parameters :  path：文件路径
// Return Value :true - 成功
//				 false - 失败
// ------------------------------------------------------------
bool ConfigurationManage::SaveConfigurationFile(const std::string& configurationName)
{	
	string savePath = CONFIGURATION_FILE_PATH + configurationName + ".xml";

	TiXmlDocument* doc = new TiXmlDocument();
	TiXmlDeclaration *pDeclaration = new TiXmlDeclaration("1.0",
		"UTF-8", "");
	doc->LinkEndChild(pDeclaration);

	TiXmlElement* rootLv1 = new TiXmlElement("root");
	doc->LinkEndChild(rootLv1);

	TiXmlElement* rootLv2 = new TiXmlElement("configuration");
	rootLv1->LinkEndChild(rootLv2);
	rootLv2->SetAttribute("name", m_configurationName.c_str());

	TiXmlElement* thesholdTypeElement = new TiXmlElement("threshold");
	rootLv2->LinkEndChild(thesholdTypeElement);
	thesholdTypeElement->SetAttribute("type", m_thresholdType);
	thesholdTypeElement->SetDoubleAttribute("manual_threshold", m_manualThreshold);
	thesholdTypeElement->SetAttribute("auto_threshold", m_autoThreshold);

	TiXmlElement* filterElement = new TiXmlElement("filter");
	rootLv2->LinkEndChild(filterElement);
	filterElement->SetDoubleAttribute("size_filter", m_MinSize);
	filterElement->SetDoubleAttribute("area_filter", m_MinArea);

	std::cout << "m_MinSize = " <<  m_MinSize << std::endl;
	std::cout << "m_MinArea = " <<  m_MinArea << std::endl;

	TiXmlElement* strategyElement = new TiXmlElement("strategy");
	rootLv2->LinkEndChild(strategyElement);
	strategyElement->SetAttribute("name", m_strategyName.c_str());

	TiXmlElement* clusteringElement = new TiXmlElement("clustering");
	rootLv2->LinkEndChild(clusteringElement);
	clusteringElement->SetAttribute("enable", m_bEnableClustering);
	clusteringElement->SetDoubleAttribute("radius", m_clusterRadius);

	TiXmlElement* reportElement = new TiXmlElement("report");
	clusteringElement->LinkEndChild(reportElement);
	reportElement->SetAttribute("type", m_clusterType);
	reportElement->SetAttribute("ROI_mask_name", m_ROIMaskName.c_str());
	reportElement->SetAttribute("strategy_name", m_clusterStrategyName.c_str());

	doc->SaveFile(savePath.c_str());

	return true;
}