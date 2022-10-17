/**
 * @file rootDetection.cpp
 * @author Johannes Fahr (johannes.fahr@posteo.de)
 * @brief Program for detection and measuríng plant roots in images.
 * @version 0.1
 * @date 2022-10-17
 * 
 * @copyright Copyright (c) 2022
 * 
 */
 
#include <iostream>
#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <cmath>

#define ZOOM_SIZE       2000
#define REF_VIEW_W      300
#define REF_VIEW_H      600
#define ZOOM_MOVE_STEP  100

/* define filter dimensions for bounding boxes here: */
#define ROOT_WIDTH_MAX  50
#define ROOT_HEIGHT_MIN 50
#define ROOT_HEIGHT_MAX 1000
 
using namespace std;
using namespace cv;

double takeReference(cv::Mat img);
void findRoots(cv::Mat & bin_img, cv::Mat & col_img, double scale, bool unicolor);

const Scalar GREEN = Scalar(0,255,0);
const Scalar COLOR_SET[] = { GREEN, 
                             Scalar(255,255,0), 
                             Scalar(0,255,255), 
                             Scalar(255,127,255) };
const int COLOR_SET_LEN = sizeof(COLOR_SET) / sizeof(Scalar);
 
int main() {
    Mat src_img = cv::imread("./data/10.10.202211-04-46_0-page-001.jpg", IMREAD_COLOR);
 
    Mat gray_img;
    cv::cvtColor(src_img, gray_img, COLOR_BGR2GRAY);

    Mat cropped_image = src_img(Range(0, REF_VIEW_H), Range(0, REF_VIEW_W));
    double scale = takeReference(cropped_image);
    
    cv::Mat bin_img;
    cv::Mat rgb_img = src_img.clone();
    int thd = 128;
    int key;
    bool zoom;
    bool unicolor = true;
    bool redraw = true;
    bool exit = false;
    cv::Range xRange = cv::Range(0, ZOOM_SIZE);
    cv::Range yRange = cv::Range(0, ZOOM_SIZE);
    int xMax = src_img.cols;
    int yMax = src_img.rows;

    cv::threshold(gray_img, bin_img, thd, 255, THRESH_BINARY_INV);

    cv::namedWindow("color", WINDOW_AUTOSIZE);
    cv::imshow("color", rgb_img);

    cv::namedWindow("bin", WINDOW_AUTOSIZE);
    cv::imshow("bin", bin_img);

    while (!exit) {
        if (redraw) {
            cv::threshold(gray_img, bin_img, thd, 255, THRESH_BINARY_INV);
            std::cout<<"Threshold:"<<thd<<endl;
            rgb_img.release();
            rgb_img = src_img.clone();

            findRoots(bin_img, rgb_img, scale, unicolor);
            redraw = false;
        }

        if (zoom) {
            imshow("bin", bin_img(yRange, xRange));
            imshow("color", rgb_img(yRange, xRange));
        } else {
            imshow("bin", bin_img);
            imshow("color", rgb_img);
        }

        switch (cv::waitKey(0)) {
        case 0:     /* up */
            if (thd > 10) thd -= 10;
            redraw = true;
            break;
        case 1:     /* down */
            if (thd < 245) thd += 10;
            redraw = true;
            break;
        case 2:     /* left */
            if (thd > 0) thd -= 1;
            redraw = true;
            break;
        case 3:     /* right */
            if (thd < 255) thd += 1;
            redraw = true;
            break;
        case 43:    /* + */
            zoom = true;
            break;
        case 45:    /* - */
            zoom = false;
            break;
        case 119:   /* w */
            if (yRange.start > ZOOM_MOVE_STEP) {
                yRange.start -= ZOOM_MOVE_STEP;
                yRange.end -= ZOOM_MOVE_STEP;
            } 
            break;
        case 97:    /* a */
            if (xRange.start > ZOOM_MOVE_STEP) {
                xRange.start -= ZOOM_MOVE_STEP;
                xRange.end -= ZOOM_MOVE_STEP;
            } 
            break;
        case 115:   /* s */
            if (yRange.end < yMax - ZOOM_MOVE_STEP) {
                yRange.start += ZOOM_MOVE_STEP;
                yRange.end += ZOOM_MOVE_STEP;
            }
            break;
        case 100:   /* d */
            if (xRange.end < xMax - ZOOM_MOVE_STEP) {
                xRange.start += ZOOM_MOVE_STEP;
                xRange.end += ZOOM_MOVE_STEP;
            } 
            break;
        case 99:    /* c */
            unicolor = !unicolor;
            redraw = true;
            break;
        case 13:    /* enter */
        case 27:    /* escape */
            exit = true;
            break;
        default:
            break;
        }
    }
}


