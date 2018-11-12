#include "stdafx.h"
#include "UnifiedCamera.h"
#include <opencv2/opencv.hpp>
#include <vector>

using namespace cv;
using namespace std;

VideoCapture g_VideoCapture;
bool g_bVideoCaptureEnable = false;
unsigned int g_nVideoWidth = 0;
unsigned int g_nVideoHeight= 0;

CascadeClassifier cascade, nestedCascade;
string cascadeName = "haarcascade_frontalface_alt.xml";
string nestedCascadeName = "haarcascade_eye_tree_eyeglasses.xml";

int DetectObjectInit();
int DetectObject(Mat& matImage);

bool VideoCameraOpen(unsigned int& nVideoWidth, unsigned int& nVideoHeight)
{
	Mat matSrc;
    g_VideoCapture.open(0);

	if (!g_VideoCapture.isOpened())
	{
		cerr << "ERROR! Unable to open camera\n";
        AfxMessageBox("ERROR! Unable to open camera");
		return false;
	}

	g_bVideoCaptureEnable = true;
	g_VideoCapture >> matSrc;

	if (matSrc.empty())
	{
		cerr << "ERROR! blank frame grabbed\n";
        AfxMessageBox("ERROR! blank frame grabbed");
		return false;
	}

    nVideoWidth = g_nVideoWidth = matSrc.cols;
    nVideoHeight = g_nVideoHeight = matSrc.rows;
	//imshow("matSrc", matSrc);

    DetectObjectInit();
    
	return true;
}

bool VideoCameraGrabFrame(unsigned char* pbyFrameData)
{
	Mat matSrc;

	if (false == g_bVideoCaptureEnable)
	{
		return false;
	}

	if (!g_VideoCapture.read(matSrc))
	{
		cerr << "ERROR! blank frame grabbed\n";
        AfxMessageBox("ERROR! blank frame grabbed");
		return false;
	}

    DetectObject(matSrc);
    
	memcpy(pbyFrameData, matSrc.ptr<uchar>(0), g_nVideoWidth * g_nVideoHeight * 3);
	//imshow("matSrc", matSrc);


	return true;
}

int DetectObjectInit()
{
    if ( !nestedCascade.load( nestedCascadeName ) )
    {
        cerr << "WARNING: Could not load classifier cascade for nested objects" << endl;
        AfxMessageBox("WARNING: Could not load classifier cascade for nested objects");
    }
    
    if( !cascade.load( cascadeName ) )
    {
        cerr << "ERROR: Could not load classifier cascade" << endl;
        AfxMessageBox("ERROR: Could not load classifier cascade");
        //help();
        return -1;
    }
}

void DetectAndDraw( Mat& img, CascadeClassifier& cascade,
                    CascadeClassifier& nestedCascade,
                    double scale, bool tryflip )
{
    double t = 0;
    vector<Rect> faces, faces2;
    const static Scalar colors[] =
    {
        Scalar(255,0,0),
        Scalar(255,128,0),
        Scalar(255,255,0),
        Scalar(0,255,0),
        Scalar(0,128,255),
        Scalar(0,255,255),
        Scalar(0,0,255),
        Scalar(255,0,255)
    };
    Mat gray, smallImg;

    cvtColor( img, gray, COLOR_BGR2GRAY );
    double fx = 1 / scale;
    resize( gray, smallImg, Size(), fx, fx, INTER_LINEAR_EXACT );
    equalizeHist( smallImg, smallImg );

    t = (double)getTickCount();
    cascade.detectMultiScale( smallImg, faces,
        1.1, 2, 0
        //|CASCADE_FIND_BIGGEST_OBJECT
        //|CASCADE_DO_ROUGH_SEARCH
        |CASCADE_SCALE_IMAGE,
        Size(30, 30) );
    if( tryflip )
    {
        flip(smallImg, smallImg, 1);
        cascade.detectMultiScale( smallImg, faces2,
                                 1.1, 2, 0
                                 //|CASCADE_FIND_BIGGEST_OBJECT
                                 //|CASCADE_DO_ROUGH_SEARCH
                                 |CASCADE_SCALE_IMAGE,
                                 Size(30, 30) );
        for( vector<Rect>::const_iterator r = faces2.begin(); r != faces2.end(); ++r )
        {
            faces.push_back(Rect(smallImg.cols - r->x - r->width, r->y, r->width, r->height));
        }
    }
    t = (double)getTickCount() - t;
    printf( "detection time = %g ms\n", t*1000/getTickFrequency());
    for ( size_t i = 0; i < faces.size(); i++ )
    {
        Rect r = faces[i];
        Mat smallImgROI;
        vector<Rect> nestedObjects;
        Point center;
        Scalar color = colors[i%8];
        int radius;

        double aspect_ratio = (double)r.width/r.height;
#if 0
        if( 0.75 < aspect_ratio && aspect_ratio < 1.3 )
        {
            center.x = cvRound((r.x + r.width*0.5)*scale);
            center.y = cvRound((r.y + r.height*0.5)*scale);
            radius = cvRound((r.width + r.height)*0.25*scale);
            circle( img, center, radius, color, 3, 8, 0 );
        }
        else
            rectangle( img, cvPoint(cvRound(r.x*scale), cvRound(r.y*scale)),
                       cvPoint(cvRound((r.x + r.width-1)*scale), cvRound((r.y + r.height-1)*scale)),
                       color, 3, 8, 0);
#endif        
        if( nestedCascade.empty() )
            continue;
        smallImgROI = smallImg( r );
        nestedCascade.detectMultiScale( smallImgROI, nestedObjects,
            1.1, 2, 0
            //|CASCADE_FIND_BIGGEST_OBJECT
            //|CASCADE_DO_ROUGH_SEARCH
            //|CASCADE_DO_CANNY_PRUNING
            |CASCADE_SCALE_IMAGE,
            Size(30, 30) );
        for ( size_t j = 0; j < nestedObjects.size(); j++ )
        {
            Rect nr = nestedObjects[j];
            center.x = cvRound((r.x + nr.x + nr.width*0.5)*scale);
            center.y = cvRound((r.y + nr.y + nr.height*0.5)*scale);
            radius = cvRound((nr.width + nr.height)*0.25*scale);
            circle( img, center, radius, color, 3, 8, 0 );
        }
    }
    //imshow( "result", img );
}


int DetectObject(Mat& matImage)
{
    double dScale = 1.0;
    bool bTryFlip = false;
    
    if (!matImage.empty())
    {
        DetectAndDraw(matImage, cascade, nestedCascade, dScale, bTryFlip);
        waitKey(0);
    }
    
	return true;
}
