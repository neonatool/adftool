dlname <- Sys.getenv ("LIBADFTOOL_R_LIBRARY", unset = "ask")

if (dlname == "ask") {
    dlname <- readline ("Where is the libadftool-r shared object? ")
}

mod <- Rcpp::Module ("adftool", dyn.load (dlname))

example_date <- as.POSIXct ("2022-02-23 09:32:43 CET") + 0.5761111

date_term <- new (mod$term)
date_term$set_date (example_date)
n3_value <- date_term$to_n3 ()

reparsed <- new (mod$term)
parse_result <- reparsed$parse_n3 (paste0 (n3_value, "hello, world!"))

if (! (parse_result$success)) {
    stop ("Parse failure")
}

if (parse_result$rest != "hello, world!") {
    stop ("Incorrect parse")
}

found_date <- reparsed$as_date ()

if (length (found_date) == 0) {
    stop ("This is not a date")
}

loss <- example_date - found_date

print (loss)

if (loss > 1e-9) {
    stop ("Wrong parsed value")
}

## Check the statements
a <- new (mod$term)
a$set_named ("a")
b <- new (mod$term)
b$set_named ("b")
c <- new (mod$term)
c$set_named ("c")
d <- new (mod$term)
d$set_named ("d")

live_statement <- new (mod$statement)
live_statement$set (list (subject = a, predicate = b, object = c, graph = d, deletion_date = FALSE))
live_statement_parameters <- live_statement$get_terms ()
if (live_statement_parameters$subject$compare (live_statement_parameters$predicate) == 0) {
    stop ("Impossible, <a> != <b>.")
}
if (live_statement_parameters$subject$compare (a) != 0) {
    stop ("Impossible, <a> is the subject.")
}
if (length (live_statement$get_deletion_date ()) != 0) {
    stop ("Impossible, the statement is live.")
}

other_statement <- new (mod$statement)
other_statement$set (list (subject = a, predicate = c, object = a, graph = b))
compared <- live_statement$compare (other_statement, "SPOG")
if (compared >= 0) {
    stop ("Impossible, <a> <b> <c> <d> . comes before <a> <c> <a> <b> . in the SPOG order.")
}
compared <- live_statement$compare (other_statement, "GSPO")
if (compared <= 0) {
    stop ("Impossible, <a> <b> <c> <d> . comes after <a> <c> <a> <b> . in the gspo order.")
}

## The authorities complain that the other statement is
## unacceptable. It is deleted 42 milliseconds after january 1st,
## 1970, midnight.
other_statement$set (list (deletion_date = 42))
if (other_statement$get_deletion_date () != 42) {
    stop ("Impossible, it has been deleted.")
}

## Let’s construct a pattern with only "a" as the subject.
only_subject <- new (mod$statement)
only_subject$set (list (subject = a, predicate = b)) #! See later
compared_to_other <- only_subject$compare (other_statement, "SPOG")
if (compared_to_other != 0) {
    ## Huh?? They should not overlap!
    ## ...
    ## ...
    ## Oh, I set predicate = b! Sorry, I did not mean it.
    only_subject$set (list (predicate = FALSE))
    compared_to_other <- only_subject$compare (other_statement, "SPOG")
    if (compared_to_other == 0) {
        ## There you go, pattern with only a as the subject and the
        ## other pattern do overlap.
    } else {
        stop ("This is impossible, your eyes are betraying you :D")
    }
} else {
    stop ("Stop interfering with my story.")
}

compared_to_live <- only_subject$compare (live_statement, "SPOG")
compared_to_other <- only_subject$compare (other_statement, "SPOG")

if (compared_to_live != 0) {
    stop ("Impossible, subject pattern and live statement match.")
}

if (compared_to_live != 0) {
    stop ("Impossible, subject pattern and live statement also match.")
}

## Checking the band-pass filter
sfreq <- 256
data <- rnorm (5120)

filter <- new (mod$fir, 256, 0.3, 35)
filtered <- filter$apply (data)

if (length (filtered) != 5120) {
    stop ("The filter application always returns the same number of points.")
}

print (summary (filtered))
