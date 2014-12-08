/* file "archInfo.h */

/*  Copyright (c) 1994 Stanford University

    All rights reserved.

    Copyright (c) 1995,1996 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

#ifndef ARCHINFO_H
#define ARCHINFO_H

class archinfo {
  private:
    char *fam;          /* architectural family name */
    char *ver;          /* architectural revision/version number */
    char *vos;          /* vendor and os designation for target */
    char *impl;         /* chip name and variant */

    if_ops first;       /* range of valid opcodes for this arch. */
    if_ops last;        /* range is [first_op, last_op) */

  protected:
    void init_from_annote(file_set_entry *fse);

  public:
    archinfo(file_set_entry *fse)       { init_from_annote(fse); }
    archinfo(proc_sym *p)               { init_from_annote(p->file()); }
    ~archinfo()                         {}

    char *family()              { return fam; }
    char *version()             { return ver; }
    char *vendor_os()           { return vos; }
    char *implementation()      { return impl; }

    if_ops first_op()           { return first; }
    if_ops last_op()            { return last; }
    char *op_string(if_ops);

    FILE *fopen_mdfile(char *ext, char *mode="r"); 

    void print(FILE *fd = stdout);
};

EXPORTED_BY_MACHINE archinfo *target_arch;      /* pointers to target data */

enum {  /* register banks */
    GPR = 0,            /* general-purpose (e.g. integer) registers */
    FPR,                /* floating-point registers */
    SEG,                /* segment registers */
    CTL,                /* control (e.g. PC, condition code) registers */
    LAST_REG_BANK       /* must be the last entry in the enum */
}; 

extern char *reg_bank_string(int);

enum {  /* register conventions */
    CONST0 = 0,         /* constantly zero */
    RA,                 /* return address register */
    SP,                 /* stack pointer */
    GP,                 /* global pointer */
    FP,                 /* frame pointer */
    ARG,                /* argument registers */
    RET,                /* function return registers */
    SAV,                /* callee saved registers */
    TMP,                /* caller saved registers */
    ASM_TMP,            /* assembler temporary registers (caller saved) */
    GEN,                /* generic -- catch all for other registers in bank */
    LAST_REG_CONV       /* must be the last entry in the enum */
}; 

extern char *reg_conv_string(int);

struct reg_desc {
    int bank;           /* register bank */
    int conv;           /* primary convention */
    int encoding;       /* hardware encoding */
    char *name;         /* assembler name */
}; 

class reginfo {
    friend int yyparse(void); 
    friend void yy_rpair(int, char *);

  private:
    int n;              /* number of grains contained in all register banks */
    int *num_in_b;      /* size n_banks */
    int *num_in_bc;     /* size n_banks * n_conventions */
    int *start_of_b;    /* size n_banks */
    int *start_of_bc;   /* size n_banks * n_conventions */
    int *width_of_b;    /* size n_banks */
    int *gsize_of_b;    /* size n_banks */
    reg_desc *desc;     /* size n */
    int *m2a_map;       /* machine encoding to abstract number map; size n */

  public:       
    int total_grains() const { return n; }
    int num_in(int b) const { 
        assert(b < LAST_REG_BANK); 
        return num_in_b[b]; 
    }
    int num_in(int b, int c) const {
        assert(b < LAST_REG_BANK && c < LAST_REG_CONV); 
        return num_in_bc[b * LAST_REG_CONV + c]; 
    }
    int start_of(int b) const {
        assert(b < LAST_REG_BANK); 
        return start_of_b[b];
    }
    int start_of(int b, int c) const {
        assert(b < LAST_REG_BANK && c < LAST_REG_CONV); 
        return start_of_bc[b * LAST_REG_CONV + c]; 
    }
    int width_of(int b) const {
        assert(num_in(b) > 0);
        return width_of_b[b];
    }
    int grain_size_of(int b) const {
        assert(num_in(b) > 0);
        return gsize_of_b[b];
    }
    int lookup(int b, int c, int i) const {
        assert(i < num_in(b, c)); 
        return start_of(b, c) + i; 
    }

    reg_desc describe(int ar) const {
        assert(ar < n);
        return desc[ar];
    }
    char *name(int ar) const { return describe(ar).name; }

    /* mapping functions between abstract reg numbers and machine encodings */
    int a2m(int ar);
    int m2a(int b, int mr);

    void print(FILE *fd = stdout);

    reginfo(FILE *fd); 
    ~reginfo(); 
};

EXPORTED_BY_MACHINE reginfo *target_regs;    /* target register information */

#define REG(b, c, i) (target_regs->lookup(b, c, i))
#define REG_const0      (REG(GPR, CONST0, 0))
#define REG_ra          (REG(GPR, RA, 0))
#define REG_sp          (REG(GPR, SP, 0))
#define REG_gp          (REG(GPR, GP, 0))
#define REG_fp          (REG(GPR, FP, 0))

#endif /* ARCHINFO_H */
