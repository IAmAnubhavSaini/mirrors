;; FIXME: Turn this into a scheme48 module

(define-syntax let-and
  (syntax-rules ()
		((let-and (expr) clause clauses ...)
		 (and expr (let-and clause clauses ...)))
		((let-and (name expr) clause clauses ...)
		 (let ((name expr))
		   (and name (let-and clause clauses ...))))
		((let-and expr) expr)))

(define (split-list-at n list)
  (let loop ((left n) (collected '()) (tail list))
    (if (= left 0) (cons (reverse collected) tail)
	(loop (- left 1) (cons (car tail) collected) (cdr tail)))))

	
(define (atom? o) (not (list? o)))
(define (lambda? o) (and (pair? o) (eq? 'lambda (car o))))

(define (make-lambda formal body) `(lambda ,formal ,body))
(define lambda-formal cadr)
(define lambda-body caddr)

(define make-application list)
(define application-op car)
(define application-arg cadr)
(define application-args cdr)

(define (normalize-application op args)
  (if (null? args) op
      (normalize-application (make-application op (car args)) (cdr args))))

;; Transform (a b c)-> ((a b) c) and
;; (lambda (a b) ...) -> (lambda a (lambda b ...)
(define (make-preprocess specials)

  (define (preprocess expr)
    (if (atom? expr) expr
	(let ((op (car expr)))
	  (cond ((and (atom? op)
		      (assq op specials))
		 => (lambda (pair) ((cdr pair) (cdr expr) preprocess)))
		(else
		 (normalize-application (preprocess op)
					(map preprocess (cdr expr))))))))
  preprocess)

(define preprocess-applications (make-preprocess '()))

(define (do-lambda args preprocess)
  (let loop ((formals (reverse (car args)))
	     (body (preprocess (cadr args))))
    (if (null? formals) body
	(loop (cdr formals)
	      (make-lambda (car formals) body)))))

(define (do-let* args preprocess)
  (let loop ((definitions (reverse (car args)))
	     (body (preprocess (cadr args))))
    (if (null? definitions) body
	(loop (cdr definitions)
	      (make-application
	       (make-lambda (caar definitions)
			    body)
	       (preprocess (cadar definitions)))))))

(define (do-let args preprocess)
  (let ((definitions (car args))
	(body (cadr args)))
    (normalize-application 
     (do-lambda (list (map car definitions) body) preprocess)
     (map (lambda (d) (preprocess (cadr d))) definitions))))

(define preprocess (make-preprocess
		    `((lambda . ,do-lambda)
		      (let . ,do-let)
		      (let* . ,do-let*))))
  
;; (define (free-variable? v expr)
;;   (cond ((atom? expr) (eq? v expr))
;; 	   ((lambda? expr)
;; 	    (and (not (eq? v (lambda-formal expr)))
;; 		 (free-variable? v (lambda-body expr))))
;; 	   (else
;; 	    (or (free-variable? v (application-op expr))
;; 		(free-variable? v (application-arg expr))))))

(define (match pattern expr)
  (if (atom? pattern)
      (if (eq? '* pattern) (list expr)
	  (and (eq? pattern expr) '()))
      (let-and ((pair? expr))
	       (op-matches (match (application-op pattern)
				  (application-op expr)))
	       (arg-matches (match (application-arg pattern)
				   (application-arg expr)))
	       (append op-matches arg-matches))))

(define (rule pattern f)
  (cons (preprocess-applications pattern) f))

;;; The reduction rules for our combinators are

;; I x        --> x
;; K a b      --> b
;; S f g x    --> (f x) (g x)
;; B f g x    --> f (g x)
;; C f y x    --> (f x) y
;; S* c f g x --> c (f x) (g x)
;; B* c f g x --> c (f (g x))
;; C* c f y x --> c (f x) y

(define (make-K e) (make-combine 'K e))
(define (make-S p q) (make-combine 'S p q))
;; (define (make-B p) (make-combine 'B p))
;; (define (make-C p q) (make-combine 'C p q))
;; (define (make-S* p q) (make-combine 'S* p q))
;; (define (make-B* p q) (make-combine 'B* p q))
;; (define (make-C* p q) (make-combine 'C* p q))

;; Some more patterns that can be useful for optimization. From "A
;; combinator-based compiler for a functional language" by Hudak &
;; Kranz.

;; S K => K I
;; S (K I) => I
;; S (K (K x)) => K (K x)
;; S (K x) I => x
;; S (K x) (K y) => K (x y)
;; S f g x = f x (g x)
;; K x y => x
;; I x => x
;; Y (K x) => x

(define optimizations
  (list (rule '(S (K *) (K *)) (lambda (p q) (make-K (make-application p q))))
	(rule '(S (K *) I) (lambda (p) p))
	;; (rule '(B K I) (lambda () 'K))
	(rule '(S (K *) (B * *)) (lambda (p q r) (make-combine 'B* p q r)))
	(rule '(S (K *) *) (lambda (p q) (make-combine 'B p q)))
	(rule '(S (B * *) (K *))  (lambda (p q r) (make-combine 'C* p q r)))
	;; (rule '(C (B * *) *) (lambda (p q r) (make-combine 'C* p q r)))
	(rule '(S * (K *)) (lambda (p q) (make-combine 'C p q)))
	(rule '(S (B * * ) r) (lambda (p q r) (make-combine 'S* p q r)))))

(define (optimize expr)
  ;; (werror "optimize ~S\n" expr)
  (let loop ((rules optimizations))
    ;; (if (not (null? rules)) (werror "trying pattern ~S\n" (caar rules)) )
    (cond ((null? rules) expr)
	  ((match (caar rules) expr)
	   => (lambda (parts) (apply (cdar rules) parts)))
	  (else (loop (cdr rules))))))

(define (optimize-application op args)
  (if (null? args) op
      (optimize-application (optimize (make-application op (car args)))
			    (cdr args))))

(define (make-combine op . args)
  (optimize-application op args))

(define (translate-expression expr)
  (cond ((atom? expr) expr)
	((lambda? expr)
	 (translate-lambda (lambda-formal expr)
			   (translate-expression (lambda-body expr))))
	(else
	 (make-application (translate-expression (application-op expr))
			  (translate-expression (application-arg expr))))))

(define (translate-lambda v expr)
  (cond ((atom? expr)
	 (if (eq? v expr) 'I (make-K expr)))
	((lambda? expr)
	 (error "translate-lambda: Unexpected lambda" expr))
	(else
	 (make-S (translate-lambda v (application-op expr))
		       (translate-lambda v (application-arg expr))))))
  
(define (make-flat-application op args)
  (if (atom? op) `(,op ,@args)
      `(,@op ,@args)))
      
(define (flatten-application expr)
  (if (or (atom? expr) (lambda? expr)) expr
      (make-flat-application (flatten-application (application-op expr))
			     (map flatten-application (application-args expr)))))

(define (translate expr)
  (flatten-application (translate-expression (preprocess expr))))

;;; Test cases
;; (translate '(lambda (port connection)
;;                 (start-io (listen port connection)
;;                 (open-direct-tcpip connection))))
;;  ===> (C (B* S (B start-io) listen) open-direct-tcpip)
;; 
;; (translate '(lambda (f) ((lambda (x) (f (lambda (z) ((x x) z))))
;; 			    (lambda (x) (f (lambda (z) ((x x) z)))) )))
;; ===> (S (C B (S I I)) (C B (S I I)))
;; 
;; (translate '(lambda (r) (lambda (x) (if (= x 0) 1 (* x (r (- x 1)))))))
;; ===> (B* (S (C* if (C = 0) 1)) (S *) (C B (C - 1)))
;;
;; (translate '(lambda (file)
;;    (let ((ctx (spki_make_context (prog1 algorithms file))))
;;      (for_sexp (lambda (e) ctx) (spki_add_acl ctx) file))))
;; ===> '(S (C (S (B for_sexp K) spki_add_acl)) (B spki_make_context (prog1 algorithms)))

;;; Reduction machine, mainly for testing. Works on the flattened representation

(define reductions
  (let ((A (lambda (op . args) (make-flat-application op args))))
    `( (I 1 ,(lambda (x) x))
       (K 2 ,(lambda (a b) b))
       (S 3 ,(lambda (f g x) (A (A f x)
				(A g x))))
       (B 3 ,(lambda (f g x) (A f
				(A g x))))
       (C 3 ,(lambda (f y x) (A (A f x)
				y)))
       (S* 4 ,(lambda (c f g x) (A c
				   (A f x)
				   (A g x))))
       (B* 4 ,(lambda (c f g x) (A c
				   (A f
				      (A g x)))))
       (C* 4 ,(lambda (c f y x) (A c
				   (A f x)
				   y))))))

(define (reduce/rule rule args)
  (let ((needed (cadr rule))
	(available (length args))
	(transform (caddr rule)))
    (cond ((= needed available)
	   (apply transform args))
	  ((< needed available)
	   (let ((pair (split-list-at needed args)))
	     (make-flat-application (apply transform (car pair))
				    (cdr pair))))
	  (else #f))))

;; Reduces the top-level expression; returns #f if no reduction is possible
(define (reduce-1 e)	    
  (if (atom? e) #f
      (let* ((op (application-op e))
	     (reduction (assq op reductions)))
	;; (werror "op = ~s, reduction: ~s\n" op reduction)
	(and reduction (reduce/rule reduction (application-args e))))))

(define (reduce-expr e)
  (let loop ((e e))
    (let ((new (reduce-1 e)))
      (if new (loop new) e))))
