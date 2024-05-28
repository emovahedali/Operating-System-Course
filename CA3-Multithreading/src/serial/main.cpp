#include <iostream>
#include <string>
#include <chrono>
#include <opencv4/opencv2/core.hpp>
#include <opencv4/opencv2/highgui.hpp>
#include <opencv4/opencv2/imgproc.hpp>

using Kernel = std::vector<std::vector<int>>;


void perform_kernel_on_pixel(int row, int col, cv::Mat& bin_img, const cv::Mat& ORIGINAL_IMG, Kernel& kernel){
    int kernel_rows = kernel.size();
    int kernel_cols = kernel[0].size();
    
    uchar chan = 0;
    for(int i = 0; i < kernel_rows; ++i)
        for(int j = 0; j < kernel_cols; ++j)
        {
            int t_row = row - kernel_rows / 2 + i;
            int t_col = col - kernel_rows / 2 + j;
            if(t_row < 0 || t_row >= bin_img.rows)
                continue;
            if(t_col < 0 || t_col >= bin_img.cols)
                continue;
            chan += ORIGINAL_IMG.at<uchar>(t_row, t_col) * kernel[i][j];
        }
    
    chan = std::min(255, (int)chan);
    chan = std::max(0, (int)chan);

    bin_img.at<uchar>(row, col) = chan;
}

void detect_edges_by_laplacian_filter(cv::Mat& bin_img, Kernel& kernel)
{
    cv::Mat original_img = bin_img.clone();
    for(int row = 0; row < bin_img.rows; ++row)
        for(int col = 0; col < bin_img.cols; ++col)
            perform_kernel_on_pixel(row, col, bin_img, original_img, kernel);
    original_img.release();
}

void convert_gray2bin(cv::Mat& gray_img, cv::Mat& bin_img)
{
    int thresh = 127;
    cv::threshold(gray_img, bin_img, thresh, 255, cv::THRESH_BINARY);
    if(bin_img.empty())
    {
        std::cout << "Error, the binary image is empty" << std::endl;
        exit(EXIT_FAILURE);
    }
}

void perform_box_blur_on_pixel(int row, int col, cv::Mat& img, const cv::Mat& ORIGINAL_IMG)
{
    uchar chan = 0;
    for(int i = 0; i < 3; ++i)
        for(int j = 0; j < 3; ++j)
        {
            int t_row = row - 1 + i;
            int t_col = col - 1 + j;
            if(t_row < 0 || t_row >= img.rows)
                continue;
            if(t_col < 0 || t_col >= img.cols)
                continue;
            chan += ORIGINAL_IMG.at<uchar>(t_row, t_col) / 9;
        }

    chan = std::min(255, (int)chan);
    chan = std::max(0, (int)chan);

    img.at<uchar>(row, col) = chan;
}

void perform_box_blur_filter(cv::Mat& img)
{
    cv::Mat original_img = img.clone();
    for(int row = 0; row < img.rows; ++row)
        for(int col = 0; col < img.cols; ++col)
            perform_box_blur_on_pixel(row, col, img, original_img);
    original_img.release();
}

void perform_sepia_filter(cv::Mat& img)
{
    for(int row = 0; row < img.rows; ++row)
        for(int col = 0; col < img.cols; ++col)
        {
            uchar b_chan = img.at<cv::Vec3b>(row, col)[0];
            uchar g_chan = img.at<cv::Vec3b>(row, col)[1];
            uchar r_chan = img.at<cv::Vec3b>(row, col)[2];

            int blue = 0.272f * r_chan + 0.534f * g_chan + 0.131f * b_chan;
            int green = 0.349f * r_chan + 0.686f * g_chan + 0.168f * b_chan;
            int red = 0.393f * r_chan + 0.769f * g_chan + 0.189f * b_chan;
            red = std::max(red, 0);
            green = std::max(green, 0);
            blue = std::max(blue, 0);
            red = std::min(red, 255);
            green = std::min(green, 255);
            blue = std::min(blue, 255);

            img.at<cv::Vec3b>(row, col)[0] = (uchar)blue;
            img.at<cv::Vec3b>(row, col)[1] = (uchar)green;
            img.at<cv::Vec3b>(row, col)[2] = (uchar)red;
        }
}

void reverse_vertically(cv::Mat& img)
{
    int rows = img.rows;
    int cols = img.cols;

    for(int row = 0; row < rows / 2; ++row)
        for(int col = 0; col < cols; ++col)
        {
            cv::Vec3b temp_pixel = img.at<cv::Vec3b>(row, col);
            img.at<cv::Vec3b>(row, col) = img.at<cv::Vec3b>(rows - row - 1, col);
            img.at<cv::Vec3b>(rows - row - 1, col) = temp_pixel;
        }
}

