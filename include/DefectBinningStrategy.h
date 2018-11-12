#pragma once

#include "ComDef.h"
#include "BinParam.h"
#include <list>
#include <string>

// �û��������ʵ������ߴ�Ϊ��λ��Strategy
typedef std::list< BinParam<float> > StrategyList;


class DefectBinningStrategy
{
public:
	DefectBinningStrategy();
	~DefectBinningStrategy();

	
	// ------------------------------------------------------------
	// Description : ���ز��������ļ�
	// Parameters :  strategyName��strategy����
	// Return Value :true - �ɹ�
	//				 false - ʧ��
	// ------------------------------------------------------------
	bool LoadStrategyFile(const std::string& strategyName);

	// ------------------------------------------------------------
	// Description : ��ȡ���������б�
	// Parameters :  ��
	// Return Value :���������б�
	// ------------------------------------------------------------
	StrategyList& GetStrategyList(void);

	// ------------------------------------------------------------
	// Description : ��ȡ���������б�
	// Parameters :  strategyName: Strategy ����
	//				 outputList: �����ֵ�����Բ�����
	// Return Value :���������б�
	// ------------------------------------------------------------
	bool GetStrategyList(const std::string& strategyName,
		StrategyList& outputList);

	// ------------------------------------------------------------
	// Description : ���浱ǰ�������õ��ļ�
	// Parameters :  path: �ļ�·��
	// Return Value :true - �ɹ�
	//				 false - ʧ��
	// ------------------------------------------------------------
	bool SaveStrategyToFile(char* path);

	// ------------------------------------------------------------
	// Description : ��һ��bin����strategy
	// Parameters :  binParam: bin����
	//				 nOrder: �����˳��Ĭ��ֵ-1��ʾ���뵽��ǰ
	// strategy�����
	// Return Value :true - �ɹ�
	//				 false - ʧ��
	// ------------------------------------------------------------
	bool AddBinToStrategy(const BinParam<float>& binParam, int nOrder = -1);
	
	// ------------------------------------------------------------
	// Description : ��strategyɾ��һ��bin
	// Parameters :  nID: ��ɾ��bin��strategy�е�λ��
	// Return Value :true - �ɹ�
	//				 false - ʧ��
	// ------------------------------------------------------------
	bool DeleteBinFromStrategy(unsigned int nID);

	// ------------------------------------------------------------
	// Description : �������ƴ�strategyɾ��bin
	// Parameters :  binName: ��ɾ����bin������
	// Return Value :
	// ------------------------------------------------------------
	bool DeleteBinFromStrategy(const std::string& binName);

private:
	StrategyList m_strategyList;	// ���������б�
};


