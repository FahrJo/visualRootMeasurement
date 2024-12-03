/**
 * @file rootDetection.cpp
 * @author Johannes Fahr (johannes.fahr@posteo.de)
 * @brief Program for detection and measuring plant roots in images.
 * @version 0.1
 * @date 2022-10-17
 *
 * @copyright Copyright (c) 2022 Johannes Fahr
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <cmath>

constexpr int ZOOM_SIZE = 2000;
constexpr int REF_VIEW_W = 300;
constexpr int REF_VIEW_H = 600;
constexpr int ZOOM_MOVE_STEP = 100;

/* define filter dimensions for bounding boxes here: */
constexpr int ROOT_WIDTH_MAX = 50;
constexpr int ROOT_HEIGHT_MIN = 50;
constexpr int ROOT_HEIGHT_MAX = 1000;

double takeReference(cv::Mat img);
void findRoots(cv::Mat &bin_img, cv::Mat &col_img, double scale, bool unicolor);

const cv::Scalar GREEN = cv::Scalar(0, 255, 0);
const std::array<cv::Scalar, 4> COLOR_SET = {GREEN,
                                             cv::Scalar(255, 255, 0),
                                             cv::Scalar(0, 255, 255),
                                             cv::Scalar(255, 127, 255)};
const int COLOR_SET_LEN = sizeof(COLOR_SET) / sizeof(cv::Scalar);

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        std::cout << "Please specify an image path as command line argument!" << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "Copyright (C) 2022 Johannes Fahr" << std::endl;
    std::cout << "This program comes with ABSOLUTELY NO WARRANTY;" << std::endl;
    std::cout << "This is free software, and you are welcome to" << std::endl;
    std::cout << "redistribute it under certain conditions;" << std::endl;
    std::cout << std::endl;

    cv::Mat src_img = cv::imread(argv[1], cv::IMREAD_COLOR);

    cv::Mat gray_img;
    cv::cvtColor(src_img, gray_img, cv::COLOR_BGR2GRAY);

    cv::Mat cropped_image = src_img(cv::Range(0, REF_VIEW_H), cv::Range(0, REF_VIEW_W));
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

    cv::threshold(gray_img, bin_img, thd, 255, cv::THRESH_BINARY_INV);

    cv::namedWindow("color", cv::WINDOW_AUTOSIZE);
    cv::imshow("color", rgb_img);

    cv::namedWindow("bin", cv::WINDOW_AUTOSIZE);
    cv::imshow("bin", bin_img);

    while (!exit)
    {
        if (redraw)
        {
            cv::threshold(gray_img, bin_img, thd, 255, cv::THRESH_BINARY_INV);
            std::cout << "\r\e[KThreshold: " << std::setfill(' ') << std::setw(3) << thd << std::flush;
            rgb_img.release();
            rgb_img = src_img.clone();

            findRoots(bin_img, rgb_img, scale, unicolor);
            redraw = false;
        }

        if (zoom)
        {
            cv::imshow("bin", bin_img(yRange, xRange));
            cv::imshow("color", rgb_img(yRange, xRange));
        }
        else
        {
            cv::imshow("bin", bin_img);
            cv::imshow("color", rgb_img);
        }

        switch (cv::waitKey(0))
        {
        case 0: /* up */
            if (thd > 10)
                thd -= 10;
            redraw = true;
            break;
        case 1: /* down */
            if (thd < 245)
                thd += 10;
            redraw = true;
            break;
        case 2: /* left */
            if (thd > 0)
                thd -= 1;
            redraw = true;
            break;
        case 3: /* right */
            if (thd < 255)
                thd += 1;
            redraw = true;
            break;
        case 43: /* + */
            zoom = true;
            break;
        case 45: /* - */
            zoom = false;
            break;
        case 119: /* w */
            if (yRange.start > ZOOM_MOVE_STEP)
            {
                yRange.start -= ZOOM_MOVE_STEP;
                yRange.end -= ZOOM_MOVE_STEP;
            }
            break;
        case 97: /* a */
            if (xRange.start > ZOOM_MOVE_STEP)
            {
                xRange.start -= ZOOM_MOVE_STEP;
                xRange.end -= ZOOM_MOVE_STEP;
            }
            break;
        case 115: /* s */
            if (yRange.end < yMax - ZOOM_MOVE_STEP)
            {
                yRange.start += ZOOM_MOVE_STEP;
                yRange.end += ZOOM_MOVE_STEP;
            }
            break;
        case 100: /* d */
            if (xRange.end < xMax - ZOOM_MOVE_STEP)
            {
                xRange.start += ZOOM_MOVE_STEP;
                xRange.end += ZOOM_MOVE_STEP;
            }
            break;
        case 99: /* c */
            unicolor = !unicolor;
            redraw = true;
            break;
        case 13: /* enter */
        case 27: /* escape */
            exit = true;
            break;
        default:
            break;
        }
    }
    return EXIT_SUCCESS;
}