double takeReference(cv::Mat img) {
    int x = 100, y = 100;
    bool firstMarkerSet;
    bool exit = false;
    cv::Point marker1, marker2;
    double factor = 1;
    cv::Mat wrk_img;
    
    while (!exit) {
        /* delete last image and redraw marker on clean image */
        wrk_img.release();
        wrk_img = img.clone();
        cv::drawMarker(wrk_img, Point(x, y), Scalar(0, 255, 255), 
            MARKER_CROSS, 50, 1);
        cv::imshow("bin", wrk_img);

        /* wait for user input to set marker */
        switch (cv::waitKey(0)) {
        case 0:     /* up */
            if (y > 0) y -= 1;
            break;
        case 1:     /* down */
            if (y < wrk_img.rows) y += 1;
            break;
        case 2:     /* left */
            if (x > 0) x -= 1;
            break;
        case 3:     /* right */
            if (x < wrk_img.cols) x += 1;
            break;
        case 13:    /* enter */
            if (!firstMarkerSet) {
                marker1 = Point(x, y);
                cv::drawMarker(img, marker1, Scalar(255, 255, 0), 
                    MARKER_TILTED_CROSS, 50, 1);
                firstMarkerSet = true;
            } else {
                marker2 = Point(x, y);
                cv::drawMarker(img, marker2, Scalar(255, 255, 0), 
                    MARKER_TILTED_CROSS, 50, 1);
                exit = true;
            }
            break;
        default:
            break;
        }
    }

    /* free recourses */
    img.release();

    /* compute reference factor */
    factor = norm(marker1 - marker2);
    std::cout << "Reference Factor: " << factor << " px/cm" << endl;

    return factor;
}


void findRoots(cv::Mat & bin_img, cv::Mat & col_img, double scale, bool unicolor) {
    cv::Scalar color;
    /* rotate whole image for vertical text */
    cv::rotate(bin_img, bin_img, ROTATE_90_COUNTERCLOCKWISE);
    cv::rotate(col_img, col_img, ROTATE_90_COUNTERCLOCKWISE);

    /* find contours on binary image */
    vector<vector<Point>> contours;
    cv::findContours(bin_img, contours, RETR_LIST, CHAIN_APPROX_NONE);

    double contour_len=0;
    cv::drawContours(bin_img,contours,-1,Scalar(255,125,0),1,8);

    for (int i=0; i < contours.size(); i++) {
        /* span recangle around contour */
        Rect rect = cv::boundingRect(contours[i]);
        double contourLenght = 0, rootLenght_cm;

        /* filter for root dimensions */ 
        if (rect.width < ROOT_HEIGHT_MAX && rect.width > ROOT_HEIGHT_MIN && rect.height < ROOT_WIDTH_MAX) {
            /* compute contour length */
            double x=contours.at(i).at(0).x;
            double y=contours.at(i).at(0).y;

            size_t count = contours[i].size();
            for(int ii=1; ii<count; ii++){
                if(contours.at(i).at(ii).x == x || contours.at(i).at(ii).y == y) {
                    contourLenght += 1;
                } else {
                    contourLenght += 1.41421356;
                }
                x = contours.at(i).at(ii).x;
                y = contours.at(i).at(ii).y;
            }

            /* calculate lenght of root in cm */
            rootLenght_cm = contourLenght / (2 * scale);

            /* choose new color if not unicolor mode */
            color = (unicolor) ? COLOR_SET[0] : COLOR_SET[i % COLOR_SET_LEN];

            /* annotate the images with bounding boxes and the length */
            cv::rectangle(bin_img, Point(rect.x,rect.y), 
                Point(rect.x+rect.width,rect.y+rect.height), 
                GREEN,2);
            cv::putText(bin_img, "Root: " + to_string(rootLenght_cm), 
                Point(rect.x+rect.width+10,rect.y+rect.height), 0, 1, 
                GREEN);
            cv::rectangle(col_img, Point(rect.x,rect.y), 
                Point(rect.x+rect.width,rect.y+rect.height), 
                color,2);
            cv::putText(col_img, "Root: " + to_string(rootLenght_cm), 
                Point(rect.x+rect.width+10,rect.y+rect.height), 0, 1, 
                color, 2);
        }
    }
    /* rotate images back to original direction */
    cv::rotate(bin_img, bin_img, ROTATE_90_CLOCKWISE);
    cv::rotate(col_img, col_img, ROTATE_90_CLOCKWISE);
}
