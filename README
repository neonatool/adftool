# ADFTool

## Requirement

Install the relevant packages. The GitHub actions environment, using
ubuntu, we use the following packages:

`gettext libgmp-dev libhdf5-dev zlib1g-dev autoconf autoconf-archive
automake libtool valgrind tar global autopoint make gcc m4 libtinfo5
flex r-base texinfo texlive libpython3-dev pkg-config emacs check
r-base r-cran-rcpp`

## Build from source

To build from the git repository, run:

```
git clone https://github.com/neonatool/adftool
git submodule update --init --recursive
```

```sh
./bootstrap-bootstrap
./bootstrap
./configure
# To maximize the chance to prevent bugs early on, we use these arguments:
#./configure CFLAGS="-g2 -O2 -DFORTIFY_SOURCE=2 -fsanitize=undefined,address -fsanitize-undefined-trap-on-error -Wall -Wextra"
make -j
make -j check
```