double takeReference(cv::Mat img)
{
    int x = 100;
    int y = 100;
    bool firstMarkerSet;
    bool exit = false;
    cv::Point marker1;
    cv::Point marker2;
    double factor = 1;
    cv::Mat wrk_img;

    while (!exit)
    {
        /* delete last image and redraw marker on clean image */
        wrk_img.release();
        wrk_img = img.clone();
        cv::drawMarker(wrk_img, cv::Point(x, y), cv::Scalar(0, 255, 255),
                       cv::MARKER_CROSS, 50, 1);
        cv::imshow("bin", wrk_img);

        /* wait for user input to set marker */
        switch (cv::waitKey(0))
        {
        case 0: /* up */
            if (y > 0)
                y -= 1;
            break;
        case 1: /* down */
            if (y < wrk_img.rows)
                y += 1;
            break;
        case 2: /* left */
            if (x > 0)
                x -= 1;
            break;
        case 3: /* right */
            if (x < wrk_img.cols)
                x += 1;
            break;
        case 13: /* enter */
            if (!firstMarkerSet)
            {
                marker1 = cv::Point(x, y);
                cv::drawMarker(img, marker1, cv::Scalar(255, 255, 0), cv::MARKER_TILTED_CROSS, 50, 1);
                firstMarkerSet = true;
            }
            else
            {
                marker2 = cv::Point(x, y);
                cv::drawMarker(img, marker2, cv::Scalar(255, 255, 0), cv::MARKER_TILTED_CROSS, 50, 1);
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
    factor = cv::norm(marker1 - marker2);
    std::cout << "Reference Factor: " << factor << " px/cm" << std::endl;

    return factor;
}

void findRoots(cv::Mat &bin_img, cv::Mat &col_img, double scale, bool unicolor)
{
    cv::Scalar color;
    cv::Rect rect;
    std::vector<std::vector<cv::Point>> contours;
    double contourLength;
    double rootLength_cm;
    /* rotate whole image for vertical text */
    cv::rotate(bin_img, bin_img, cv::ROTATE_90_COUNTERCLOCKWISE);
    cv::rotate(col_img, col_img, cv::ROTATE_90_COUNTERCLOCKWISE);

    /* find contours on binary image */
    findContours(bin_img, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);
    drawContours(bin_img, contours, -1, cv::Scalar(255, 125, 0), 1, 8);

    for (int i = 0; i < contours.size(); i++)
    {
        /* span rectangle around contour */
        rect = cv::boundingRect(contours[i]);

        /* filter for root dimensions */
        if (rect.width < ROOT_HEIGHT_MAX && rect.width > ROOT_HEIGHT_MIN && rect.height < ROOT_WIDTH_MAX)
        {
            /* compute contour length */
            contourLength = cv::arcLength(contours[i], false);

            /* calculate length of root in cm */
            rootLength_cm = contourLength / (2 * scale);

            /* choose new color if not unicolor mode */
            color = (unicolor) ? COLOR_SET[0] : COLOR_SET[i % COLOR_SET_LEN];

            /* annotate the images with bounding boxes and the length */
            rectangle(bin_img, cv::Point(rect.x, rect.y),
                      cv::Point(rect.x + rect.width, rect.y + rect.height),
                      GREEN, 2);
            putText(bin_img, "Root: " + std::to_string(rootLength_cm),
                    cv::Point(rect.x + rect.width + 10, rect.y + rect.height), 0, 1,
                    GREEN);
            rectangle(col_img, cv::Point(rect.x, rect.y),
                      cv::Point(rect.x + rect.width, rect.y + rect.height),
                      color, 2);
            putText(col_img, "Root: " + std::to_string(rootLength_cm),
                    cv::Point(rect.x + rect.width + 10, rect.y + rect.height), 0, 1,
                    color, 2);
        }
    }
    /* rotate images back to original direction */
    cv::rotate(bin_img, bin_img, cv::ROTATE_90_CLOCKWISE);
    cv::rotate(col_img, col_img, cv::ROTATE_90_CLOCKWISE);
}
