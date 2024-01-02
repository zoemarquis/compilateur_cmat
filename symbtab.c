#include "symbtab.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "EXPR.tab.h"

unsigned string_num = 1;
unsigned const_int_num = 1;
unsigned const_float_num = 1;
unsigned extract_num = 1;
unsigned indices_num = 1;

SymTable *symtable_new() {
  SymTable *t = malloc(sizeof(SymTable));
  t->capacity = 1024;
  t->symbols = malloc(t->capacity * sizeof(Symbol));
  t->temporary = 0;
  t->size = 0;
  return t;
}

static void symtable_grow(SymTable *t) {
  t->capacity += 1024;
  t->symbols = realloc(t->symbols, t->capacity * sizeof(Symbol));
  if (t->symbols == NULL) {
    fprintf(stderr,
            "Erreur lors de la tentative d'aggreandissement de la table des "
            "symboles (taille actuelle = %d)\n",
            t->size);
    exit(1);
  }
}

Symbol *symtable_const_int(SymTable *t, int v) {
  unsigned int i;
  for (i = 0; i < t->size && t->symbols[i].const_int != v; i++)
    ;
  if (i == t->size) {
    if (t->size == t->capacity) symtable_grow(t);
    Symbol *s = &(t->symbols[t->size]);
    s->kind = CONST_INT;
    sprintf(s->name, "%s%d", "cint", const_int_num);
    const_int_num++;
    s->const_int = v;
    ++(t->size);
    return s;
  } else {
    return &(t->symbols[i]);
  }
}

Symbol *symtable_const_float(SymTable *t, float v) {
  unsigned int i;
  for (i = 0; i < t->size && t->symbols[i].const_float != v; i++)
    ;
  if (i == t->size) {
    if (t->size == t->capacity) symtable_grow(t);
    Symbol *s = &(t->symbols[t->size]);
    s->kind = CONST_FLOAT;
    sprintf(s->name, "%s%d", "cflt", const_float_num);
    const_float_num++;
    s->const_float = v;
    ++(t->size);
    return s;
  } else {
    return &(t->symbols[i]);
  }
}

Symbol *symtable_string(SymTable *t, const char *string) {
  if (t->size == t->capacity) symtable_grow(t);
  Symbol *s = &(t->symbols[t->size]);
  s->kind = STRING;
  sprintf(s->name, "%s%d", "str", string_num);
  string_num++;
  s->string = string;
  ++(t->size);
  return s;
}

Symbol *symtable_extract(SymTable *t, Extract extract) {
  if (t->size == t->capacity) symtable_grow(t);
  Symbol *s = &(t->symbols[t->size]);
  s->kind = EXTRACT;
  sprintf(s->name, "%s%d", "extr", extract_num);
  extract_num++;
  s->extr = extract;
  ++(t->size);
  return s;
}

Symbol *symtable_indices(SymTable *t, Indices tuple) {
  if (t->size == t->capacity) symtable_grow(t);
  Symbol *s = &(t->symbols[t->size]);
  s->kind = INDICES;
  sprintf(s->name, "%s%d", "ind", indices_num);
  indices_num++;
  s->tuple = tuple;
  ++(t->size);
  return s;
}

Symbol *symtable_get(SymTable *t, const char *id) {
  unsigned int i;
  for (i = 0; i < t->size && strcmp(t->symbols[i].name, id) != 0; i++)
    ;
  if (i < t->size) return &(t->symbols[i]);
  return NULL;
}

Symbol *symtable_put(SymTable *t, const char *id, variable *var) {
  if (t->size == t->capacity) symtable_grow(t);
  Symbol *s = &(t->symbols[t->size]);
  s->kind = NAME;
  strcpy(s->name, id);
  s->var = var;
  ++(t->size);
  return s;
}

// inutile ?
void symtable_dump(SymTable *t) {
  unsigned int i;
  for (i = 0; i < t->size; i++) {
    if (t->symbols[i].kind == CONST_INT)
      printf("       %p = %d\n", &(t->symbols[i]),
             t->symbols[i].var->val.entier);
    if (t->symbols[i].kind == NAME)
      printf("       %p = %s\n", &(t->symbols[i]), t->symbols[i].name);
  }
  printf("       --------\n");
}

