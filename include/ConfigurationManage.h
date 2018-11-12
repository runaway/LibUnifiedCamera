#pragma once

#include <string>
#include "ComDef.h"


class ConfigurationManage
{
public:
	ConfigurationManage();
	
	// ------------------------------------------------------------
	// Description : 加载Configuration文件
	// Parameters :  path：文件路径
	// Return Value :true - 成功
	//				 false - 失败
	// ------------------------------------------------------------
	bool LoadConfigurationFile(const std::string& configurationName);

	// ------------------------------------------------------------
	// Description : 保存Configuration文件
	// Parameters :  path：文件路径
	// Return Value :true - 成功
	//				 false - 失败
	// ------------------------------------------------------------
	bool SaveConfigurationFile(const std::string& configurationName);


	std::string m_configurationName;		// 配置文件名称
	thresholdType m_thresholdType;			// 阈值类型
	int m_manualThreshold;					// 人工灰度阈值
	autoThresholdType m_autoThreshold;		// 自动阈值
	float m_MinSize;						// 尺寸阈值
	float m_MinArea;						// 面积阈值
	float m_MinWidth;
	float m_MinLength;
	std::string m_strategyName;				// 策略名称
	bool m_bEnableClustering;				// 是否聚类
	float m_clusterRadius;					// 聚类半径
	clusterType m_clusterType;				// 聚类方式
	std::string m_ROIMaskName;				// ROI模板名称
	std::string m_clusterStrategyName;		// Cluster Defect Binning Strategy Name
};
