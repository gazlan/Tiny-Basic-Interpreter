#ifndef TINY_H_1528923969
#define TINY_H_1528923969

typedef union {
  long alignment;
  char ag_vt_2[sizeof(int)];
  char ag_vt_4[sizeof(symbol *)];
  char ag_vt_5[sizeof(ExpressionList *)];
} tiny_vs_type;

typedef enum {
  tiny_basic_token = 1, tiny_white_space_token, tiny_then_part_token = 7,
  tiny_all_character_string_token, tiny_basic_statement_list_token = 27,
  tiny_eof_token, tiny_basic_line_token, tiny_opt_line_number_token,
  tiny_basic_statement_token, tiny_declaration_token,
  tiny_line_number_token, tiny_decl_var_list_token = 35,
  tiny_string_decl_var_list_token = 38, tiny_variable_name_token,
  tiny_assignment_token = 49, tiny_conditional_token,
  tiny_forloop_token = 54, tiny_input_token = 56, tiny_output_token,
  tiny_expression_token = 60, tiny_variable_name_list_token = 66,
  tiny_expression_list_token = 68, tiny_if_part_token, tiny_nothing_token,
  tiny_else_part_token, tiny_optional_step_token = 77,
  tiny_unary_expression_token = 79, tiny_postfix_expression_token,
  tiny_primary_expression_token, tiny_all_letters_token = 88,
  tiny_letter_token, tiny_digit_token = 91, tiny_eol_token = 95,
  tiny_integer_token = 101, tiny_name_string_token = 107,
  tiny_String_Value_token = 148
} tiny_token_type;

typedef struct tiny_pcb_struct{
  tiny_token_type token_number, reduction_token, error_frame_token;
  int input_code;
  int input_value;
  int line, column;
  int ssx, sn, error_frame_ssx;
  int drt, dssx, dsn;
  int ss[128];
  tiny_vs_type vs[128];
  int ag_ap;
  char *error_message;
  char read_flag;
  char exit_flag;
  int bts[128], btsx;
  unsigned char * pointer;
  unsigned char * la_ptr;
  const unsigned char *key_sp;
  int save_index, key_state;
  char ag_msg[82];
} tiny_pcb_type;

#ifndef PRULE_CONTEXT
#define PRULE_CONTEXT(pcb)  (&((pcb).cs[(pcb).ssx]))
#define PERROR_CONTEXT(pcb) ((pcb).cs[(pcb).error_frame_ssx])
#define PCONTEXT(pcb)       ((pcb).cs[(pcb).ssx])
#endif

#ifndef AG_RUNNING_CODE_CODE
/* PCB.exit_flag values */
#define AG_RUNNING_CODE         0
#define AG_SUCCESS_CODE         1
#define AG_SYNTAX_ERROR_CODE    2
#define AG_REDUCTION_ERROR_CODE 3
#define AG_STACK_ERROR_CODE     4
#define AG_SEMANTIC_ERROR_CODE  5
#endif
void init_tiny(void);
void tiny(void);

int tiny_value(void);
#endif