void symtable_free(SymTable *t) {
  unsigned int i;
  for (i = 0; i < t->size; i++) {
    if (t->symbols[i].kind == NAME) {
      delete_var(t->symbols[i].var);
    } else if (t->symbols[i].kind == STRING) {
      free(t->symbols[i].string);
    } else if (t->symbols[i].kind == EXTRACT) {
      free(t->symbols[i].extr.liste);
    }
  }
  free(t->symbols);
  free(t);
}

// VARIABLE

variable *creer_variable(name_t name, unsigned type, bool init, valeur val) {
  variable *var = (variable *)malloc(sizeof(variable));
  strcpy(var->name, name);
  var->type = type;
  var->init = init;
  var->val = val;
  return var;
}

variable **creer_var_liste(variable *v1) {
  variable **liste = (variable **)malloc(sizeof(variable *));
  liste[0] = v1;
  return liste;
}

variable **ajouter_var_liste(variable **liste, unsigned taille, variable *var) {
  liste = (variable **)realloc(liste, taille * sizeof(variable *));
  liste[taille - 1] = var;
  return liste;
}

void delete_var(variable *var) {
  if (var->type == MATRIX) {
    delete_matrix(var->val.matrix);
  }
  if (var) free(var);
}

// MATRIX

Matrix *create_matrix(unsigned l, unsigned c) {
  Matrix *matrix = (Matrix *)malloc(sizeof(Matrix));
  matrix->l = l;
  matrix->c = c;
  matrix->data = (float *)calloc(l * c, sizeof(float));
  return matrix;
}

void printmat(Matrix *matrix) {
  for (unsigned i = 0; i < matrix->l; i++) {
    for (unsigned j = 0; j < matrix->c; j++) {
      printf("%f\t", matrix->data[(i * matrix->c) + j]);
    }
    printf("\n");
  }
}

void delete_matrix(Matrix *matrix) {
  free(matrix->data);
  free(matrix);
}

void put_value_at(Matrix *m, unsigned ligne, unsigned colonne, float valeur) {
  assert(ligne < m->l && colonne < m->c);
  m->data[ligne * m->c + colonne] = valeur;
}

void add_colonne(Matrix *m) {
  assert(m->l == 1);
  m->c++;
  m->data = (float *)realloc(m->data, m->c * sizeof(float));
}

void add_ligne(Matrix *m, Matrix *m2) {
  assert(m->c == m2->c);
  assert(m2->l == 1);
  m->l++;
  m->data = (float *)realloc(m->data, m->l * m->c * sizeof(float));
  for (unsigned i = 0; i < (m->c); i++) {
    m->data[(m->l - 1) * (m->c) + i] = m2->data[i];
  }
  delete_matrix(m2);
}

// extraction

Matrix *extraction(Matrix *a, int *lignes, int *colonnes, int n, int m) {
  Matrix *mnew = create_matrix(n, m);
  int k = 0;
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < m; j++) {
      int l = lignes[i];
      int c = colonnes[j];
      mnew->data[k++] = a->data[l * a->c + c];
    }
  }
  return mnew;
}

Extract creer_liste_extract(int valeur) {
  int *v = (int *)malloc(sizeof(int));
  *v = valeur;
  return (Extract){v, 1};
}

Extract creer_liste_extract_intervalle(int vmin, int vmax) {
  int *l = (int *)malloc(sizeof(int) * (vmax - vmin + 1));
  for (int i = 0; i < (vmax - vmin + 1); i++) {
    l[i] = vmin + i;
  }
  return (Extract){l, vmax - vmin + 1};
}

Extract concat_extract_liste(Extract e1, Extract e2) {
  int *l = (int *)malloc(sizeof(int) * (e1.taille + e2.taille));
  for (unsigned i = 0; i < e1.taille; i++) {
    l[i] = e1.liste[i];
  }
  for (unsigned i = 0; i < e2.taille; i++) {
    l[i + e1.taille] = e2.liste[i];
  }
  free(e1.liste);
  free(e2.liste);
  return (Extract){l, e1.taille + e2.taille};
}