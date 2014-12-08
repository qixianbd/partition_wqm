#include <iostream>
#include <fstream>
#include "threshold.h"

std::string threshold::threshold_file_name = "threshold";
///home/harry/Prophet/i386-redhat-linux/bin

/*
unsigned int threshold::dependence_threshold = 4;
unsigned int threshold::thread_size_lower = 15;
unsigned int threshold::thread_size_upper = 50;
unsigned int threshold::spawning_distance_lower = 5;
unsigned int threshold::call_lower_threshold = 5;
*/


unsigned int threshold::nloop_dependence_threshold = 0;
unsigned int threshold::nloop_thread_size_lower = 0;
unsigned int threshold::nloop_thread_size_upper = 0;
unsigned int threshold::nloop_spawning_distance_lower = 0;
unsigned int threshold::nloop_spawning_distance_upper = 0;
//static unsigned int threshold::nloop_call_lower_threshold;
double threshold::definit_prob_delta = 0.0;
double threshold::multi_prob_delta = 0.35;

unsigned int threshold::time_ahead = 2;



void threshold::parse_file()
{
   std::ifstream input(threshold::threshold_file_name.c_str());
    if(!input)
        return;
//    process_charact();


    while(input)
    {
        std::string threshold_name;
        int value;

        if(!(input >> threshold_name))
            return;
        if(!(input >> value))
            return;

        if(threshold_name == "nloop_dependence_threshold")
            threshold::nloop_dependence_threshold = value;
        else if(threshold_name == "nloop_thread_size_lower")
            threshold::nloop_thread_size_lower = value;
        else if(threshold_name == "nloop_thread_size_upper")
            threshold::nloop_thread_size_upper = value;
        else if(threshold_name == "nloop_spawning_distance_lower")
            threshold::nloop_spawning_distance_lower = value;
        else if(threshold_name == "nloop_spawning_distance_upper")
                   threshold::nloop_spawning_distance_upper = value;
        else if(threshold_name == "definit_prob_delta")
                 threshold::definit_prob_delta = (double)value / (double)100;
        else if(threshold_name == "time_ahead")
                 threshold::time_ahead = value;
        //else if(threshold_name == "call_lower_threshold")
          //  threshold::call_lower_threshold = value;
    }

}

void threshold::print()
{
    std::cout<<"dependence_threshold : "<<threshold::nloop_dependence_threshold << std::endl;
    std::cout<<"thread_size_lower : " << threshold::nloop_thread_size_lower <<std::endl;
    std::cout<<"thread_size_upper :" << threshold::nloop_thread_size_upper <<std::endl;
    std::cout<<"spawning_distance_lower : "<< threshold::nloop_spawning_distance_lower << std::endl;
    std::cout<<"spawning_distance_upper : "<< threshold::nloop_spawning_distance_upper << std::endl;
    std::cout<<"definit_prob_delta : "<< threshold::definit_prob_delta << std::endl;
    std::cout<<"time_ahead : "<< threshold::time_ahead << std::endl;
    //std::cout<<"call_lower_threshold : "<<threshold::nloop_call_lower_threshold <<std::endl;
}

/*
 *判断线程大小是否合适
 */
bool threshold::is_middle(const unsigned int thread_size) {
	if (thread_size >= threshold::nloop_thread_size_lower && thread_size
			<= threshold::nloop_thread_size_upper)
		return true;
	else
		return false;
}

/*
 *线程太大
 */
bool threshold::is_big(const unsigned int thread_size) {
	if (thread_size > threshold::nloop_thread_size_upper)
		return true;
	else
		return false;
}

/*
 *线程太小
 */
bool threshold::is_small(const unsigned int thread_size) {
	if (thread_size < threshold::nloop_thread_size_lower)
		return true;
	else
		return false;
}
