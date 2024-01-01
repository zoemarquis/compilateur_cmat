#include "generation.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "EXPR.tab.h"
#include "cmat.h"

unsigned cpt_label = 0;

struct code *code_new() {
  struct code *r = malloc(sizeof(struct code));
  r->capacity = 1024;
  r->quads = malloc(r->capacity * sizeof(struct quad));
  r->nextquad = 0;
  return r;
}

static void code_grow(struct code *c) {
  c->capacity += 1024;
  c->quads = realloc(c->quads, c->capacity * sizeof(struct quad));
  if (c->quads == NULL) {
    fprintf(stderr,
            "Erreur lors de la tentative d'expansion de la liste des "
            "quadruplets (taille actuelle = %d)\n",
            c->nextquad);
    exit(1);
  }
}

void gencode(struct code *c, enum quad_kind k, struct symbol *s1,
             struct symbol *s2, struct symbol *s3) {
  if (c->nextquad == c->capacity) code_grow(c);
  struct quad *q = &(c->quads[c->nextquad]);
  q->kind = k;
  q->sym1 = s1;
  q->sym2 = s2;
  q->sym3 = s3;
  ++(c->nextquad);
}

struct symbol *newtemp(struct symtable *t, unsigned type, valeur val) {
  struct symbol *s;
  name_t name;
  sprintf(name, "t%d", t->temporary);
  variable *v = creer_variable(name, type, false, val);
  s = symtable_put(t, name, v);
  ++(t->temporary);
  return s;
}

static void symbol_dump(struct symbol *s) {
  switch (s->kind) {
    case NAME:
      printf("%s", s->name);
      break;
    case CONST_INT:
      printf("%d", s->const_int);
      break;
    case CONST_FLOAT:
      printf("%f", s->const_float);
      break;
    default:
      break;
  }
}

