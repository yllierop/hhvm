(copy_files smart_constructors/*.ml)

(library
  (name full_fidelity)
  (wrapped false)
  (modules
   full_fidelity_ast_types
   full_fidelity_parser_env
   full_fidelity_declaration_parser
   full_fidelity_declaration_parser_type
   full_fidelity_editable_positioned_original_source_data
   full_fidelity_editable_positioned_syntax
   full_fidelity_editable_positioned_token
   full_fidelity_editable_syntax
   full_fidelity_editable_token
   full_fidelity_editable_trivia
   full_fidelity_expression_parser
   full_fidelity_expression_parser_type
   full_fidelity_lexer
   full_fidelity_lexer_sig
   full_fidelity_minimal_syntax
   full_fidelity_minimal_token
   full_fidelity_minimal_trivia
   full_fidelity_operator_generated
   full_fidelity_operator
   full_fidelity_parser
   full_fidelity_parser_errors
   full_fidelity_parser_helpers
   full_fidelity_parser_profiling
   full_fidelity_positioned_syntax
   full_fidelity_positioned_token
   full_fidelity_positioned_trivia
   full_fidelity_precedence_parser
   full_fidelity_rewriter
   full_fidelity_simple_parser
   full_fidelity_source_text
   full_fidelity_statement_parser
   full_fidelity_statement_parser_type
   stats_container
   full_fidelity_token_kind
   full_fidelity_syntax
   full_fidelity_syntax_error
   full_fidelity_syntax_kind
   full_fidelity_syntax_tree
   full_fidelity_syntax_type
   full_fidelity_trivia_kind
   full_fidelity_type_lexer
   full_fidelity_type_parser
   full_fidelity_type_parser_type
   full_fidelity_validated_syntax
   lambda_analyzer
   full_fidelity_parser_context
   little_cache
   lexable_trivia_sig
   lexable_token_sig
   lexable_positioned_token_sig
   minimal_parser
   positioned_parser
   positioned_syntax_sig
   syntax_sig
   parserSig
   rust_parser_ffi
   rust_lazy_trivia_ffi
   syntaxTransforms

   ; from smart_constructors
   coroutine_smart_constructor
   smartConstructors
   smartConstructorsWrappers
   verifySmartConstructors
   declModeSmartConstructors
   syntaxSmartConstructors)
  (libraries
    common
    file_info
    hh_autoimport
    naming_special_names
    parser_options
    parser_schema
    parser_schema_def
    partial_provider
    relative_path

    rust_parser_ffi
    (select rust_lazy_trivia_ffi.ml from
      (-> ffi/rust_lazy_trivia_ffi.ml)
    )
    (select rust_parser_ffi.ml from
      (-> ffi/rust_parser_ffi.ml)
    )
  )
  (preprocess (pps visitors.ppx ppx_deriving.std)))

(library
  (name parser)
  (wrapped false)
  (modules
    ast_check
    docblock_finder
    full_fidelity_ast
    globalParserOptions
    hack_grammar_descriptor_helper
    html_entities
    ide_parser_cache
    limited_width_pretty_printing_library
    namespaces
    parser_return
    pretty_printing_library
    pretty_printing_library_sig
    random_ast_generator_config
    random_ast_generator)
  (preprocess (pps visitors.ppx ppx_deriving.std)) ; See T41851208
  (libraries
    ast
    fixme_provider
    full_fidelity
    heap_shared_mem
    logging
    namespace_env
    nast
    parser_ppl
    partial_provider
    utils_lint
    utils_php_escape))

(library
  (name hh_autoimport)
  (wrapped false)
  (modules hh_autoimport)
  (preprocess (pps visitors.ppx ppx_deriving.std)) ; See T41851208
  (libraries
    collections
    core_kernel))

(library
  (name parsing_service)
  (wrapped false)
  (modules
    parsing_service)
  (preprocess (pps visitors.ppx ppx_deriving.std)) ; See T41851208
  (libraries
    ast_provider
    procs_procs))
