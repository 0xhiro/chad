#include "../include/compiler/compiler.h"
#include "../include/utils/const.h"
#include "../include/utils/utils.h"

#include <string.h>

void interpret(chad_args_t chad_args, char *file_content) {

  // LEXER
  tracker_t *lexer_tracker = init_tracker();
  lexer_tracker->filename = chad_args.filename;
  lexer_t *lexer = init_lexer(file_content);
  lexer->tracker = lexer_tracker;
  lexer->tracker->mode = LEXER;
  bundle_t bundle = get_all_tokens(lexer);
  token_t **all_tokens = bundle.all_tokens;
  // print_all_tokens(all_tokens);

  // PARSER
  tracker_t *parser_tracker = init_tracker();
  parser_tracker->filename = chad_args.filename;
  parser_t *parser = init_parser(all_tokens, bundle.tokens_size);
  parser->tracker = parser_tracker;
  parser->tracker->mode = PARSER;
  parser->tracker->line = 1;
  parser->tracker->column = 1;
  scope_t *start_scope = init_scope();
  start_scope->tracker = parser->tracker;
  AST_T *root = parser_start(parser, start_scope);

  // construct a use statement for the core library
  AST_T *core_lib = init_ast(parser->tracker, AST_USE_STATEMENT);
  char *core_path = "core";
  char *core_name = "*";

  core_lib->use_file_path = malloc((strlen(core_path) + 1) * sizeof(char));
  core_lib->use_function_name = malloc((strlen(core_name) + 1) * sizeof(char));
  strcpy(core_lib->use_file_path, core_path);
  strcpy(core_lib->use_function_name, core_name);

  // add a use statement for the core library
  add_node_to_tail(core_lib, root);

  root = parser_hoist(parser, root, AST_USE_STATEMENT);
  root = parser_import(parser, root);
  root = parser_hoist(parser, root, AST_FUNCTION_DEFINITION);

  // construct a main function call
  AST_T *main_call = init_ast(parser->tracker, AST_FUNCTION_CALL);
  main_call->scope = root->scope;
  main_call->function_call_name = chad_args.start;
  main_call->function_call_arguments_size = 0;

  // add the main function call as the first node
  add_node_to_tail(main_call, root);

  // add an EOF node
  add_node_to_tail(init_ast(parser->tracker, AST_EOF), root);

  // print_all_nodes(root);

  // RUNTIME
  // BUG: i get a segfault on recurssive functions possibly due to stack
  // overflow
  tracker_t *visitor_tracker = init_tracker();
  visitor_tracker->filename = chad_args.filename;
  visitor_t *visitor = init_visitor();
  visitor->tracker = visitor_tracker;
  visitor->tracker->mode = RUNTIME;
  visit(visitor, root);

  log_error(NULL, "reached end of main file");
}