static void quad_dump(struct quad *q) {
  switch (q->kind) {
    case COPY:

      // int = int
      if (q->sym1->var->type == INT) {
        if (q->sym2->kind == CONST_INT ||
            (q->sym2->kind == NAME && q->sym2->var->type == INT))
          printf("\tlw $t0, %s\n", q->sym2->name);
        else {
          fprintf(stderr, "Incompatibilité de types à l'affectation de %s.\n",
                  q->sym1->name);
          exit(1);
        }
        printf("\tsw $t0,%s\n", q->sym1->name);
      }

      if (q->sym1->var->type == FLOAT) {
        // float = float
        if ((q->sym2->kind == CONST_FLOAT) ||
            (q->sym2->kind == NAME && q->sym2->var->type == FLOAT))
          printf("\tl.s $f0, %s\n", q->sym2->name);
        // float = int
        else if (q->sym2->kind == CONST_INT ||
                 (q->sym2->kind == NAME && q->sym2->var->type == INT)) {
          printf("\tlw $t0, %s\n", q->sym2->name);
          printf("\tmtc1 $t0, $f0\n");
          printf("\tcvt.s.w $f0, $f0\n");
        } else {
          fprintf(stderr, "Incompatibilité de types à l'affectation de %s.\n",
                  q->sym1->name);
          exit(1);
        }
        printf("\ts.s $f0, %s\n", q->sym1->name);
      }

      // matrix = matrix
      if (q->sym1->var->type == MATRIX) {
        if (q->sym2->kind == NAME && q->sym2->var->type == MATRIX) {
          printf("\tla $a0, %s\n", q->sym2->name);
          printf("\tla $a1, %s\n", q->sym1->name);
          printf("\tlw $t2, %s_rows\n", q->sym1->name);
          printf("\tlw $t3, %s_cols\n", q->sym1->name);

          printf("\tli $t0, 0\n");
          printf("boucle_copy_matrices_ligne_%d:\n", cpt_label);
          printf("\tbge $t0, $t2, fin_copy_%d\n", cpt_label);

          printf("\tli $t1, 0\n");
          printf("\tboucle_copy_matrices_colonne_%d:\n", cpt_label);
          printf("\t\tbge $t1, $t3, ligne_suivante_%d\n", cpt_label);

          printf("\t\tl.s $f0, 0($a0)\n");
          printf("\t\ts.s $f0, 0($a1)\n");

          printf("\t\taddi $a0, $a0, 4\n");
          printf("\t\taddi $a1, $a1, 4\n");
          printf("\t\taddi $t1, $t1, 1\n");

          printf("\t\tj boucle_copy_matrices_colonne_%d\n", cpt_label);
          printf("\tligne_suivante_%d:\n", cpt_label);
          printf("\t\taddi $t0, $t0, 1\n");
          printf("\t\tj boucle_copy_matrices_ligne_%d\n", cpt_label);

          printf("fin_copy_%d:\n", cpt_label);
          cpt_label++;
        } else {
          fprintf(stderr, "Incompatibilité de types à l'affectation de %s.\n",
                  q->sym1->name);
          exit(1);
        }
      }

      break;

    case BOP_PLUS:

      unsigned type1, type2;

      if ((q->sym2->kind == NAME && q->sym2->var->type == MATRIX) &&
          (q->sym3->kind == NAME &&
           q->sym3->var->type == MATRIX)) {  // add 2 matrix
        printf("\tla $a0, %s\n", q->sym2->name);
        printf("\tla $a1, %s\n", q->sym3->name);
        printf("\tla $a2, %s\n", q->sym1->name);
        printf("\tlw $t2, %s_rows\n", q->sym2->name);
        printf("\tlw $t3, %s_cols\n", q->sym2->name);

        printf("\tli $t0, 0\n");
        printf("boucle_add_matrices_ligne_%d:\n", cpt_label);
        printf("\tbge $t0, $t2, fin_add_%d\n", cpt_label);

        printf("\tli $t1, 0\n");
        printf("\tboucle_add_matrices_colonne_%d:\n", cpt_label);
        printf("\t\tbge $t1, $t3, ligne_suivante_%d\n", cpt_label);

        printf("\t\tl.s $f0, 0($a0)\n");
        printf("\t\tl.s $f1, 0($a1)\n");
        printf("\t\tadd.s $f2, $f0, $f1\n");
        printf("\t\ts.s $f2, 0($a2)\n");

        printf("\t\taddi $a0, $a0, 4\n");
        printf("\t\taddi $a1, $a1, 4\n");
        printf("\t\taddi $a2, $a2, 4\n");
        printf("\t\taddi $t1, $t1, 1\n");

        printf("\t\tj boucle_add_matrices_colonne_%d\n", cpt_label);
        printf("\tligne_suivante_%d:\n", cpt_label);
        printf("\t\taddi $t0, $t0, 1\n");
        printf("\t\tj boucle_add_matrices_ligne_%d\n", cpt_label);

        printf("fin_add_%d:\n", cpt_label);

        cpt_label++;
      } else if ((q->sym2->kind == NAME && q->sym2->var->type == MATRIX) ||
                 (q->sym3->kind == NAME &&
                  q->sym3->var->type == MATRIX)) {  // mat + cst ou cst + mat

        name_t mat;
        name_t cst;
        unsigned type = 0;

        if (q->sym2->kind == NAME && q->sym2->var->type == MATRIX) {
          strcpy(mat, q->sym2->name);
          strcpy(cst, q->sym3->name);
          if (q->sym3->kind == NAME) {
            type = q->sym3->var->type;
          } else {
            if (q->sym3->kind == CONST_INT)
              type = INT;
            else if (q->sym3->kind == CONST_FLOAT)
              type = FLOAT;
            else {  // cas impossible ?
              fprintf(stderr, "Incompatibilité de types dans une addition.\n");
              exit(1);
            }
          }
        } else {
          strcpy(mat, q->sym3->name);
          strcpy(cst, q->sym2->name);
          if (q->sym2->kind == NAME) {
            type = q->sym2->var->type;
          } else {
            if (q->sym2->kind == CONST_INT)
              type = INT;
            else if (q->sym2->kind == CONST_FLOAT)
              type = FLOAT;
            else {  // cas impossible ?
              fprintf(stderr, "Incompatibilité de types dans une addition.\n");
              exit(1);
            }
          }
        }

        printf("\tla $a0, %s\n", mat);
        printf("\tlwc1 $f1, %s\n", cst);
        if (type == INT) {
          printf("\tcvt.s.w $f1, $f1\n");
        }
        printf("\tla $a1, %s\n", q->sym1->var->name);
        printf("\tlw $t2, %s_rows\n", mat);
        printf("\tlw $t3, %s_cols\n", mat);

        printf("\tli $t4, 0\n");
        printf("boucle_add_matrice_cst_ligne_%d:\n", cpt_label);
        printf("\tbge $t4, $t2, fin_add_%d\n", cpt_label);

        printf("\tli $t5, 0\n");
        printf("\tboucle_add_matrice_cst_colonne_%d:\n", cpt_label);
        printf("\t\tbge $t5, $t3, ligne_suivante_%d\n", cpt_label);

        printf("\t\tlwc1 $f2, 0($a0)\n");
        printf("\t\tadd.s $f3, $f2, $f1\n");

        printf("\t\tswc1 $f3, 0($a1)\n");

        printf("\t\taddi $a0, $a0, 4\n");
        printf("\t\taddi $a1, $a1, 4\n");
        printf("\t\taddi $t5, $t5, 1\n");

        printf("\t\tj boucle_add_matrice_cst_colonne_%d\n", cpt_label);
        printf("\tligne_suivante_%d:\n", cpt_label);
        printf("\t\taddi $t4, $t4, 1\n");
        printf("\t\tj boucle_add_matrice_cst_ligne_%d\n", cpt_label);

        printf("fin_add_%d:\n", cpt_label);
        cpt_label++;
      } else {  // add de int et de float
        // sym2
        if (q->sym2->kind == CONST_INT ||
            (q->sym2->kind == NAME && q->sym2->var->type == INT)) {
          printf("\tlw $t0, %s\n", q->sym2->name);
          type1 = INT;
        }  // int
        else if ((q->sym2->kind == CONST_FLOAT) ||
                 (q->sym2->kind == NAME && q->sym2->var->type == FLOAT)) {
          printf("\tl.s $f0, %s\n", q->sym2->name);
          type1 = FLOAT;
        }  // float

        // sym3
        if (q->sym3->kind == CONST_INT ||
            (q->sym3->kind == NAME && q->sym3->var->type == INT)) {
          printf("\tlw $t1, %s\n", q->sym3->name);
          type2 = INT;
        }  // int
        else if ((q->sym3->kind == CONST_FLOAT) ||
                 (q->sym3->kind == NAME && q->sym3->var->type == FLOAT)) {
          printf("\tl.s $f1, %s\n", q->sym3->name);
          type2 = FLOAT;
        }  // float

        if (type1 == INT && type2 == INT) {  // add int
          printf("\tadd $t0,$t0,$t1\n");
          printf("\tsw $t0, %s\n", q->sym1->name);
        } else {
          if (type1 == INT && type2 == FLOAT) {
            printf("mtc1 $t0, $f0\n");
            printf("cvt.s.w $f0, $f0\n");
          } else if (type1 == FLOAT && type2 == INT) {
            printf("mtc1 $t1, $f1\n");
            printf("cvt.s.w $f1, $f1\n");
          }  // cast int -> float
          printf("\tadd.s $f2,$f0,$f1\n");
          printf("\ts.s $f2, %s\n", q->sym1->name);  // add float
        }
      }
      break;

    // TODO
    case BOP_MINUS:
      if (q->sym2->kind == CONST_INT)
        printf("\tli $t0, %d\n", q->sym2->var->val.entier);
      else
        printf("\tlw $t0, %s\n", q->sym2->name);
      if (q->sym3->kind == CONST_INT)
        printf("\tli $t1, %d\n", q->sym3->var->val.entier);
      else
        printf("\tlw $t1, %s\n", q->sym3->name);
      printf("\tsub $t0,$t0,$t1\n");
      printf("\tsw $t0, %s\n", q->sym1->name);
      break;

    // TODO
    case BOP_MULT:
      if (q->sym2->kind == CONST_INT)
        printf("\tli $t0, %d\n", q->sym2->var->val.entier);
      else
        printf("\tlw $t0, %s\n", q->sym2->name);
      if (q->sym3->kind == CONST_INT)
        printf("\tli $t1, %d\n", q->sym3->var->val.entier);
      else
        printf("\tlw $t1, %s\n", q->sym3->name);
      printf("\tmul $t0,$t0,$t1\n");
      printf("\tsw $t0, %s\n", q->sym1->name);
      break;

    // TODO
    case UOP_MINUS:
      if (q->sym2->kind == CONST_INT)
        printf("\tli $t0, %d\n", q->sym2->var->val.entier);
      else
        printf("\tlw $t0, %s\n", q->sym2->name);
      printf("\tnegu $t0,$t0\n");
      printf("\tsw $t0, %s\n", q->sym1->name);
      break;

    case CALL_PRINT:
      if (q->sym1->var->type == INT) {
        printf("\tli $v0,1\n");
        printf("\tlw $a0, %s\n", q->sym1->name);
      } else if (q->sym1->var->type == FLOAT) {
        printf("\tli $v0,2\n");
        printf("\tlwc1 $f12, %s\n", q->sym1->name);
      }
      printf("\tsyscall\n");
      break;

    case CALL_PRINTMAT:
      // charger les dimensions de la matrice
      printf("\tlw $t0, %s_rows\n", q->sym1->name);
      printf("\tlw $t1, %s_cols\n", q->sym1->name);

      printf("\tli $t2, 0\n");
      printf("boucle_afficher_matrix_%s_ligne_%d:\n", q->sym1->name, cpt_label);

      printf("\tli $t3, 0\n");
      printf("boucle_afficher_matrix_%s_colonne_%d:\n", q->sym1->name,
             cpt_label);

      // indice dans la matrice
      printf("\tmul $t4, $t2, $t1\n");
      printf("\tadd $t5, $t4, $t3\n");

      // afficher l'élément
      printf("\tla $t6, %s\n", q->sym1->name);
      printf("\tmul $t5, $t5, 4\n");
      printf("\tadd $t6, $t6, $t5\n");
      printf("\tlw $t7, 0($t6)\n");
      printf("\tmtc1 $t7, $f12\n");

      printf("\tli $v0, 2\n");
      printf("\tsyscall\n");

      // \t
      printf("\tli $v0, 4\n");
      printf("\tla $a0, tab\n");
      printf("\tsyscall\n");

      // indice colonne ++
      printf("\taddi $t3, $t3, 1\n");

      // indice colonne < matrix_cols
      printf("\tbne $t3, $t1, boucle_afficher_matrix_%s_colonne_%d\n",
             q->sym1->name, cpt_label);

      // \n
      printf("\tli $v0, 4\n");
      printf("\tla $a0, newline\n");
      printf("\tsyscall\n");

      // indice ligne ++
      printf("\taddi $t2, $t2, 1\n");

      // indice ligne < matrix_rows
      printf("\tbne $t2, $t0, boucle_afficher_matrix_%s_ligne_%d\n",
             q->sym1->name, cpt_label);
      cpt_label++;
      break;

    case CALL_PRINTF:
      printf("\tli $v0, 4\n");
      printf("\tla $a0, %s\n", q->sym1->name);
      printf("\tsyscall\n");

      break;

    default:
      printf("BUG\n");
      break;
  }
}

void code_dump(struct code *c) {
  unsigned int i;

  printf("\t.text\n");
  printf("main:\n");

  for (i = 0; i < c->nextquad; i++) {
    quad_dump(&(c->quads[i]));
  }

  printf("# exit\n");
  printf("\tli $v0,10\n");
  printf("\tsyscall\n");
}

void code_free(struct code *c) {
  free(c->quads);
  free(c);
}