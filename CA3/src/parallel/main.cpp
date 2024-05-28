#include <iostream>
#include <pthread.h>
#include <vector>
#include <chrono>
#include <opencv4/opencv2/core.hpp>
#include <opencv4/opencv2/highgui.hpp>
#include <opencv4/opencv2/imgproc.hpp>

using Kernel = std::vector<std::vector<int>>;

const int NUM_THREADS = 8;

class ThreadData{
public:
    ThreadData(int start, int num): start(start), num(num){};
    ThreadData(int start, int num, Kernel kernel): start(start), 
                                                   num(num),
                                                   kernel(kernel){};
    Kernel kernel;
    int start;
    int num;
};

int rows;
int cols;
cv::Mat img, original_img, gray_img, bin_img;    
    

void perform_kernel_on_pixel(int row, int col, Kernel& kernel){
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
            chan += original_img.at<uchar>(t_row, t_col) * kernel[i][j];
        }
    
    chan = std::min(255, (int)chan);
    chan = std::max(0, (int)chan);

    bin_img.at<uchar>(row, col) = chan;
}

void *filter_kernel_thread(void* data){
    ThreadData* thread_data = (ThreadData*)data;
    int num = thread_data->num;
    Kernel kernel = thread_data->kernel;

    for (int i = 0; i < num; i++){
        for (int j = 0; j < cols; j++){
            perform_kernel_on_pixel(thread_data->start + i, j, kernel);
        }
    }
    pthread_exit(NULL);
}

void detect_edges_by_laplacian_filter(Kernel& kernel)
{
    original_img = bin_img.clone();
    
    ThreadData* data[NUM_THREADS]; 
    pthread_t threads[NUM_THREADS];
    int start = 0;
    int thread_num = rows / NUM_THREADS;

    for (int i = 0; i < NUM_THREADS; i++){
        data[i] = new ThreadData(start, thread_num, kernel);
        if (i == NUM_THREADS - 1){
            data[i]->num += rows % NUM_THREADS;
        }
        int return_code = pthread_create(&threads[i], NULL, filter_kernel_thread, (void*)data[i]);
        if (return_code){
            std::cout << "Error: unable to create thread, " << return_code << std::endl;
            exit(-1);
        }
        start += thread_num;
    }

    for (int i = 0; i < NUM_THREADS; i++){
        pthread_join(threads[i], NULL);
    }
}

void convert_gray2bin()
{
    int thresh = 127;
    cv::threshold(gray_img, bin_img, thresh, 255, cv::THRESH_BINARY);
    if(bin_img.empty())
    {
        std::cout << "Error, the binary image is empty" << std::endl;
        exit(EXIT_FAILURE);
    }
}

void perform_box_blur_on_pixel(int row, int col)
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
            chan += original_img.at<uchar>(t_row, t_col) / 9;
        }

    chan = std::min(255, (int)chan);
    chan = std::max(0, (int)chan);

    gray_img.at<uchar>(row, col) = chan;
}

void *perform_box_blur_thread(void* data){
    ThreadData* thread_data = (ThreadData*)data;
    int num = thread_data->num;

    for (int i = 0; i < num; i++){
        for (int j = 0; j < cols; j++){
            perform_box_blur_on_pixel(thread_data->start + i, j);
        }
    }
    pthread_exit(NULL);
}

void perform_box_blur_filter()
{
    original_img = gray_img.clone();
    
    ThreadData* data[NUM_THREADS]; 
    pthread_t threads[NUM_THREADS];
    int start = 0;
    int thread_num = rows / NUM_THREADS;

    for (int i = 0; i < NUM_THREADS; i++){
        data[i] = new ThreadData(start, thread_num);
        if (i == NUM_THREADS - 1){
            data[i]->num += rows % NUM_THREADS;
        }
        int return_code = pthread_create(&threads[i], NULL, perform_box_blur_thread, (void*)data[i]);
        if (return_code){
            std::cout << "Error: unable to create thread, " << return_code << std::endl;
            exit(-1);
        }
        start += thread_num;
    }

    for (int i = 0; i < NUM_THREADS; i++){
        pthread_join(threads[i], NULL);
    }
}

void* perform_sepia_thread(void* data)
{
    ThreadData* thread_data = (ThreadData*)data;
    int num = thread_data->num;

    for(int row = 0; row < num; ++row)
        for(int col = 0; col < cols; ++col)
        {
            uchar b_chan = img.at<cv::Vec3b>(thread_data->start + row, col)[0];
            uchar g_chan = img.at<cv::Vec3b>(thread_data->start + row, col)[1];
            uchar r_chan = img.at<cv::Vec3b>(thread_data->start + row, col)[2];

            int blue = 0.272f * r_chan + 0.534f * g_chan + 0.131f * b_chan;
            int green = 0.349f * r_chan + 0.686f * g_chan + 0.168f * b_chan;
            int red = 0.393f * r_chan + 0.769f * g_chan + 0.189f * b_chan;
            red = std::max(red, 0);
            green = std::max(green, 0);
            blue = std::max(blue, 0);
            red = std::min(red, 255);
            green = std::min(green, 255);
            blue = std::min(blue, 255);

            img.at<cv::Vec3b>(thread_data->start + row, col)[0] = (uchar)blue;
            img.at<cv::Vec3b>(thread_data->start + row, col)[1] = (uchar)green;
            img.at<cv::Vec3b>(thread_data->start + row, col)[2] = (uchar)red;
        }
    pthread_exit(NULL);
}

