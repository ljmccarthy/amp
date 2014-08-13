(define cons (constructor <pair>))
(define car (accessor <pair> 'car))
(define cdr (accessor <pair> 'cdr))
(define set-car! (mutator <pair> 'car))
(define set-cdr! (mutator <pair> 'cdr))

(define (id x) x)
(define (list . xs) xs)
(define (>= x y) (not (< x y)))
(define (<= x y) (not (> x y)))
(define (nil? x) (eqv? x nil))

(define (caar x) (car (car x)))
(define (cadr x) (car (cdr x)))
(define (cdar x) (cdr (car x)))
(define (cddr x) (cdr (cdr x)))
(define (cadar x) (car (cdr (car x))))
(define (cddar x) (cdr (cdr (car x))))

(define (expand-and xs)
  (if (nil? xs)
      true
  (if (nil? (cdr xs))
      (car xs)
      (list 'if (car xs) (expand-and (cdr xs)) false))))

(define (expand-or xs)
  (if (nil? xs)
      false
  (if (nil? (cdr xs))
      (car xs)
      (list 'if (car xs) true (expand-or (cdr xs))))))

(define-macro (and . xs) (expand-and xs))
(define-macro (or . xs) (expand-or xs))

(define (syntax-error . msg)
  (apply error (cons "syntax error:" msg)))

(define (expand-cond p)
  (if (pair? p)
      (if (pair? (cdr p))
          (list 'if (car p)
                (cadr p)
                (expand-cond (cddr p)))
          (car p))
      (syntax-error "cond")))

(define-macro (cond . form)
  (expand-cond form))

(define (self-quoting? x)
  (not (or (pair? x) (symbol? x))))

(define (enquote x)
  (if (self-quoting? x)
      x
      (list 'quote x)))

(define (expand-quasiquote-pair x n)
  (list cons
        (expand-quasiquote (car x) n)
        (expand-quasiquote (cdr x) n)))

(define (expand-unquote x n)
  (cond (> n 0)
          (expand-quasiquote-pair x (- n 1))
        (nil? (cddr x))
          (cadr x)
        (syntax-error "unquote")))

(define (expand-unquote-splicing x n)
  (cond (> n 0)
          (list cons
                (expand-quasiquote-pair (car x) (- n 1))
                (expand-quasiquote (cdr x) (- n 1)))
        (nil? (cddar x))
          (list append
                (cadar x)
                (expand-quasiquote (cdr x) n))
        (syntax-error "unquote-splicing")))

(define (expand-quasiquote x n)
  (cond (not (pair? x))
          (enquote x)
        (eqv? (car x) 'quasiquote)
          (expand-quasiquote-pair x (+ n 1))
        (eqv? (car x) 'unquote)
          (expand-unquote x n)
        (and (eqv? n 0) (eqv? (car x) 'unquote-splicing))
          (syntax-error "unquote-splicing: not inside list")
        (and (pair? (car x)) (eqv? (caar x) 'unquote-splicing))
          (expand-unquote-splicing x n)
        (expand-quasiquote-pair x n)))

(define-macro (quasiquote x)
  (expand-quasiquote x 0))

(define-macro (unquote x)
  (syntax-error "unquote: not inside quasiquote"))

(define-macro (unquote-splicing x)
  (syntax-error "unquote-splicing: not inside quasiquote"))

(define (append xs ys)
  (if (nil? xs)
      ys
      (cons (car xs) (append (cdr xs) ys))))

(define (map f xs)
  (if (nil? xs)
      nil
      (cons (f (car xs)) (map f (cdr xs)))))

(define (foldl f x xs)
  (if (nil? xs)
      x
      (foldl f (f x (car xs)) (cdr xs))))

(define (foldr f x xs)
  (if (nil? xs)
      x
      (f (car xs) (foldr f x (cdr xs)))))

(define (flip f)
  (lambda (x y) (f y x)))

(define (reverse xs)
  (foldl (flip cons) '() xs))

(define (filter pred xs)
  (if (nil? xs)
      xs
      (if (pred (car xs))
          (cons (car xs) (filter pred (cdr xs)))
          (filter pred (cdr xs)))))

(define (concat xss)
  (foldl append '() xss))

(define (list? p)
  (cond (nil? p) true
        (pair? p) (list? (cdr p))
        false))

(define (merge-sorted xs ys)
  (if (nil? xs)
      ys
  (if (nil? ys)
      xs
  (if (< (car xs) (car ys))
      (cons (car xs) (merge-sorted (cdr xs) ys))
      (cons (car ys) (merge-sorted xs (cdr ys)))))))

(define (range i j)
  (if (<= i j)
      (cons i (range (+ i 1) j))
      nil))

(define (sum xs) (foldl + 0 xs))
(define (product xs) (foldl * 1 xs))
(define (any xs) (foldl or false xs))
(define (all xs) (foldl and true xs))

(define (abs n) (if (< n 0) (negate n) n))
(define (min a b) (if (< a b) a b))
(define (max a b) (if (< a b) b a))

(define-macro (when pred . then)
  (if (nil? then)
      (syntax-error "when")
      `(if ,pred (begin ,@then) nil)))

(define (expand-assert preds)
  (if (nil? preds)
      nil
      `(if ,(car preds)
           ,(expand-assert (cdr preds))
           (error "assertion failed:" ',(car preds)))))

(define-macro (assert . preds)
  (expand-assert preds))

(define-macro (define-record-type name slots)
  `(begin
      (define ,(make-symbol '< name '>) (make-record-type ',name ',slots))))

(define (factorial n) (product (range 2 n)))

(define (ins x ys)
  (cond (nil? ys) (list x)
        (< x (car ys)) (cons x ys)
        (cons (car ys) (ins x (cdr ys)))))

(define (insort xs)
  (if (nil? xs)
      nil
      (ins (car xs) (insort (cdr xs)))))

(puts "Amp (c) 2006-2007 Luke McCarthy")
