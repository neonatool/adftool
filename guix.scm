(use-modules
 (gnu packages autotools)
 (gnu packages base)
 (gnu packages bash)
 (gnu packages code)
 (gnu packages compression)
 (gnu packages cran)
 (gnu packages flex)
 (gnu packages gettext)
 (gnu packages maths)
 (gnu packages multiprecision)
 (gnu packages node)
 (gnu packages statistics)
 (gnu packages tex)
 (gnu packages texinfo)
 (gnu packages valgrind)
 (gnu packages version-control)
 (gnu packages web)
 (guix build utils)
 (guix build-system copy)
 (guix build-system gnu)
 (guix derivations)
 (guix gexp)
 (guix git)
 (guix modules)
 (guix monads)
 (guix packages)
 (guix store)
 (ice-9 popen)
 (ice-9 rdelim)
 )

(define gnulib
  (git-checkout
   (url "https://git.savannah.gnu.org/git/gnulib.git")))

(define gnulib-patched
  (directory-union
   "gnulib-patched"
   (list
    (file-union
     "gnulib-tool-patched"
     `(("gnulib-tool"
	,(program-file
	  "gnulib-tool"
	  (with-imported-modules
	   (source-module-closure '((guix build utils)))
	   #~(begin
	       (use-modules (guix build utils))
	       (apply invoke
		      #$(file-append bash "/bin/bash")
		      #$(file-append gnulib "/gnulib-tool")
		      (cdr (command-line)))))))))
    gnulib)))

(define git-exec
  (run-with-store
   (open-connection)
   (mlet %store-monad ((git (package->derivation git)))
         (mlet %store-monad ((built
                              (built-derivations
                               (list git))))
               (return (string-append (derivation->output-path git)
                                      "/bin/git"))))))

(define adftool-version
  (with-directory-excursion
   (dirname (current-filename))
   (let ((port (open-pipe* OPEN_READ git-exec "describe" "--tags")))
     (let ((version (read-line port)))
       (close-port port)
       (if (eof-object? version)
	   "0.0.0"
	   version)))))

(define po-download
  (program-file
   "po-download"
   (with-imported-modules
    (source-module-closure '((guix build utils)))
    #~(begin
	(use-modules (guix build utils))
	(use-modules (ice-9 match) (ice-9 ftw))
	(match (command-line)
	  ((_ subdir "adftool")
	   (let ((enter? (lambda (name stat result) #t))
		 (leaf (lambda (name stat result)
			 (when (string-suffix? ".po" name)
			   (copy-file
			    name
			    (string-append subdir "/" (basename name)))
			   (chmod
			    (string-append subdir "/" (basename name))
			    #o644))))
		 (down (const #t))
		 (up (const #t))
		 (skip (const #t))
		 (error (const #t)))
	     (file-system-fold
	      enter? leaf down up skip error #t
	      #$(local-file "./po" "adftool-po"
			    #:recursive? #t
			    #:select?
			    (lambda (file stat)
			      (string-suffix? ".po" file)))))))))))

(define po-download-command-format
  #~(format #f "~a %s %s" #$po-download))

(package
 (name "adftool")
 (version adftool-version)
 (source
  (local-file "." "adftool-source"
	      #:recursive? #t
	      #:select?
	      (lambda (file stat)
		(and
		 (not (string-suffix? "/gnulib" file))
		 (not (string-suffix? "/hdf5" file))
		 (not (string-suffix? "/public" file))
		 (not (string-suffix? "~" file))
		 (not (string-suffix? ".git" file))
		 (not (string-suffix? ".pot" file))
		 (not (string-suffix? ".po" file))
		 ))))
 (build-system gnu-build-system)
 (outputs (list "out" "dist" "devel" "html" "pdf"))
 (arguments
  (list
   #:phases
   #~(modify-phases
      %standard-phases
      (add-after
       'unpack 'write-version
       (lambda _
	 (call-with-output-file ".tarball-version"
	   (lambda (port)
	     (display #$adftool-version port)))))
      (add-after
       'unpack 'setup-gnulib
       (lambda _
	 (setenv "GNULIB_SRCDIR" #$gnulib-patched)
	 (rename-file "bootstrap.conf" "bootstrap-maintainer.conf")
	 (call-with-output-file "bootstrap.conf"
	   (lambda (port)
	     (format port "po_download_command_format=~s\n"
		     #$po-download-command-format)
	     (format port "checkout_only_file=\n")
	     (format port "source ./bootstrap-maintainer.conf\n")))
         (patch-shebang "autopull.sh")
         (patch-shebang "autogen.sh")))
      (add-after
       'bootstrap 'fix-/bin/sh-in-po
       (lambda _
	 (substitute*
	  "po/Makefile.in.in"
	  (("SHELL = /bin/sh")
	   "SHELL = @SHELL@"))))
      (add-after
       'check 'distcheck
       (lambda _
	 (invoke "make" "-j" "distcheck"
		 "DISTCHECK_CONFIGURE_FLAGS=\"LDFLAGS=-Wl,-rpath -Wl,./.libs\"")))
      (add-after
       'install 'install-source
       (lambda* (#:key outputs #:allow-other-keys)
	 (mkdir-p (string-append (assoc-ref outputs "dist") "/share/adftool"))
	 (invoke "tar" "-x"
		 "-C" (string-append (assoc-ref outputs "dist") "/share/adftool")
		 "-f" (string-append #$name "-" #$version ".tar.gz"))))
      (add-after
       'check 'tags
       (lambda _
	 (for-each
	  (lambda (file)
	    (false-if-exception
	     (delete-file file)))
	  '("GTAGS" "GRTAGS" "GPATH"))
	 (invoke "make" "-j" "GTAGS")))
      (add-after
       'install 'install-tags
       (lambda* (#:key outputs #:allow-other-keys)
	 (mkdir-p (string-append
		   (assoc-ref outputs "devel")
		   "/share/adftool/"))
	 (for-each
	  (lambda (file)
	    (copy-file
	     file
	     (string-append
	      (assoc-ref outputs "devel")
	      "/share/adftool/"
	      file)))
	  '("GTAGS" "GRTAGS" "GPATH"))))
      (add-after
       'install 'updated-translations
       (lambda* (#:key outputs #:allow-other-keys)
	 (use-modules (ice-9 ftw))
	 (mkdir-p (string-append
		   (assoc-ref outputs "devel")
		   "/share/adftool/po/"))
	 (let ((enter?
		(lambda (name stat result)
		  (not (string-suffix? "/.reference" name))))
	       (leaf
		(lambda (name stat result)
		  (when (and (or (string-suffix? ".po" name)
				 (string-suffix? ".pot" name))
			     (not (string-prefix? "en@" name)))
		    (copy-file name
			       (string-append
				(assoc-ref outputs "devel")
				"/share/adftool/po/"
				(basename name))))))
	       (down
		(lambda (name stat result)
		  #t))
	       (up
		(lambda (name stat result)
		  #t))
	       (skip
		(lambda (name stat result)
		  result))
	       (error
		(lambda (name stat errno result)
		  result)))
	   (file-system-fold enter? leaf down up skip error
			     '()
			     "po"))))
      (add-after
       'build 'pdf
       (lambda _
	 (invoke "make" "-j" "pdf")))
      (add-after
       'install 'install-pdf
       (lambda* (#:key outputs #:allow-other-keys)
	 (invoke "make" "-j" "install-pdf"
		 (format #f "prefix = ~a" (assoc-ref outputs "pdf")))))
      (add-after
       'build 'html
       (lambda _
	 (invoke "make" "-j" "html")))
      (add-after
       'install 'install-html
       (lambda* (#:key outputs #:allow-other-keys)
	 (invoke "make" "-j" "install-html"
		 (format #f "prefix = ~a" (assoc-ref outputs "html"))))))))
 (inputs
  (list gnu-gettext hdf5 zlib gmp))
 (native-inputs
  (list autoconf autoconf-archive automake libtool
	valgrind
	;; valgrind needs to mess with strlen:
	(list glibc "debug")
	tar global gnu-gettext flex r
	texinfo
	(texlive-updmap.cfg (list texlive-epsf texlive-tex-texinfo))))
 (home-page "https://localhost/adftool")
 (synopsis "Tool to parse and generate my ADF spec")
 (description "The ADF specification is slightly incomplete, so I made one in the
same spirit but with more precise expectations.")
 (license "none"))