void perform_sepia_filter() {
    ThreadData* data[NUM_THREADS];
    pthread_t threads[NUM_THREADS];
    int start = 0;
    int thread_num = rows / NUM_THREADS;

    for (int i = 0; i < NUM_THREADS; i++){
        data[i] = new ThreadData(start, thread_num);
        if (i == NUM_THREADS - 1){
            data[i]->num += rows % NUM_THREADS;
        }
        int return_code = pthread_create(&threads[i], NULL, perform_sepia_thread, (void*)data[i]);
        if (return_code){
            std::cout << "Error: unable to create thread, " << return_code << std::endl;
            exit(-1);
        }
        start += thread_num;

    }
    for (int i = 0; i < NUM_THREADS; i++){
        pthread_join(threads[i], NULL);
    }
}

void* reverse_v_thread(void* data)
{
    ThreadData* thread_data = (ThreadData*)data;
    int num = thread_data->num;

    for (int i = 0; i < num; i++) {
        for (int j = 0; j < cols; j++) {
            cv::Vec3b temp_pixel = img.at<cv::Vec3b>(thread_data->start + i, j);
            img.at<cv::Vec3b>(thread_data->start + i, j) = img.at<cv::Vec3b>(rows - thread_data->start - i - 1, j);
            img.at<cv::Vec3b>(rows - thread_data->start - i - 1, j) = temp_pixel;
        }
    }
    pthread_exit(NULL);
}

void reverse_vertically()
{
    ThreadData* data[NUM_THREADS];
    pthread_t threads[NUM_THREADS];
    int start_row = 0;
    int thread_num_rows = rows / (NUM_THREADS * 2);

    for (int i = 0; i < NUM_THREADS; i++){
        data[i] = new ThreadData(start_row, thread_num_rows);
        if (i == NUM_THREADS - 1){
            data[i]->num += rows % (NUM_THREADS * 2);
        }
        int return_code = pthread_create(&threads[i], NULL, reverse_v_thread, (void*)data[i]);
        if (return_code){
            std::cout << "Error: unable to create thread, " << return_code << std::endl;
            exit(-1);
        }
        start_row += thread_num_rows;

    }
    for (int i = 0; i < NUM_THREADS; i++){
        pthread_join(threads[i], NULL);
    }
}

void* reverse_h_thread(void* data){
    ThreadData* thread_data = (ThreadData*)data;
    int num = thread_data->num;

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < num; j++) {
            cv::Vec3b temp_pixel = img.at<cv::Vec3b>(i, thread_data->start + j);
            img.at<cv::Vec3b>(i, thread_data->start + j) = img.at<cv::Vec3b>(i, cols - thread_data->start - j - 1);
            img.at<cv::Vec3b>(i, cols - thread_data->start - j - 1) = temp_pixel;
        }
    }
    pthread_exit(NULL);
}

void reverse_horizontally()
{
    ThreadData* data[NUM_THREADS];
    pthread_t threads[NUM_THREADS];
    int start_col = 0;
    int thread_num_cols = cols / (NUM_THREADS * 2);

    for (int i = 0; i < NUM_THREADS; i++){
        data[i] = new ThreadData(start_col, thread_num_cols);
        if (i == NUM_THREADS - 1){
            data[i]->num += cols % (NUM_THREADS * 2);
        }
        int return_code = pthread_create(&threads[i], NULL, reverse_h_thread, (void*)data[i]);
        if (return_code){
            std::cout << "Error: unable to create thread, " << return_code << std::endl;
            exit(-1);
        }
        start_col += thread_num_cols;

    }
    for (int i = 0; i < NUM_THREADS; i++){
        pthread_join(threads[i], NULL);
    }
}

void convert_bgr2gray()
{
    cv::cvtColor(img, gray_img, cv::ColorConversionCodes::COLOR_BGR2GRAY);
    if(gray_img.empty())
    {
        std::cout << "Error, the gray image is empty" << std::endl;
        exit(EXIT_FAILURE);
    }
}

void read_image(char* file_path, int imread_mode)
{
    const std::string FILE_PATH(file_path);
    img = cv::imread(FILE_PATH, imread_mode);
    if(img.empty())
    {
        std::cout << "Error, input image is empty" << std::endl;
        exit(EXIT_FAILURE);
    }
    rows = img.rows;
    cols = img.cols;
}

int main(int argc, char* argv[])
{
    if(argc != 2)
    {
        std::cout << "Bad arguments!" << std::endl;
        exit(EXIT_FAILURE);
    }
    
    Kernel kernel = {{0, -1, 0}, {-1, 4, -1}, {0, -1, 0}};
    
    auto t1 = std::chrono::high_resolution_clock::now();
    read_image(argv[1], cv::ImreadModes::IMREAD_ANYCOLOR);
    auto t2 = std::chrono::high_resolution_clock::now();
    convert_bgr2gray();
    auto t3 = std::chrono::high_resolution_clock::now();
    reverse_horizontally();
    auto t4 = std::chrono::high_resolution_clock::now();
    reverse_vertically();
    auto t5 = std::chrono::high_resolution_clock::now();
    perform_sepia_filter();

    auto t6 = std::chrono::high_resolution_clock::now();
    perform_box_blur_filter();
    auto t7 = std::chrono::high_resolution_clock::now();
    convert_gray2bin();
    auto t8 = std::chrono::high_resolution_clock::now();
    detect_edges_by_laplacian_filter(kernel);
    auto t9 = std::chrono::high_resolution_clock::now();

    cv::imwrite("First_Parallel.bmp", img);
    cv::imwrite("Second_Parallel.bmp", bin_img);
    auto t10 = std::chrono::high_resolution_clock::now();

    img.release();
    gray_img.release();
    bin_img.release();
    original_img.release();
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
