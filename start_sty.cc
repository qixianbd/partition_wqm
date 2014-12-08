/* Generated automatically by the SUIF makefiles. */

#include <suif1.h>

extern void start_suif(int &argc, char *argv[])
  {
    LIBRARY(dfa, init_dfa, exit_dfa);
    LIBRARY(useful, init_useful, exit_useful);
    LIBRARY(machine, init_machine, exit_machine);
    LIBRARY(cfg, init_cfg, exit_cfg);

    init_suif(argc, argv);
  }
