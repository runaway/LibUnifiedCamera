#pragma once

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <vector>

class ParallelDiff : public cv::ParallelLoopBody
{
private:
	std::vector<cv::Mat>* difflistlocal;
	int* shiftxlistlocal;
	int* shiftylistlocal;
	int shiftlocal;
	cv::Mat immedianlocal;
	std::vector<cv::Mat>* imageslocal;
	cv::Size szlocal;
	int typelocal;
	cv::Rect validSiteRect;

public:
	ParallelDiff(
		std::vector<cv::Mat>* diffl,
		int* shiftxl, 
		int* shiftyl,
		int shiftl, 
		std::vector<cv::Mat>* imagesl, 
		cv::Mat immedianl,
		const cv::Rect& siteRect,
		cv::Size szl, 
		int typel):
		difflistlocal(diffl), 	
		shiftxlistlocal(shiftxl),
		shiftylistlocal(shiftyl),
		shiftlocal(shiftl),
		imageslocal(imagesl), 
		immedianlocal(immedianl),
		szlocal(szl),
		typelocal(typel),
		validSiteRect(siteRect)
	{}

	void operator()(const cv::Range &r) const
	{

		if (1) {

			cv::Mat out = cv::Mat::zeros(szlocal, typelocal);
			cv::Mat diff = cv::Mat::zeros(szlocal, typelocal);

			for (int i = r.start; i != r.end; ++i)
			{
				int sumtmp = std::numeric_limits<int>::max();

				for (int s1 = -shiftlocal; s1 <= shiftlocal; s1++)
					for (int s2 = -shiftlocal; s2 <= shiftlocal; s2++)
					{
						(*imageslocal)[i](cv::Rect(shiftlocal - s1, shiftlocal - s2,
							(*imageslocal)[i].cols - 2 * shiftlocal,
							(*imageslocal)[i].rows - 2 * shiftlocal)).copyTo(
								out(cv::Rect(shiftlocal, shiftlocal,
								(*imageslocal)[i].cols - 2 * shiftlocal,
									(*imageslocal)[i].rows - 2 * shiftlocal)));

						cv::absdiff(immedianlocal, out, diff);

						int sumc = cv::sum(diff(cv::Rect(shiftlocal, shiftlocal,
							(*imageslocal)[i].cols - 2 * shiftlocal,
							(*imageslocal)[i].rows - 2 * shiftlocal)))[0];
						if (sumc < sumtmp)
						{
							sumtmp = sumc;
							shiftxlistlocal[i] = s1;
							shiftylistlocal[i] = s2;
						}
					}

				(*imageslocal)[i](cv::Rect(shiftlocal - shiftxlistlocal[i],
					shiftlocal - shiftylistlocal[i],
					(*imageslocal)[i].cols - 2 * shiftlocal,
					(*imageslocal)[i].rows - 2 * shiftlocal)).copyTo(
						out(cv::Rect(shiftlocal, shiftlocal,
						(*imageslocal)[i].cols - 2 * shiftlocal,
							(*imageslocal)[i].rows - 2 * shiftlocal)));

				cv::absdiff(immedianlocal, out, diff);

				cv::Mat DiffRegions = cv::Mat::zeros((*imageslocal)[i].size(),
					(*imageslocal)[i].type());

				diff(validSiteRect).copyTo(DiffRegions(validSiteRect));
				(*difflistlocal)[i] = DiffRegions;

				std::cout << "******s1 = " << shiftxlistlocal[i] << ", s2 = "
					<< shiftylistlocal[i] << ", shiftlocal = " << shiftlocal << std::endl
					<< std::endl;
			}

		}



		if (0) {
			//四个角进行对准
			cv::Size szlocalcut;
			szlocalcut.width = szlocal.width / 4;
			szlocalcut.height = szlocal.height / 4;

			std::vector<cv::Mat> immedianlocalCut;// = cv::Mat::zeros(szlocalcut, typelocal);
			cv::Mat imageslocalCut = cv::Mat::zeros(szlocalcut, typelocal);
			cv::Mat out = cv::Mat::zeros(szlocalcut, typelocal);
			cv::Mat diff = cv::Mat::zeros(szlocalcut, typelocal);

			int x[4] = { 0 }, y[4] = { 0 };
			std::vector<cv::Rect> CutRect;
			CutRect.push_back(cvRect(0, 0, szlocalcut.width, szlocalcut.height));
			CutRect.push_back(cvRect(szlocal.width * 3.0 / 4, 0, szlocalcut.width, szlocalcut.height));
			CutRect.push_back(cvRect(szlocal.width * 3.0 / 4, szlocal.height * 3.0 / 4, szlocalcut.width, szlocalcut.height));
			CutRect.push_back(cvRect(0, szlocal.height * 3.0 / 4, szlocalcut.width, szlocalcut.height));
			for (int i = 0; i < CutRect.size(); i++) {
				cv::Mat tempCut = cv::Mat::zeros(szlocalcut, typelocal);
				immedianlocal(CutRect[i]).copyTo(tempCut);
				immedianlocalCut.push_back(tempCut);
			}

			cv::Mat outOrg = cv::Mat::zeros(szlocal, typelocal);
			cv::Mat diffOrg = cv::Mat::zeros(szlocal, typelocal);
			cv::Mat immedianlocalOrg = cv::Mat::zeros(szlocal, typelocal);
			immedianlocal(cv::Rect(shiftlocal, shiftlocal,
				immedianlocal.cols - 2 * shiftlocal,
				immedianlocal.rows - 2 * shiftlocal)).copyTo(
					immedianlocalOrg(cv::Rect(shiftlocal, shiftlocal,
						immedianlocal.cols - 2 * shiftlocal,
						immedianlocal.rows - 2 * shiftlocal)));


			for (int i = r.start; i != r.end; ++i)
			{
				for (int j = 0; j < CutRect.size(); j++) {
					(*imageslocal)[i](CutRect[j]).copyTo(imageslocalCut);
					int sumtmp = std::numeric_limits<int>::max();
					for (int s1 = -shiftlocal; s1 <= shiftlocal; s1++) {
						for (int s2 = -shiftlocal; s2 <= shiftlocal; s2++) {
							imageslocalCut(cv::Rect(shiftlocal - s1, shiftlocal - s2,
								imageslocalCut.cols - 2 * shiftlocal,
								imageslocalCut.rows - 2 * shiftlocal)).copyTo(
									out(cv::Rect(shiftlocal, shiftlocal,
										imageslocalCut.cols - 2 * shiftlocal,
										imageslocalCut.rows - 2 * shiftlocal)));

							cv::absdiff(immedianlocalCut[j], out, diff);
							int sumc = cv::sum(diff(cv::Rect(shiftlocal, shiftlocal,
								imageslocalCut.cols - 2 * shiftlocal,
								imageslocalCut.rows - 2 * shiftlocal)))[0];
							if (sumc < sumtmp)
							{
								sumtmp = sumc;
								x[j] = s1;
								y[j] = s2;
							}
						}
					}
				}

				shiftxlistlocal[i] = shiftlocal + (x[0] + x[1] + x[2] + x[3]) * 1.0 / 4 + 0.5;
				shiftylistlocal[i] = shiftlocal + (y[0] + y[1] + y[2] + y[3]) * 1.0 / 4 + 0.5;
				shiftxlistlocal[i] -= shiftlocal;
				shiftylistlocal[i] -= shiftlocal;
				(*imageslocal)[i](cv::Rect(shiftlocal - shiftxlistlocal[i], shiftlocal - shiftylistlocal[i],
					(*imageslocal)[i].cols - 2 * shiftlocal,
					(*imageslocal)[i].rows - 2 * shiftlocal)).copyTo(
						outOrg(cv::Rect(shiftlocal, shiftlocal,
						(*imageslocal)[i].cols - 2 * shiftlocal,
							(*imageslocal)[i].rows - 2 * shiftlocal)));
				cv::absdiff(immedianlocalOrg, outOrg, diffOrg);
				cv::Mat DiffRegions = cv::Mat::zeros((*imageslocal)[i].size(), (*imageslocal)[i].type());
				diffOrg(validSiteRect).copyTo(DiffRegions(validSiteRect));
				(*difflistlocal)[i] = DiffRegions;

 				std::cout << "******s1 = " << shiftxlistlocal[i] << ", s2 = " 
 					<< shiftylistlocal[i] << ", shiftlocal = " << shiftlocal << std::endl
 					<< "x = " << x[0] << " " << x[1] << " " << x[2] << " " << x[3] << std::endl
 					<< "y = " << y[0] << " " << y[1] << " " << y[2] << " " << y[3] << std::endl
 					<< std::endl;
			}
		}
	}

};