void reverse_horizontally(cv::Mat& img)
{
    int rows = img.rows;
    int cols = img.cols;

    for(int row = 0; row < rows; ++row)
        for(int col = 0; col < cols / 2; col++)
        {
            cv::Vec3b temp_pixel = img.at<cv::Vec3b>(row, col);
            img.at<cv::Vec3b>(row, col) = img.at<cv::Vec3b>(row, cols - col - 1);
            img.at<cv::Vec3b>(row, cols - col - 1) = temp_pixel;
        }
}

void convert_bgr2gray(cv::Mat& color_img, cv::Mat& gray_img)
{
    cv::cvtColor(color_img, gray_img, cv::ColorConversionCodes::COLOR_BGR2GRAY);
    if(gray_img.empty())
    {
        std::cout << "Error, the gray image is empty" << std::endl;
        exit(EXIT_FAILURE);
    }
}

void read_image(cv::Mat& img, char* file_path, int imread_mode)
{
    const std::string FILE_PATH(file_path);
    img = cv::imread(FILE_PATH, imread_mode);
    if(img.empty())
    {
        std::cout << "Error, input image is empty" << std::endl;
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char* argv[])
{
    if(argc != 2)
    {
        std::cout << "Bad arguments!" << std::endl;
        exit(EXIT_FAILURE);
    }
    cv::Mat color_img, gray_img, bin_img;
    Kernel kernel = {{0, -1, 0}, {-1, 4, -1}, {0, -1, 0}};

    auto t1 = std::chrono::high_resolution_clock::now();
    read_image(color_img, argv[1], cv::ImreadModes::IMREAD_ANYCOLOR);
    auto t2 = std::chrono::high_resolution_clock::now();
    convert_bgr2gray(color_img, gray_img);
    auto t3 = std::chrono::high_resolution_clock::now();
    reverse_horizontally(color_img);
    auto t4 = std::chrono::high_resolution_clock::now();
    reverse_vertically(color_img);
    auto t5 = std::chrono::high_resolution_clock::now();
    perform_sepia_filter(color_img);

    auto t6 = std::chrono::high_resolution_clock::now();
    perform_box_blur_filter(gray_img);
    auto t7 = std::chrono::high_resolution_clock::now();
    convert_gray2bin(gray_img, bin_img);
    auto t8 = std::chrono::high_resolution_clock::now();
    detect_edges_by_laplacian_filter(bin_img, kernel);
    auto t9 = std::chrono::high_resolution_clock::now();

    cv::imwrite("First_Serial.bmp", color_img);
    cv::imwrite("Second_Serial.bmp", bin_img);
    auto t10 = std::chrono::high_resolution_clock::now();

    color_img.release();
    gray_img.release();
    bin_img.release();
    auto t11 = std::chrono::high_resolution_clock::now();

    // std::cout << "Time to read: " << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << "ms" << std::endl;
    // std::cout << "Time to convert bgr to gray: " << std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count() << "ms" << std::endl;
    // std::cout << "Time to reverse horizontally: " << std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count() << "ms" << std::endl;
    // std::cout << "Time to reverse vertically: " << std::chrono::duration_cast<std::chrono::milliseconds>(t5 - t4).count() << "ms" << std::endl;
    // std::cout << "Time to perform sepia filter: " << std::chrono::duration_cast<std::chrono::milliseconds>(t6 - t5).count() << "ms" << std::endl;
    // std::cout << "Time to perform box blur filter: " << std::chrono::duration_cast<std::chrono::milliseconds>(t7 - t6).count() <<  "ms" <<std::endl;
    // std::cout << "Time to convert gray to bin: " << std::chrono::duration_cast<std::chrono::milliseconds>(t8 - t7).count() << "ms" << std::endl;
    // std::cout << "Time to detect edges by laplacian filter: " << std::chrono::duration_cast<std::chrono::milliseconds>(t9 - t8).count() << "ms" << std::endl;
    // std::cout << "Time to write: " << std::chrono::duration_cast<std::chrono::milliseconds>(t10 - t9).count() << "ms" << std::endl;
    // std::cout << "Time to release: " << std::chrono::duration_cast<std::chrono::milliseconds>(t11 - t10).count() << "ms" << std::endl;
    std::cout << "Execution Time : " << std::chrono::duration_cast<std::chrono::milliseconds>(t11 - t1).count() << std::endl;

    return EXIT_SUCCESS;
}