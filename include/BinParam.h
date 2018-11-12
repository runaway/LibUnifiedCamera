#pragma once

#include "ComDef.h"

// ����������
template<typename T>
class BinParamRange
{
public:
	inline BinParamRange() :
		lowerBound(0),
		upperBound(0)
	{
	}

	inline BinParamRange(T lower, T upper) :
		lowerBound(lower),
		upperBound(upper)
	{
	}

	// �Ƿ�Χ�������޶���Ϊ0
	inline bool IsRangeValid()
	{
		return (lowerBound >= 0 && upperBound > 0);
	}

	inline bool IsInRange(T data)
	{
		return (lowerBound <= data && upperBound >= data);
	}

	T lowerBound;		// ����
	T upperBound;		// ����
};

// ȱ�ݷ�����������
template<typename T>
class BinParam
{
public:

	inline BinParam(std::string binN,
		BinParamRange<T> area,
		BinParamRange<float> aspectRatio,
		BinParamRange<unsigned char> brightness,
		BinParamRange<T> size,
		std::string classN) :
		areaRange(area),
		aspectRatioRange(aspectRatio),
		brightnessRange(brightness),
		sizeRange(size),
		className(classN),
		binName(binN)
	{
	}

	inline BinParam() :
		areaRange(BinParamRange<T>(0, 0)),
		aspectRatioRange(BinParamRange<float>(0, 0)),
		brightnessRange(BinParamRange<unsigned char>(0, 0)),
		sizeRange(BinParamRange<T>(0, 0)),
		className(""),
		binName("")
	{

	}

	// ------------------------------------------------------------
	// Description : �жϵ�ǰȱ�ݲ����Ƿ�����bin������
	// Parameters :  bin: bin����
	//				 defectData: ȱ�ݲ���
	// Return Value :true - ����
	//				 false - ������
	// ------------------------------------------------------------
	inline bool MeetBinCriteria(DefectParam<T>& defectData)
	{
		// ��Χ������Ϊ0������Ϊ�����
		char mask = 0x0;	// �ڶ����������У���һλ��Ϊ���ʾ�����Ӧ�Ĳ�����Ҫ�ж�
		char res = 0x0;		// �ڶ����������У���һλ��Ϊ���ʾ�����Ӧ�Ĳ�������bin��

		if (areaRange.IsRangeValid())
		{
			mask |= 0x1;
			res |= areaRange.IsInRange(defectData.area) ? 0x1 : 0;
		}

		if (aspectRatioRange.IsRangeValid())
		{
			mask |= 0x2;
			res |= aspectRatioRange.IsInRange(defectData.aspectRatio) ? 0x2 : 0;
		}

		if (brightnessRange.IsRangeValid())
		{
			mask |= 0x4;
			res |= brightnessRange.IsInRange(defectData.brightness) ? 0x4 : 0;
		}

		if (sizeRange.IsRangeValid())
		{
			mask |= 0x8;
			res |= sizeRange.IsInRange(defectData.length) ? 0x8 : 0;
		}
		
		return res == mask;
	}

	BinParamRange<T> areaRange;						// ���
	BinParamRange<float> aspectRatioRange;			// �����
	BinParamRange<unsigned char> brightnessRange;	// ����
	BinParamRange<T> sizeRange;						// �ߴ�
	std::string className;							// ���������
	std::string binName;							// ��ǰbin������
};
