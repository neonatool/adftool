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
