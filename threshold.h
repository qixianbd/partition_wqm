#ifndef COMMAND_PARAMETERS_H
#define COMMAND_PARAMETERS_H

#include <string>

class threshold
{
public:
    static std::string threshold_file_name;
    /*    static unsigned int dependence_threshold;
    static unsigned int thread_size_lower;
    static unsigned int thread_size_upper;
    static unsigned int spawning_distance_lower;
    static unsigned int call_lower_threshold;*/

    /*
     * 要用我的公式计算， dependence/ spawning_distance = 越小越好s
     */
    static unsigned int nloop_thread_size_lower;
    static unsigned int nloop_thread_size_upper;
    static unsigned int nloop_spawning_distance_lower;
    static unsigned int nloop_spawning_distance_upper;
    static unsigned int nloop_call_lower_threshold;
    static unsigned int nloop_dependence_threshold;
    static unsigned int time_ahead;  //能够提前的指令数(=spawn_distance - dept_count)

public:
    static void parse_file();
    static void print();

    static bool is_middle(const unsigned int thread_size);
    static bool is_big(const unsigned int thread_size);
    static bool is_small(const unsigned int thread_size);


protected:
    static double definit_prob_delta;
    static double multi_prob_delta;
public:
    static bool is_path_definite(double taken_prob){
    	return is_taken_definite(taken_prob) || is_fall_definite(taken_prob);
    }
    /*
    static bool is_multi_path_opt(double taken_prob){
    	return is_taken_definite(taken_prob) || is_fall_definite(taken_prob);
    }*/


    static bool is_taken_definite(double taken_prob){
    	return taken_prob > 0.5 + definit_prob_delta;
    }

    static bool is_non_taken_opt(double taken_prob){
    	return taken_prob < 0.5 - multi_prob_delta;
    }


    static bool is_fall_definite(double taken_prob){
    	return taken_prob < 0.5 - definit_prob_delta;
    }

    static bool is_non_fall_opt(double taken_prob){
    	return taken_prob > 0.5 +  multi_prob_delta;
    }
};


#endif
