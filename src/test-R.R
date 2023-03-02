set.seed (0)

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

## Checking the file
file.remove ("test-R.adf")
f <- new (mod$file, "test-R.adf", TRUE)
data <- matrix (rnorm (15), 5, 3)
f$set_eeg_data (5, 3, c (t (data)))

## Setting the EEG data added some metadata to the file…
wildcard <- new (mod$statement)
dataset_statements <- f$lookup (wildcard)
dataset_terms <- lapply (dataset_statements, function (st) st$get_terms ())
dataset_subjects <- lapply (dataset_terms, function (terms) terms$subject)
dataset_predicates <- lapply (dataset_terms, function (terms) terms$predicate)
dataset_objects <- lapply (dataset_terms, function (terms) terms$object)
dataset_graphs <- lapply (dataset_terms, function (terms) terms$graph)
dataset <- data.frame (subject = sapply (dataset_subjects, function (st) st$to_n3 ()),
                       predicate = sapply (dataset_predicates, function (st) st$to_n3 ()),
                       object = sapply (dataset_objects, function (st) st$to_n3 ()),
                       graph = sapply (dataset_graphs, function (st) st$to_n3 ()),
                       deletion_date = sapply (dataset_statements, function (st) {
                           date <- st$get_deletion_date ()
                           if (length (date) == 0) {
                               date <- NA
                           }
                           date[1]
                       }))
print (dataset)

eeg_itself <- new (mod$term)
eeg_itself$set_named ("")

channel_ids <- f$lookup_objects (eeg_itself, "https://localhost/lytonepal#has-channel")

if (length (channel_ids) != 3) {
    stop ("The file did not create identifiers for the channel data!")
}

## Let’s take any channel... The second one for instance.
picked <- 2
channel <- channel_ids[[picked]]
column_number <- f$lookup_integer (channel, "https://localhost/lytonepal#column-number")
if (length (column_number) != 1) {
    stop (paste0 (channel$to_n3 (), " does not have exactly 1 column number"))
}
eeg_data <- f$get_eeg_data (0, 10, column_number)
if (eeg_data$n != 5 || eeg_data$p != 3) {
    stop ("Invalid data dimension")
}
if (length (eeg_data$data) != 10) {
    stop ("Requested size has not been respected")
}
if (any (is.finite (eeg_data$data[6:10]))) {
    stop ("Eeg data not padded with NaNs")
}
if (any (abs (eeg_data$data[1:5] - data[1:5, picked]) >= 1e-4)) {
    stop ("Eeg data loaded the wrong column")
}

## What if we pick a channel that does not exist?
eeg_data <- f$get_eeg_data (0, 10, 42)
if (eeg_data$n != 5 || eeg_data$p != 3) {
    stop ("Invalid data dimension")
}
if (length (eeg_data$data) != 10) {
    stop ("Requested size has not been respected")
}
if (any (is.finite (eeg_data$data))) {
    stop ("Eeg data should have used NaNs for the whole request.")
}

file.remove ("test-R.adf")

## Check strings
file <- new (mod$file, "test-R.adf", TRUE)
gen_comment <- function (comment, langtag) {
    subject <- eeg_itself
    predicate <- new (mod$term)
    predicate$set_named ("http://www.w3.org/2000/01/rdf-schema#comment")
    object <- new (mod$term)
    object$set_langstring (comment, langtag)
    statement <- new (mod$statement)
    statement$set (list (subject = subject, predicate = predicate, object = object))
    file$insert (statement)
}

gen_comment ("En français", "fr-FR")
gen_comment ("In English", "en-US")

comments <- file$lookup_string (eeg_itself, "http://www.w3.org/2000/01/rdf-schema#comment")

stopifnot (comments$`fr-FR` == "En français")
stopifnot (comments$`en-US` == "In English")

file.remove ("test-R.adf")
