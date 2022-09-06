(use-modules
 (guix build utils)
 (guix derivations)
 (guix gexp)
 (guix modules)
 (guix monads)
 (guix packages)
 (guix store)
 (ice-9 ftw)
 )

(define adftool-devel
  (run-with-store
   (open-connection)
   (mlet %store-monad ((adftool
			(package->derivation
			 (load "./adftool.scm"))))
	 (mlet %store-monad ((built
			      (built-derivations
			       (list adftool))))
	       (return (string-append (derivation->output-path adftool "devel")
				      "/share/adftool/"))))))

(define (enter? name stat initialized?)
  #t)

(define (leaf name stat initialized?)
  (false-if-exception
   (delete-file (basename name)))
  (copy-file name (basename name))
  (chmod (basename name) #o644)
  initialized?)

(define (down name stat initialized?)
  (when initialized?
    (mkdir-p (basename name))
    (chdir (basename name)))
  #t)

(define (up name stat initialized?)
  (chdir "..")
  initialized?)

(define (skip name stat initialized?)
  initialized?)

(define (error name stat initialized?)
  initialized?)

(file-system-fold enter? leaf down up skip error
		  #f
		  adftool-devel)
