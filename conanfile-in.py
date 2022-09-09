from conans import ConanFile
from conan.tools.gnu import Autotools

class Adftool(ConanFile):
    settings = "os", "arch", "compiler", "build_type"
    name = "adftool"
    requires = "gettext/[>0.0.0]", "hdf5/[>0.0.0]", "gmp/[>0.0.0]", "cpython/[>0.0.0]", "zlib/[>0.0.0]"
    version = "@VERSION@"
    generators = "AutotoolsToolchain", "AutotoolsDeps"
    exports_sources = "*"

    def build(self):
        autotools = Autotools(self)
        autotools.configure(args=["--enable-relocatable"])
        autotools.make()
        autotools.install()
