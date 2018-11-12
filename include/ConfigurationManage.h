#pragma once

#include <string>
#include "ComDef.h"


class ConfigurationManage
{
public:
	ConfigurationManage();
	
	// ------------------------------------------------------------
	// Description : ����Configuration�ļ�
	// Parameters :  path���ļ�·��
	// Return Value :true - �ɹ�
	//				 false - ʧ��
	// ------------------------------------------------------------
	bool LoadConfigurationFile(const std::string& configurationName);

	// ------------------------------------------------------------
	// Description : ����Configuration�ļ�
	// Parameters :  path���ļ�·��
	// Return Value :true - �ɹ�
	//				 false - ʧ��
	// ------------------------------------------------------------
	bool SaveConfigurationFile(const std::string& configurationName);


	std::string m_configurationName;		// �����ļ�����
	thresholdType m_thresholdType;			// ��ֵ����
	int m_manualThreshold;					// �˹��Ҷ���ֵ
	autoThresholdType m_autoThreshold;		// �Զ���ֵ
	float m_MinSize;						// �ߴ���ֵ
	float m_MinArea;						// �����ֵ
	float m_MinWidth;
	float m_MinLength;
	std::string m_strategyName;				// ��������
	bool m_bEnableClustering;				// �Ƿ����
	float m_clusterRadius;					// ����뾶
	clusterType m_clusterType;				// ���෽ʽ
	std::string m_ROIMaskName;				// ROIģ������
	std::string m_clusterStrategyName;		// Cluster Defect Binning Strategy Name
};
