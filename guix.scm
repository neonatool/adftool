(define-module (guix))

(use-modules
 ((guix licenses) #:prefix license:)
 (gnu packages autotools)
 (gnu packages base)
 (gnu packages bash)
 (gnu packages check)
 (gnu packages code)
 (gnu packages compression)
 (gnu packages cran)
 (gnu packages emacs)
 (gnu packages emacs-xyz)
 (gnu packages flex)
 (gnu packages gettext)
 (gnu packages image-processing)
 (gnu packages machine-learning)
 (gnu packages maths)
 (gnu packages multiprecision)
 (gnu packages perl)
 (gnu packages pkg-config)
 (gnu packages python)
 (gnu packages python-build)
 (gnu packages python-check)
 (gnu packages python-science)
 (gnu packages python-web)
 (gnu packages python-xyz)
 (gnu packages qt)
 (gnu packages rdf)
 (gnu packages sphinx)
 (gnu packages statistics)
 (gnu packages tex)
 (gnu packages texinfo)
 (gnu packages time)
 (gnu packages valgrind)
 (gnu packages version-control)
 (gnu packages video)
 (gnu packages xml)
 (gnu packages xorg)
 (guix build utils)
 (guix build-system python)
 (guix build-system trivial)
 (guix build-system gnu)
 (guix derivations)
 (guix download)
 (guix gexp)
 (guix git-download)
 (guix git)
 (guix modules)
 (guix monads)
 (guix packages)
 (guix store)
 (guix utils)
 (ice-9 match)
 (ice-9 popen)
 (ice-9 rdelim)
 )

(define git-exec
  (run-with-store
   (open-connection)
   (mlet %store-monad ((git (package->derivation git)))
         (mlet %store-monad ((built
                              (built-derivations
                               (list git))))
               (return (string-append (derivation->output-path git)
                                      "/bin/git"))))))

(format (current-error-port) "Working with git: ~a\n" git-exec)

(define adftool:package-version
          (with-directory-excursion
           (dirname (current-filename))
           (let ((port (open-pipe* OPEN_READ git-exec "describe" "--tags")))
             (let ((version (read-line port)))
               (close-port port)
	       version))))

(format (current-error-port) "The package version is: ~a\n" adftool:package-version)

(define adftool:package-source
  (local-file (dirname (current-filename))
              "bplus-source"
              #:recursive? #t
              #:select? (lambda (filename stat)
			  (and (not (string-suffix? "/gnulib" filename))
			       (not (string-suffix? "/hdf5" filename))))))

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

(package
 (name "adftool")
 (version adftool:package-version)
 (source adftool:package-source)
 (build-system gnu-build-system)
 (outputs
  '("out" "devel" "pdf" "html"))
 (arguments
  (list
   #:configure-flags
   #~`("--enable-relocatable")
   #:phases
   #~(modify-phases
      %standard-phases
      (add-before
       'bootstrap 'prepare
       (lambda _
	 (invoke "git" "submodule" "deinit" "hdf5")
	 (invoke "git" "rm" "hdf5")
	 (delete-file ".gitignore")
	 (invoke "git" "clean" "-f" "-d")
	 (invoke "git" "clean" "-f" "-d")
	 (invoke "git" "clean" "-f" "-d")
	 (symlink #$gnulib-patched "gnulib")
	 (setenv "GNULIB_SRCDIR" #$gnulib-patched)
	 (invoke "cp" "-r" #$(file-append gnulib-patched "/top/.") "./")
         (patch-shebang "autopull.sh")
         (patch-shebang "autogen.sh")))
      (add-after
       'bootstrap 'fix-/bin/sh-in-po
       (lambda _
	 (substitute*
	  "po/Makefile.in.in"
	  (("SHELL = /bin/sh")
	   "SHELL = @SHELL@"))))
      (add-before
       'check 'set-timezone
       (lambda* (#:key inputs #:allow-other-keys)
	 (setenv "TZ" "UTC+1")
	 (setenv "TZDIR" (search-input-directory inputs "share/zoneinfo"))))
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
	 (mkdir-p (string-append #$output:devel "/share/adftool/"))
	 (for-each
	  (lambda (file)
	    (copy-file
	     file
	     (string-append #$output:devel "/share/adftool/" file)))
	  '("GTAGS" "GRTAGS" "GPATH"))))
      (add-after
       'build 'pdf
       (lambda _
	 (invoke "make" "-j" "pdf")))
      (add-after
       'install 'install-pdf
       (lambda* (#:key outputs #:allow-other-keys)
	 (invoke "make" "-j" "install-pdf"
		 (format #f "prefix = ~a" #$output:pdf))))
      (add-after
       'build 'html
       (lambda _
	 (invoke "make" "-j" "html")))
      (add-after
       'install 'install-html
       (lambda* (#:key outputs #:allow-other-keys)
	 (invoke "make" "-j" "install-html"
		 (format #f "prefix = ~a" #$output:html))))
      (add-after
       'check 'syntax-check
       (lambda* _
	 (invoke "make" "syntax-check")))
      (add-after
       'check 'distcheck
       (lambda* _
	 (setenv "LD_LIBRARY_PATH" ".libs")
	 (invoke "make" "-j" "distcheck"))))))
 (native-inputs
  (list emacs emacs-org autoconf autoconf-archive automake libtool gnu-gettext
	perl git
	valgrind (list glibc "debug")
	tar global flex r pkg-config
	texinfo (texlive-updmap.cfg (list texlive))
	r-minimal r-rcpp tzdata-for-tests))
 (inputs
  (list hdf5 check gnu-gettext zlib gmp python))
 (home-page "https://plmlab.math.cnrs.fr/vkraus/bplus")
 (synopsis "B+ implementatiof for adftool")
 (description "This library provides a B+ implementation for adftool.")
 (license ((@@ (guix licenses) license) "?" "?" "?")))
