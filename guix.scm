(use-modules
 (gnu packages autotools)
 (gnu packages version-control)
 (gnu packages web)
 (gnu packages node)
 (gnu packages bash)
 (gnu packages valgrind)
 (gnu packages base)
 (gnu packages code)
 (gnu packages gettext)
 (gnu packages maths)
 (gnu packages compression)
 (guix build utils)
 (guix build-system gnu)
 (guix build-system copy)
 (guix derivations)
 (guix gexp)
 (guix modules)
 (guix monads)
 (guix packages)
 (guix store)
 (guix git)
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
		 (not (string-suffix? "~" file))
		 (not (string-suffix? ".git" file))
		 ))))
 (build-system gnu-build-system)
 (outputs (list "out" "dist" "devel"))
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
       'unpack 'bootstrap-force
       (lambda _
	 (invoke "sh" "-c" "printf '\\ncheckout_only_file=\\n' >> bootstrap.conf")
	 (invoke "sh" "-c" "printf '\\ngnulib_tool=\\\"bash $gnulib_tool\\\"\\n' >> bootstrap.conf")))
      (add-after
       'unpack 'save-po-files
       (lambda _
	 (mkdir-p ".download-po")
	 (invoke "find" "po" "-name" "*.po" "-exec" "mv" "{}" ".download-po" ";")
	 (invoke "sh" "-c" "printf '\\npo_download_command_format=\"mv .download-po/*.po %%s/ ; echo %%s\"\\n' >> bootstrap.conf")))
      (add-before
       'bootstrap 'set-gnulib-srcdir
       (lambda* (#:key inputs native-inputs #:allow-other-keys)
         (patch-shebang "autopull.sh")
         (patch-shebang "autogen.sh")
	 (setenv "GNULIB_SRCDIR" #$gnulib-patched)))
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
		  #t))
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
			     "po")))))))
 (inputs
  (list gnu-gettext hdf5 zlib))
 (native-inputs
  (list autoconf autoconf-archive automake libtool
	valgrind
	;; valgrind needs to mess with strlen:
	(list glibc "debug")
	tar global gnu-gettext))
 (home-page "https://localhost/adftool")
 (synopsis "Tool to parse and generate my ADF spec")
 (description "The ADF specification is slightly incomplete, so I made one in the
same spirit but with more precise expectations.")
 (license "none"))
