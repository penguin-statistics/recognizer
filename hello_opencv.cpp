#include <stdio.h>
#include <opencv2/opencv.hpp>

using namespace cv;

int main(int argc, char **argv)
{
    Mat image;
    image = imread("lena.jpg", 1);
    if (!image.data)
    {
        printf("No image data \n");
        return -1;
    }
    namedWindow("Hello OpenCV", WINDOW_AUTOSIZE);
    imshow("Hello OpenCV", image);
    waitKey(0);
    return 0;
}