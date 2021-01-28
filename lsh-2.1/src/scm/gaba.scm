;; gaba.scm
;;
;; Run with
;;   $ scsh -e main -l scsh-compat.scm -l compiler.scm -s gaba.scm
;;   $ guile -e main -l guile-compat.scm -l compiler.scm -s gaba.scm

;; Reads a C source file on stdin. Comments of the form
;;
;; /*
;; GABA:
;;    expression
;; */
;;
;; are treated specially, and C code for the class is written to
;; stdout. Typically, the code is saved to a file and included by the
;; C source file in question.

;; FIXME: Perhaps the files should somehow be fed through the
;; preprocessor first?

(define (werror f . args)
  (display (apply format #f f args) (current-error-port)))

(define (string-prefix? prefix s)
  (let ((l (string-length prefix)))
    (and (<= l (string-length s))
	 (string=? prefix (substring s 0 l)))))

(define (read-expression p)
  (let ((line (read-line)))
    ; (werror "read line: '~s'\n" (if (eof-object? line) "<EOF>" line))
    (cond ((eof-object? line) line)
	  ((p line) (read))
	  (else (read-expression p)))))

(define (get key alist select)
  (cond ((assq key alist) => select)
	(else #f)))

(define (identity x) x)

(define (filter p list)
  (cond ((null? list) list)
	((p (car list)) (cons (car list)
			      (filter p (cdr list))))
	(else (filter p (cdr list)))))

(define (list-prefix l n)
  (if (zero? n) '()
      (cons (car l) (list-prefix (cdr l) (- n 1)))))

(define (atom? o) (not (list? o)))

(define (nth n l)
  (cond ((< n 0) (error "nth: negative index not allowed" n))
        ((null? l) (error "nth: index too big" n))
        ((= n 0) (car l))
        (else (nth (- n 1) (cdr l)))))

(define-syntax when
  (syntax-rules ()
    ((when <cond> . <body>)
     (if <cond> (begin . <body>)))))

(define-syntax unless
  (syntax-rules ()
    ((unless <cond> . <body>)
     (if (not <cond>) (begin . <body>)))))
  
;; Variables are describes as lists (name . type)
;; Known types (and corresponding C declarations) are
;;
;; (string)                     struct lsh_string *name
;; (object class)               struct class *name
;; (bignum)                     mpz_t name
;; (simple c-type)              c-type
;; (special c-type mark-fn free-fn)
;; (indirect-special c-type mark-fn free-fn)
;;
;; (struct tag)
;;
;; (array type size)            type name[size]
;;
;; size-field, when present, is the name of a field that holds
;; the current size of variable size objects.
;;
;; Variable size array (must be last) */
;; (var-array type size-field)  type name[1]
;;
;; FIXME: Split into var-pointer and var-space?
;; (pointer type [size-field])  type *name
;; (space type [size-field])    Like pointer, but should be freed
;;
;; (function type . arg-types) type name(arg-types)
;;
;; NOTE: For function types, the arguments are represented simply as
;; strings or lists containing C declarations; they do not use the
;; type syntax.
;;
;; (method type args)
;; is transformed into (pointer (function type self-arg args)) before
;; processing,
;;
;; (const . type)               Like type, but declared const.
;;                              Primarily used for const string.

;;; C code generation

;; A portion of C code is represented as a either
;;
;; an atom (string, symbol or number), or
;;
;; procedure taking a single INDENT argument, sending
;; output to current-output-stream, or
;;
;; a list, whose elements are displayed indented one more level.
;;
;; #f, which is ignored
;;
;; It would be cleaner to let indent be a dynamically bound variable.

(define (out level . args)
  (for-each (lambda (o)
	      (cond ((procedure? o) (o level))
		    ((list? o) (apply out (+ 1 level) o))
		    (o (display o))))
	    args))

; This isn't very optimal
(define (indent i)
  (display "\n")
  (let loop ((count 0))
    (when (< count i)
	  (display "  ")
	  (loop (+ 1 count)))))
	     
(define (c-append . args)
  (lambda (i) (apply out i args)))

(define (c-string name)
  ;; FIXME: Could do quoting better
  (c-append "\"" name "\""))

(define (c-address expr)
  (c-append "&(" expr ")"))

(define (c-list separator list)
      (if (null? list) '()
	  (cons (car list)
		(map (lambda (o)
		       (c-append separator o))
		     (cdr list)))))

(define (c-block statements)
  (c-append "{" (map (lambda (s) (c-append indent s ";"))
		     statements)
	    indent "}"))

(define (c-block* . statements) (c-block statements))

(define (c-initializer expressions)
  (c-append "{" (map (lambda (s) (c-append indent s ","))
		     expressions)
	    indent "}"))

(define (c-initializer* . expressions) (c-initializer expressions))

(define (c-prototype return name args)
  (c-append return indent name
	    "("
	    (if (null? args ) "void"
		(c-list (c-append "," indent) args))
	    ")"))

(define (c-prototype* return name . args)
  (c-prototype return name args))

(define (c-for var range body)
  (c-append "for(" var "=0; "
	    var "<" range "; "
	    var "++)"
	    (list indent body)))

(define (c-call f args)
  (c-append f "(" (c-list (c-append "," indent) args) ")"))

(define (c-call* f . args) (c-call f args))

(define (c-declare var)
  (define (c-decl-1 type expr)
    (case (car type)
      ((simple special indirect-special)
       (c-append (cadr type) " " expr))
      ((string)
       (c-append "struct lsh_string *" expr))
      ((object)
       (c-append "struct " (cadr type) " *" expr))
      ((struct)
       (c-append "struct " (cadr type) " " expr))
      ((bignum)
       (c-append "mpz_t " expr))
      ((pointer space)
       (c-decl-1 (cadr type) 
		 (c-append "(*(" expr "))")))
      ((array)
       (c-decl-1 (cadr type)
		 (c-append "((" expr ")[" (caddr type) "])")))
      ((var-array)
       (c-decl-1 (cadr type)
		 (c-append "((" expr ")[1])")))
      ((function)
       (c-decl-1 (cadr type) 
		 (c-append expr "(" (c-list "," (cddr type)) ")")))
      ((const)
       (c-append "const " (c-decl-1 (cdr type) expr)))
      (else (error "c-decl: Invalid type " type))))
  (c-decl-1 (var-type var) (var-name var)))

(define (c-struct name vars)
  (c-append "struct " name indent
	    (c-block (map c-declare vars))
	    ";" indent))


(define var-name car)
(define var-type cdr)

; New version
(define (make-instance-struct name super vars)
  (c-struct name (cons `(super struct ,(or super "lsh_object"))
		       vars)))

; For counter variables
(define make-var
  (let ((*count* 0))
    (lambda ()
      (set! *count* (+ 1 *count*))
      (c-append "k" *count*))))

; Invokes f on type and expression for each variable.
(define (map-variables f vars pointer)
  (filter identity (map (lambda (var)
			  (f (var-type var)
			     (c-append pointer "->" (var-name var))))
			vars)))

(define (make-marker type expr)
  (case (car type)
    ((string simple function bignum) #f)
    ((object) (c-call* "mark" (c-append "(struct lsh_object *) " expr)))
    ((struct) (c-call* (c-append (cadr type) "_mark")
		       (c-address expr)
		       "mark"))
    ((pointer space)
     (if (null? (cddr type))
	 (make-marker (cadr type)
		      (c-append "*(" expr ")"))
	 ;; The optional argument should be the name of
	 ;; an instance variable holding the length of
	 ;; the area pointed to.
	 (let* ((counter (make-var))
		(mark-k (make-marker (cadr type)
				     (c-append "(" expr ")[" counter "]"))))
	   (and mark-k
		(c-block* (c-declare `( ,counter simple unsigned))
			  (c-for counter (c-append "i->" (caddr type))
				 mark-k))))))
    ((special)
     (let ((mark-fn (caddr type)))
       (and mark-fn (c-call* mark-fn expr "mark"))))
      
    ((indirect-special)
     (let ((mark-fn (caddr type)))
       (and mark-fn (c-call* mark-fn
			     (c-address expr)
			     "mark"))))
    ((array)
     (let* ((counter (make-var))
	    (mark-k (make-marker (cadr type)
				 (c-append "(" expr ")[" counter "]"))))
       (and mark-k
	    (c-block* (c-declare `( ,counter simple unsigned))
		      (c-for counter (caddr type)
			     mark-k)))))
    ((var-array)
     (let* ((counter (make-var))
	    (mark-k (make-marker (cadr type)
				 (c-append "(" expr ")[" counter "]"))))
       (and mark-k
	    (c-block* (c-declare `( ,counter simple unsigned))
		      (c-for counter (c-append "i->" (caddr type))
			     mark-k)))))
    ((const) (make-marker (cdr type) expr))
    (else (error "make-marker: Invalid type " type))))

(define (make-mark-function name vars)
  (let ((markers (map-variables make-marker vars "i")))
    (and (not (null? markers))
	 (c-append (c-prototype* "static void" (c-append "do_" name "_mark")
				 "struct lsh_object *o"
				 "void (*mark)(struct lsh_object *o)")
		   indent
		   (c-block (cons (c-append "struct " name
					    " *i = (struct " name " *) o")
				  markers))
		   indent))))

(define (make-freer type expr)
  (case (car type)
    ((object simple function pointer) #f)
    ((struct) (c-call* (c-append (cadr type) "_free") (c-address expr)))
    ((string) (c-call* "lsh_string_free" expr))
    ((bignum) (c-call* "mpz_clear" expr))
    ((space)
     (let* ((free-space (c-call* "lsh_space_free" expr))
	    (counter (make-var))
	    (free-k (and (not (null? (cddr type)))
			 ;; The optional argument should be the name
			 ;; of an instance variable holding the length
			 ;; of the area pointed to.
			 (make-freer (cadr type)
				     (c-append "(" expr ")[" counter "]")))))
       (if free-k
	   (c-block* (c-declare `( ,counter simple unsigned))
			   (c-for counter (c-append "i->" (caddr type))
				  free-k)
			   free-space)
	   free-space)))
    ((special) (c-call* (cadddr type) expr))
    ((indirect-special)
     (let ((free (cadddr type)))
       (and free (c-call* free (c-address expr)))))
    ((array)
     (let* ((counter (make-var))
	    (free-k (make-freer (cadr type)
				(c-append "(" expr ")[" counter "]"))))
       (and free-k
	    (c-block* (c-declare `( ,counter simple unsigned))
		      (c-for counter (caddr type)
			     free-k)))))

    ((var-array)
     (let* ((counter (make-var))
	    (free-k (make-freer (cadr type)
				(c-append "(" expr ")[" counter "]"))))
       (and free-k
	    (c-block* (c-declare `( ,counter simple unsigned))
		      (c-for counter (c-append "i->" (caddr type))
			     free-k)))))
    ((const) (make-freer (cdr type) expr))
    (else (error "make-freer: Invalid type " type))))

(define (make-free-function name vars)
  (let ((freers (map-variables make-freer vars "i")))
    (and (not (null? freers))
	 (c-append (c-prototype* "static void" (c-append "do_" name "_free")
				 "struct lsh_object *o")
		   indent
		   (c-block (cons (c-append "struct " name
					    " *i = (struct " name " *) o")
				  freers))
		   indent))))
	 
(define (struct-mark-prototype name)
  (c-append "void " name "_mark(struct " name " *i,\n"
	    " void (*mark)(struct lsh_object *o))"))

(define (struct-mark-function name vars)
  (c-append (struct-mark-prototype name) indent
	    (c-block
	     ;; To avoid warnings for unused parameters
	     (cons "(void) mark; (void) i"
		   (map-variables make-marker vars "i")))
	    indent))

(define (struct-free-prototype name)
  (c-append "void " name "_free(struct " name " *i)"))

(define (struct-free-function name vars)
  (c-append (struct-free-prototype name) indent
	    (c-block
	     ;; To avoid warnings for unused parameters
	     (cons "(void) i"
		   (map-variables make-freer vars "i")))
	    indent))

(define (make-class name super mark free meta methods)
  (let ((initializer
	 (c-initializer*
	  "STATIC_HEADER"
	  (if super
	      ;; FIXME: A cast (struct lsh_class *) or something
	      ;; equivalent is needed if the super class is not a
	      ;; struct lsh_class *. For now, fixed with macros
	      ;; expanding to the right component of extended class
	      ;; structures.
	      (c-address (c-append super "_class"))
	      "NULL")
	  (c-string name)
	  ;; FIXME: For classes using var-array,
	  ;; we should emit offsetof(struct <name>, <last-var>)
	  ;; instead of sizeof.
	  (c-call* "sizeof" (c-append "struct " name))
	  (if mark (c-append "do_" name "_mark") "NULL")
	  (if free (c-append "do_" name "_free") "NULL"))))
    (if meta
	(c-append "struct " meta "_meta "name "_class_extended ="
		  indent
		  (c-initializer (cons initializer (or methods '())))
		  ";" indent)
	(c-append "struct lsh_class " name "_class ="
		  indent initializer ";" indent))))

(define (make-meta name methods)
  (c-append "struct " name "_meta" indent
	    (c-block (cons "struct lsh_class super"
			   methods))
	    ";" indent)) 

(define (declare-struct-mark-function name)
  (list "void "	name "_mark(struct " name " *i, \n"
	"    void (*mark)(struct lsh_object *o))"))

(define (declare-struct-free-function name)
  (list "void " name "_free(struct " name " *i)"))

(define (preprocess-vars name vars)
  (define (preprocess-type type)
    (if (atom? type)
	`(simple ,type)
	(case (car type)
	  ;; Primitive types
	  ((string object bignum simple special indirect-special struct)
	   type)
	  ;; Second element is a type
	  ((array var-array pointer space function)
	   `( ,(car type) ,(preprocess-type (cadr type)) ,@(cddr type)))
	  ;; Tail is a type
	  ((const)
	   (cons 'const (preprocess-type (cdr type))))
	  ;; Shorthands
	  ((method)
	   `(pointer (function ,(preprocess-type (cadr type))
			       ("struct " ,name " *self")
			       ,@(cddr type))))
	  ((indirect-method)
	   `(pointer (function ,(preprocess-type (cadr type))
			       ("struct " ,name " **self")
			       ,@(cddr type))))
	  (else (error "preprocess-type: Invalid type " type)))))
    
  (map (lambda (var)
	 (cons (var-name var) (preprocess-type (var-type var))))
       vars))

(define (class-annotate name super meta)
  (c-append "/*\nCLASS:" name ":" (or super "")
	    (if meta (list ":" meta "_meta") "") "\n*/\n"))

(define (process-class attributes)
  (let* ((name (get 'name attributes cadr))
	 (condition (get 'condition attributes cadr))
	 (super (get 'super attributes cadr))
	 (vars (preprocess-vars name (get 'vars attributes cdr)))
	 (meta (get 'meta attributes cadr))
	 (methods (get 'methods attributes cdr)))
    (werror "Processing class ~S\n" name)
    ; (werror "foo\n")
    (let ((mark-function (make-mark-function name vars))
	  (free-function (make-free-function name vars)))
					; (werror "baar\n")
      (c-append (class-annotate name super meta)
		(and condition (c-append "#if " condition "\n"))
		"#ifndef GABA_DEFINE\n"	
		(make-instance-struct name super vars)
		(if meta
		    (c-append "extern struct " meta "_meta "
			      name "_class_extended;\n"
			      "#define " name "_class (" name
			      "_class_extended.super)\n")
		    (c-append "extern struct lsh_class " name "_class;\n"))
		"#endif /* !GABA_DEFINE */\n\n"
		"#ifndef GABA_DECLARE\n"
		(or mark-function "")
		(or free-function "")
		(make-class name super mark-function free-function
			    meta methods)
		"#endif /* !GABA_DECLARE */\n\n"
		(and condition (c-append "#endif /* " condition " */\n")) ))))

(define (process-meta attributes)
  (let ((name (get 'name attributes cadr))
	(methods (get 'methods attributes cdr)))
    (werror "Processing meta ~S\n" name)
    (c-append "#ifndef GABA_DEFINE\n"
	      (make-meta name methods)
	      "#endif /* !GABA_DEFINE */"
	      indent)))

(define (process-struct attributes)
  (let* ((name (get 'name attributes cadr))
	 ;; FIXME: Do we really handle super?
	 (super (get 'super attributes cadr))
	 (vars (preprocess-vars name (get 'vars attributes cdr)))
	 (meta (get 'meta attributes cadr))
	 (methods (get 'methods attributes cdr)))
    (werror "Processing struct ~S\n" name)
    ;; (werror "foo\n")
    ;; FIXME: Is this really needed?
    ;; (werror "baar\n")
    (c-append "#ifndef GABA_DEFINE\n"	
	      (c-struct name vars)
	      "extern " (struct-mark-prototype name) ";\n"
	      "extern " (struct-free-prototype name) ";\n"
	      "#endif /* !GABA_DEFINE */\n\n"
	      "#ifndef GABA_DECLARE\n"
	      (struct-mark-function name vars)
	      (struct-free-function name vars)
	      "#endif /* !GABA_DECLARE */\n\n")))


;;;; Expression compiler

;; Constants is an alist of (name value call_1 call_2 ... call_n)
;; where value is a C expression representing the value. call_i is
;; present, it is a function that can be called to apply the value to
;; i arguments directly.
(define (make-output constants expr)
  ;; OP and ARGS are C expressons
  (define (apply-generic op args)
    ;; (werror "(apply-generic ~S)\n" (cons op args))
    (if (null? args) op
	(apply-generic (c-call* "A" op (car args))
		       (cdr args))))
  ;; INFO is the (value [n]) associated with a constant,
  ;; and ARGS is a list of C expressions
  (define (apply-constant info args)
    ;; (werror "apply-constant : ~S\n" info)
    ;; (werror "          args : ~S\n" args)
    (let ((calls (cdr info)))
      (if (null? calls)
	(apply-generic (car info) args)
	(let ((n (min (length calls) (length args))))
	  ;; (werror "n: ~S\n" n)
	  (apply-generic (c-call (nth n info)
				 (list-prefix args n))
			 (list-tail args n))))))
  (define (lookup-global v)
    (cond ((assq v constants) => cdr)
	  (else (list (string-upcase (symbol->string v))))))
  
  (define (output-expression expr)
    ;; (werror "output-expression ~S\n" expr)
    (if (atom? expr)
	(car (lookup-global expr))
	(let ((op (application-op expr))
	      (args (map output-expression (application-args expr))))
	  (if (atom? op)
	      (apply-constant (lookup-global op) args)
	      (apply-generic op args)))))
  (output-expression expr))

(define (process-expr attributes)
  (define (params->alist params)
    (map (lambda (var)
	   (let ((name (var-name var)))
	     (list name (list "((struct lsh_object *) " name ")" ))))
	 params))
  
  ;; (werror "foo\n")
  (let ((name (get 'name attributes cadr))
	(globals (or (get 'globals attributes cdr) '()))
	(params (preprocess-vars #f
				 (or (get 'params attributes cdr) '())))
	(expr (get 'expr attributes cadr)))
    (werror "Processing expression ~S\n" name)
    (let ((translated (translate expr)))
      (werror "Compiled to ~S\n" translated)
      ;; (werror "Globals: ~S\n" globals)
      ;; (werror "Params: ~S\n" params)
      (c-append (c-prototype "static struct command *" name
			     (map c-declare params))
		indent
		(format #f "  /* ~S */\n" translated)
		"#define A GABA_APPLY\n"
		"#define I GABA_VALUE_I\n"
		"#define K GABA_VALUE_K\n"
		"#define K1 GABA_APPLY_K_1\n"
		"#define S GABA_VALUE_S\n"
		"#define S1 GABA_APPLY_S_1\n"
		"#define S2 GABA_APPLY_S_2\n"
		"#define B GABA_VALUE_B\n"
		"#define B1 GABA_APPLY_B_1\n"
		"#define B2 GABA_APPLY_B_2\n"
		"#define C GABA_VALUE_C\n"
		"#define C1 GABA_APPLY_C_1\n"
		"#define C2 GABA_APPLY_C_2\n"
		"#define Sp GABA_VALUE_Sp\n"
		"#define Sp1 GABA_APPLY_Sp_1\n"
		"#define Sp2 GABA_APPLY_Sp_2\n"
		"#define Sp3 GABA_APPLY_Sp_3\n"
		"#define Bp GABA_VALUE_Bp\n"
		"#define Bp1 GABA_APPLY_Bp_1\n"
		"#define Bp2 GABA_APPLY_Bp_2\n"
		"#define Bp3 GABA_APPLY_Bp_3\n"
		"#define Cp GABA_VALUE_Cp\n"
		"#define Cp1 GABA_APPLY_Cp_1\n"
		"#define Cp2 GABA_APPLY_Cp_2\n"
		"#define Cp3 GABA_APPLY_Cp_3\n"
		;; "  trace(\"Entering " name "\\n\");\n"
		(c-block*
		 (c-append
		  "CAST_SUBTYPE(command, res,\n"
		  (c-call* "MAKE_TRACE"
			  (c-string name)
			  (make-output (append '( (I I)
						   (K K K1)
						   (S S S1 S2)
						   (B B B1 B2)
						   (C C C1 C2)
						   (S* Sp Sp1 Sp2 Sp3)
						   (B* Bp Bp1 Bp2 Bp3)
						   (C* Cp Cp1 Cp2 Cp3))
						globals
						(if params
						    (params->alist params)
						    '()))
				       translated))
		  ");\n"
		  "return res;\n"))
		indent
		"#undef A\n"
		"#undef I\n" 
		"#undef K\n"
		"#undef K1\n"
		"#undef S\n"
		"#undef S1\n"
		"#undef S2\n"
		"#undef B\n"
		"#undef B1\n"
		"#undef B2\n"
		"#undef C\n"
		"#undef C1\n"
		"#undef C2\n"
		"#undef Sp\n"
		"#undef Sp1\n"
		"#undef Sp2\n"
		"#undef Sp3\n"
		"#undef Bp\n"
		"#undef Bp1\n"
		"#undef Bp2\n"
		"#undef Bp3\n"
		"#undef Cp\n"
		"#undef Cp1\n"
		"#undef Cp2\n"
		"#undef Cp3\n"))))

(define (process-input exp)
  (let ((type (car exp))
	(body (cdr exp)))
    ;; (werror "process-class: type = ~S\n" type)
    (case type
      ((class) (process-class body))
      ((meta) (process-meta body))
      ((struct) (process-struct body))
      ((expr) (process-expr body))
      (else (list "#error Unknown expression type " type "\n")))))

(define (main args)
  (define (loop)
    (let ((exp (read-expression
		(lambda (s) (string-prefix? "/* GABA:" s)))))
      (unless (eof-object? exp)
	      (out 0 (process-input exp))
	      (loop))))
  (loop)
  (exit 0))
