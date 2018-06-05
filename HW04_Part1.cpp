// HW04_Part1.cpp
//
//
// The methods you needs to write are marked TODO
//
// Once you have written your methods, test them as follows:
// Use the < and > keys to translate the outline left and right
// Use the + and - keys to scale the outline larger and smaller
// q to quit
// spacebar to save an output image


#include "stdafx.h"

#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <vector>

using namespace cv;
using namespace std;

//Make the original a global variable for the sake of event handling
Mat originalImage;

//Required: Translate (move) an outline stored in a vector of points by adding an offset to 
//the coordinates of all points in the outline
void translateOutline(vector<Point>& outline, Point center);

//Required: Scale an outline by multiplying all coordinates of all points in the outline by a constant
void scaleOutline(vector<Point>& outline, double scale);

//Given: function to detect the largest red object in an image
bool findLargestRedObject(Mat& view, Point& location, vector<Point>& outline, int redThreshold);

//Given: Compute the area and center of a region bounded by an outline
void computeObjectAreaAndCenter(vector<Point>& outline, double& area, Point& center);

//Given: Draw an outline stored in a vector of points
void drawOutline(Mat& image, vector<Point>& outline);

//a dummy function to pass to the slider bar to threshold the red object
void onTrackbar(int value, void* data);

int main(int argc, char* argv[])
{
	//moving and scaling an outline

	if (argc <= 1)
	{
		cout << "Please provide a filename of an image" << endl;
		return 0;
	}

	originalImage = imread(argv[1]);
	Mat displayImage(originalImage.rows, originalImage.cols, CV_8UC3);
	originalImage.copyTo(displayImage);
	vector<Point> outline;
	int redThreshold = 190;

	namedWindow("Image Window", 1);
	createTrackbar("Red Threshold", "Image Window", &redThreshold, 255, onTrackbar, &outline);

	while (1 == 1)
	{
		originalImage.copyTo(displayImage);
		drawOutline(displayImage, outline);

		imshow("Image Window", displayImage);
		char key = waitKey(33);
		if (key == 'q')
		{
			break;
		}
		if (key == '>')
		{
			translateOutline(outline, Point(5, 0));
		}
		if (key == '<')
		{
			translateOutline(outline, Point(-5, 0));
		}
		if (key == '+')
		{
			Point tempCenter;
			double tempArea;
			computeObjectAreaAndCenter(outline, tempArea, tempCenter);
			scaleOutline(outline, 1.1);
		}
		if (key == '-')
		{
			Point tempCenter;
			double tempArea;
			computeObjectAreaAndCenter(outline, tempArea, tempCenter);

			scaleOutline(outline, 0.9); // this was 1.1, not sure why 
		}
		if (key == ' ')
		{
			imwrite("Part1_result.png", displayImage);
		}
	}

	return 0;
}

void drawOutline(Mat& image, vector<Point>& outline)
{
	int numPoints = outline.size() - 1;
	for (int f = 0; f<numPoints; f++)
	{
		line(image, outline[f], outline[f + 1], Scalar(255, 0, 0), 3);
	}
}

void translateOutline(vector<Point>& outline, Point center)
{
		// just add the center offset to each point to translate it
		for (int i = 0; i < outline.size() - 1; i++) {
			outline[i] += center;
		}
	
}


void scaleOutline(vector<Point>& outline, double scale)
{
	Point center; 
	double area;
	computeObjectAreaAndCenter(outline, area, center); // need old center for later
	// scale each vector 
	for (int i = 0; i < outline.size() - 1; i++) {
		outline[i] *= scale;
	}
	Point newCenter;
	computeObjectAreaAndCenter(outline, area, newCenter);
	// offset is the difference between the old center and the new one
	// add offset back into the new outline to center it back where it firsts was
	for (int i = 0; i < outline.size() - 1; i++) {
		outline[i] += (center - newCenter);
	}
}

// Need to overload on the type of the point
void computeObjectAreaAndCenter(vector<Point>& outline, double& area, Point& center)
{
	// http://docs.opencv.org/modules/imgproc/doc/structural_analysis_and_shape_descriptors.html
	Moments objectProperties;
	objectProperties = moments(outline, false);

	area = objectProperties.m00;
	center.x = (objectProperties.m10 / area);
	center.y = (objectProperties.m01 / area);
}


bool findLargestRedObject(Mat& view, Point& location, vector<Point>& outline, int redThreshold)
{
	//allocate some images to store intermediate results
	vector<Mat> YCrCb;
	YCrCb.push_back(Mat(view.rows, view.cols, CV_8UC3));
	vector<Mat> justRed;
	justRed.push_back(Mat(view.rows, view.cols, CV_8UC1));
	vector<Mat> displayRed;
	displayRed.push_back(Mat(view.rows, view.cols, CV_8UC3));

	//Switch color spaces to YCrCb so we can detect red objects even if they are dark
	cvtColor(view, YCrCb[0], CV_BGR2YCrCb);

	//Pull out just the red channel
	int extractRed[6] = { 1,0, 1, 1, 1, 2 };
	mixChannels(&(YCrCb[0]), 1, &(justRed[0]), 1, extractRed, 1);

	// Threshold the red object (with the threshold from the slider)
	threshold(justRed[0], justRed[0], redThreshold, 255, CV_THRESH_BINARY);
	vector<vector<Point>> objectContours;
	vector<Vec4i> dummy;

	//Find all of the contiguous image regions
	findContours(justRed[0], objectContours, dummy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

	//find the largest object
	int largestArea(-1), largestIndex(-1);
	Point largestCenter;
	for (int i = 0; i<objectContours.size(); i++)
	{
		Point tempCenter;
		double tempArea;
		computeObjectAreaAndCenter(objectContours[i], tempArea, tempCenter);

		if (tempArea > largestArea)
		{
			largestArea = tempArea;
			largestIndex = i;
			largestCenter = tempCenter;
		}
	}
	location = largestCenter;
	if (largestIndex >= 0)
	{
		outline = objectContours[largestIndex];
	}

	//Construct an image for display that shows the red channel as gray
	mixChannels(&(YCrCb[0]), 1, &(displayRed[0]), 1, extractRed, 3);
	if (largestIndex >= 0)
	{
		//put a red circle around the red object
		circle(displayRed[0], largestCenter, std::min(double(view.cols) / 2, sqrt(largestArea)), Scalar(0, 0, 255), 1);
	}
	imshow("Just Red", displayRed[0]);


	if (largestIndex >= 0)
	{
		return true;
	}
	else
	{
		return false;
	}

}

void onTrackbar(int redThreshold, void* data)
{
	Point largestCenter;
	vector<Point>* largestOutline = (vector<Point>*)(data);
	findLargestRedObject(originalImage, largestCenter, *largestOutline, redThreshold);
}